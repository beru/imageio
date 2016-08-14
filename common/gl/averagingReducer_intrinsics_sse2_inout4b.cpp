#include "AveragingReducer_intrinsics_sse2_inout4b.h"

#include "AveragingReducer_intrinsics.h"

#include "common.h"
#include "arrayutil.h"
#include <algorithm>

#ifdef _DEBUG
#include <tchar.h>
#include "trace.h"
#endif

/*
reference

Visual C++ Language Reference Compiler Intrinsics
http://msdn.microsoft.com/ja-jp/library/26td21ds.aspx
http://msdn.microsoft.com/ja-jp/library/x8zs5twb.aspx

*/

namespace gl
{

namespace intrinsics_sse2_inout4b
{

uint16_t getTargetLineArrayCount(const AveragingReduceParams& params)
{
	const uint16_t newWidth = (params.srcWidth * params.widthRatioTarget) / params.widthRatioSource;
	return newWidth / 4 + ((newWidth % 4) ? 1 : 0);
}

class ILineAveragingReducer
{
public:
	virtual void fillRead(const __m128i* srcBuff, __m128i* tmpBuff) = 0;
	virtual void addRead(const __m128i* srcBuff, __m128i* tmpBuff) = 0;
};

#define ILineAveragingReducer_Impl \
	virtual void fillRead(const __m128i* srcBuff, __m128i* tmpBuff)	{ iterate<Operation_ReturnB>(srcBuff, tmpBuff); } \
	virtual void addRead(const __m128i* srcBuff, __m128i* tmpBuff)	{ iterate<Operation_mm_adds_epu16>(srcBuff, tmpBuff); } \

__forceinline __m128i make_vec_ratios(uint32_t numerator, uint32_t denominator)
{
	__m128d dummy;
	__m128d numerators = _mm_cvtsi32_sd(dummy, numerator);
	__m128d denominators = _mm_cvtsi32_sd(dummy, denominator);
	__m128d ratios = _mm_div_sd(numerators, denominators);
	ratios = _mm_add_sd(ratios, _mm_set_sd(0.5));
	__m128i ints = _mm_cvttpd_epi32(ratios);
	__m128i shorts = _mm_shufflelo_epi16(ints, 0);
	return _mm_unpacklo_epi64(shorts, shorts);
}

class Operation_ReturnA
{
public:
	static __forceinline __m128i work(__m128i a, __m128i b)
	{
		return a;
	}
};

class Operation_ReturnB
{
public:
	static __forceinline __m128i work(__m128i a, __m128i b)
	{
		return b;
	}
};

class Operation_mm_adds_epu16
{
public:
	static __forceinline __m128i work(__m128i a, __m128i b)
	{
		return _mm_adds_epu16(a, b);
	}
};

// 横2加算 8bits -> 16bits
// in	128bits * 1
// out	128bits * 1
__forceinline __m128i Scale2(__m128i src)
{
/*
	12345678abcdefgh
	↓
	1234abcd5678efgh
	↓
	 1 2 3 4 a b c d
	        +
	 5 6 7 8 e f g h
*/
	__m128i shuffled0 = _mm_shuffle_epi32(src, _MM_SHUFFLE(3, 1, 2, 0));
	__m128i twoPixels00 = _mm_unpacklo_epi8(shuffled0, _mm_setzero_si128());
	__m128i twoPixels01 = _mm_unpackhi_epi8(shuffled0, _mm_setzero_si128());
	__m128i result0 = _mm_add_epi16(twoPixels00, twoPixels01);
	
	return result0;
}

// 横3加算 8bits -> 16bits
// in	128bits * 3
// out	128bits * 2
__forceinline void Scale3(const __m128i* pSrc, __m128i& result0, __m128i& result1)
{
	// 3, 1 + 2, 2 + 1, 3
/*
	"12345678abcdefgh
	↓
	"5678abcd1234efgh
	'12345678abcdefgh
	↓
	" 1 2 3 4 e f g h
	" 5 6 7 8 a b c d
	' 1 2 3 4 5 6 7 8
	↓
	1 2 3 4 e f g h
	5 6 7 8 1 2 3 4
	a b c d 5 6 7 8

	"1234 "efgh
	"5678 '1234
	"abcd '5678

	'abcd `5678
	'efgh `abcd
	`1234 `efgh
*/
	__m128i fourPixels0 = pSrc[0];
	__m128i fourPixels1 = pSrc[1];
	__m128i fourPixels2 = pSrc[2];
	
	__m128i shuffled0 = _mm_shuffle_epi32(fourPixels0, _MM_SHUFFLE(2, 1, 3, 0));
	__m128i twoPixels00 = _mm_unpacklo_epi8(shuffled0, _mm_setzero_si128());
	__m128i twoPixels01 = _mm_unpackhi_epi8(shuffled0, _mm_setzero_si128());
	__m128i twoPixels10 = _mm_unpacklo_epi8(fourPixels1, _mm_setzero_si128());
	__m128i twoPixels11 = _mm_unpackhi_epi8(fourPixels1, _mm_setzero_si128());
	
	__m128i left00 = twoPixels00;
	__m128i left01 = _mm_unpacklo_epi64(twoPixels01, twoPixels10);
	__m128i left02 = _mm_unpackhi_epi64(twoPixels01, twoPixels10);
	result0 = _mm_add_epi16(_mm_add_epi16(left00, left01), left02);
	
	__m128i shuffled2 = _mm_shuffle_epi32(fourPixels2, _MM_SHUFFLE(2, 1, 3, 0));
	__m128i twoPixels20 = _mm_unpacklo_epi8(shuffled2, _mm_setzero_si128());
	__m128i twoPixels21 = _mm_unpackhi_epi8(shuffled2, _mm_setzero_si128());
	
	__m128i right00 = _mm_unpacklo_epi64(twoPixels11, twoPixels21);
	__m128i right01 = _mm_unpackhi_epi64(twoPixels11, twoPixels21);
	__m128i right02 = twoPixels20;
	result1 = _mm_add_epi16(_mm_add_epi16(right00, right01), right02);
}

// 横4加算 8bits -> 16bits
// in	128bits * 2
// out	128bits * 1
__forceinline __m128i Scale4(const __m128i* pSrc)
{
/*
	12345678abcdefgh
	↓
	 1 2 3 4 5 6 7 8
	        +
	 9 a b c d e f g
	↓
	 1 2 3 4 5 6 7 8

	"1234 '1234 
	"5678 '5678
	"abcd 'abcd
	"efgh 'efgh

*/
	__m128i fourPixels0 = pSrc[0];
	__m128i fourPixels1 = pSrc[1];
	
	__m128i twoPixels00 = _mm_unpacklo_epi8(fourPixels0, _mm_setzero_si128());
	__m128i twoPixels01 = _mm_unpackhi_epi8(fourPixels0, _mm_setzero_si128());
	__m128i sum0 = _mm_add_epi16(twoPixels00, twoPixels01);
	
	__m128i twoPixels10 = _mm_unpacklo_epi8(fourPixels1, _mm_setzero_si128());
	__m128i twoPixels11 = _mm_unpackhi_epi8(fourPixels1, _mm_setzero_si128());
	__m128i sum1 = _mm_add_epi16(twoPixels10, twoPixels11);
	
	__m128i ordered0 = _mm_unpacklo_epi64(sum0, sum1);
	__m128i ordered1 = _mm_unpackhi_epi64(sum0, sum1);
	
	__m128i result = _mm_add_epi16(ordered0, ordered1);

	return result;
}

// [1, 横3加算]
/*
123456789abcdefg
↓
 1 2 3 4 5 6 7 8
        +
 0 0 0 0 9 a b c
        +
 0 0 0 0 d e f g
*/
__forceinline __m128i Split_1_3(__m128i src)
{
/*
 1 2 3 4 5 6 7 8
 9 a b c d e f g

 1 2 3 4 5 6 7 8
 0 0 0 0 9 a b c
 0 0 0 0 d e f g
*/
	__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
	__m128i twoPixels1 = _mm_unpackhi_epi8(src, _mm_setzero_si128());
	__m128i right0 = _mm_slli_si128(twoPixels1, 8);
	__m128i right1 = _mm_unpackhi_epi64(_mm_setzero_si128(), twoPixels1);
	__m128i result = _mm_add_epi16(twoPixels0, _mm_add_epi16(right0, right1));
	
	return result;
}

// [横2加算]
/*
12345678********
↓
 1 2 3 4 5 6 7 8
        ＋
 5 6 7 8 * * * *
*/
__forceinline __m128i Split_2_0(__m128i src)
{
	__m128i sp01 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
	__m128i sp1 = _mm_srli_si128(sp01, 8);
	__m128i result = _mm_add_epi16(sp01, sp1);
	
	return result;
}

// [横2加算, 横2加算]
/*
123456789abcdefg
↓
12349abc5678defg
↓
 1 2 3 4 9 a b c
        +
 5 6 7 8 d e f g
*/
__forceinline __m128i Split_2_2(__m128i src)
{
	__m128i shuffled = _mm_shuffle_epi32(src, _MM_SHUFFLE(3, 1, 2, 0));
	__m128i twoPixels0 = _mm_unpacklo_epi8(shuffled, _mm_setzero_si128());
	__m128i twoPixels1 = _mm_unpackhi_epi8(shuffled, _mm_setzero_si128());
	__m128i result = _mm_add_epi16(twoPixels0, twoPixels1);
	
	return result;
}

// [横3加算, 1]
/*
123456789abcdefg
↓
 1 2 3 4 d e f g
        +
 5 6 7 8 0 0 0 0
        +
 9 a b c 0 0 0 0
*/
__forceinline __m128i Split_3_1(__m128i src)
{
/*
 1 2 3 4 0 0 0 0
        +
 5 6 7 8 0 0 0 0
        +
 9 a b c d e f g
*/
	__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
	__m128i twoPixels1 = _mm_unpackhi_epi8(src, _mm_setzero_si128());
	
	__m128i left0 = _mm_move_epi64(twoPixels0);
	__m128i left1 = _mm_srli_si128(twoPixels0, 8);
	__m128i result = _mm_add_epi16(twoPixels1, _mm_add_epi16(left0, left1));
	
	return result;
}

// [横4加算, 0]
/*
123456789abcdefg
↓
 1 2 3 4 0 0 0 0
        +
 5 6 7 8 0 0 0 0
        +
 9 a b c 0 0 0 0
        +
 d e f g 0 0 0 0
*/
__forceinline __m128i Collect_4(__m128i src)
{
/*
123456789abcdefg
↓
 1 2 3 4 5 6 7 8
        +
 9 a b c d e f g
*/
	__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
	__m128i twoPixels1 = _mm_unpackhi_epi8(src, _mm_setzero_si128());
	__m128i sum = _mm_add_epi16(twoPixels0, twoPixels1);
	
	__m128i result = _mm_add_epi16(_mm_move_epi64(sum), _mm_srli_si128(sum, 8));
	
	return result;
}

// 8bit * 4 を集計、16bit * 4 で返す
__forceinline __m128i CollectSum_1(const __m128i* pSrc)
{
	__m128i ret = _mm_loadl_epi64(pSrc);
	ret = _mm_unpacklo_epi8(ret, _mm_setzero_si128());
	return ret;
}

__forceinline __m128i CollectSum_2(const __m128i* pSrc)
{
	__m128i m0 = _mm_loadl_epi64(pSrc);
	m0 = _mm_unpacklo_epi8(m0, _mm_setzero_si128());
	__m128i m1 = _mm_srli_si128(m0, 8);
	__m128i ret = _mm_add_epi16(m0, m1);
	return ret;
}

__forceinline __m128i CollectSum_3(const __m128i* pSrc)
{
	return Split_3_1(load_unaligned_128(pSrc));
}

__forceinline __m128i CollectSum_4(const __m128i* pSrc)
{
	__m128i ret = Collect_4(load_unaligned_128(pSrc));
	return ret;
}

__forceinline __m128i CollectSum_4(const __m128i*& pSrc, const uint16_t addCount)
{
	__m128i src = load_unaligned_128(pSrc++);
	__m128i sum = _mm_add_epi16(
		_mm_unpacklo_epi8(src, _mm_setzero_si128()),
		_mm_unpackhi_epi8(src, _mm_setzero_si128())
	);
	for (uint16_t i=1; i<addCount; ++i) {
		src = load_unaligned_128(pSrc++);
		__m128i sum2 = _mm_add_epi16(
			_mm_unpacklo_epi8(src, _mm_setzero_si128()),
			_mm_unpackhi_epi8(src, _mm_setzero_si128())
		);
		sum = _mm_add_epi16(sum, sum2);
	}
	return _mm_add_epi16(_mm_move_epi64(sum), _mm_srli_si128(sum, 8));
}

__forceinline __m128i CollectSum_4_aligned(const __m128i*& pSrc, const uint16_t addCount)
{
	__m128i src = *pSrc++;
	__m128i sum = _mm_add_epi16(
		_mm_unpacklo_epi8(src, _mm_setzero_si128()),
		_mm_unpackhi_epi8(src, _mm_setzero_si128())
	);
	for (uint16_t i=1; i<addCount; ++i) {
		src = *pSrc++;
		__m128i sum2 = _mm_add_epi16(
			_mm_unpacklo_epi8(src, _mm_setzero_si128()),
			_mm_unpackhi_epi8(src, _mm_setzero_si128())
		);
		sum = _mm_add_epi16(sum, sum2);
	}
	return _mm_add_epi16(_mm_move_epi64(sum), _mm_srli_si128(sum, 8));
}

class LineAveragingReducer_RatioAny_Base : public ILineAveragingReducer
{
public:
	uint16_t targetRatio_;
	uint16_t srcRatio_;
	uint16_t srcWidth_;
};

__forceinline __m128i set1_epi16_low(int v)
{
	return _mm_shufflelo_epi16(_mm_cvtsi32_si128(v), 0);
}

// greater than 4
class CollectSum_X
{
public:
	__forceinline static __m128i work(const __m128i* pSrc, const uint16_t count, const uint16_t countBit)
	{
		__m128i ret = CollectSum_4(pSrc, count >> 2);
		switch (count & 0x03) {
		case 0:
			break;
		case 1:
			ret = _mm_adds_epu16(ret, CollectSum_1(pSrc));
			break;
		case 2:
			ret = _mm_adds_epu16(ret, CollectSum_2(pSrc));
			break;
		case 3:
			ret = _mm_adds_epu16(ret, CollectSum_3(pSrc));
			break;
		default:
			__assume(false);
		}
		return ret;
	}
};

class CollectSum_X_1or0
{
public:
	__forceinline static __m128i work(const __m128i* pSrc, const uint16_t count, const uint16_t is1)
	{
		__m128i ret = CollectSum_4(pSrc, count >> 2);
		if (is1) {
			ret = _mm_adds_epu16(ret, CollectSum_1(pSrc));
		}
		return ret;
	}
};

class CollectSum_X_2or1
{
public:
	__forceinline static __m128i work(const __m128i* pSrc, const uint16_t count, const uint16_t is2)
	{
		__m128i ret = CollectSum_4(pSrc, count >> 2);
		if (is2) {
			ret = _mm_adds_epu16(ret, CollectSum_2(pSrc));
		}else {
			ret = _mm_adds_epu16(ret, CollectSum_1(pSrc));
		}
		return ret;
	}
};

class CollectSum_X_3or2
{
public:
	__forceinline static __m128i work(const __m128i* pSrc, const uint16_t count, const uint16_t is3)
	{
		__m128i ret = CollectSum_4(pSrc, count >> 2);
		if (is3) {
			ret = _mm_adds_epu16(ret, CollectSum_3(pSrc));
		}else {
			ret = _mm_adds_epu16(ret, CollectSum_2(pSrc));
		}
		return ret;
	}
};

class CollectSum_X_0or3
{
public:
	__forceinline static __m128i work(const __m128i* pSrc, const uint16_t count, const uint16_t is0)
	{
		__m128i ret = CollectSum_4(pSrc, count >> 2);
		if (is0 == 0) {
			ret = _mm_adds_epu16(ret, CollectSum_3(pSrc));
		}
		return ret;
	}
};

class CollectSum_5or4
{
public:
	__forceinline static __m128i work(const __m128i* pSrc, const uint16_t count, const uint16_t is5)
	{
		__m128i ret = CollectSum_4(pSrc);
		++pSrc;
		if (is5) {
			ret = _mm_adds_epu16(ret, CollectSum_1(pSrc));
		}
		return ret;
	}
};

class CollectSum_4or3
{
public:
	__forceinline static __m128i work(const __m128i* pSrc, const uint16_t count, const uint16_t is4)
	{
		if (is4) {
			return CollectSum_4(pSrc);
		}else {
			return CollectSum_3(pSrc);
		}	
	}
};

class CollectSum_3or2
{
public:
	__forceinline static __m128i work(const __m128i* pSrc, const uint16_t count, const uint16_t is3)
	{
		if (is3) {
			return CollectSum_3(pSrc);
		}else {
			return CollectSum_2(pSrc);
		}	
	}
};

class CollectSum_2or1
{
public:
	__forceinline static __m128i work(const __m128i* pSrc, const uint16_t count, const uint16_t is2)
	{
		if (is2) {
			return CollectSum_2(pSrc);
		}else {
			return CollectSum_1(pSrc);
		}	
	}
};

class CollectSumAndNext1_1or0
{
public:
	__forceinline static void work(const __m128i*& pSrc, const uint8_t base, const uint8_t is1, __m128i& sum, __m128i& one)
	{
		if (is1) {
	//		sum = _mm_cvtepu8_epi16(_mm_loadl_epi64(pSrc));
			sum = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
			one = _mm_srli_si128(sum, 8);
			OffsetPtr(pSrc, 8);
		}else {
			one = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*(const int*)pSrc), _mm_setzero_si128());
			OffsetPtr(pSrc, 4);
		}
	}
};

class CollectSumAndNext1_2or1
{
public:
	__forceinline static void work(const __m128i*& pSrc, const uint8_t base, const uint8_t is2, __m128i& sum, __m128i& one)
	{
		if (is2) {
			__m128i src = load_unaligned_128(pSrc);
			__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
			sum = _mm_add_epi16(twoPixels0, _mm_srli_si128(twoPixels0, 8));
			one = _mm_unpackhi_epi8(src, _mm_setzero_si128());
			OffsetPtr(pSrc, 12);
		}else {
	//		sum = _mm_cvtepu8_epi16(_mm_loadl_epi64(pSrc));
			sum = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
			one = _mm_srli_si128(sum, 8);
			OffsetPtr(pSrc, 8);
		}
	}
};

class CollectSumAndNext1_3or2
{
public:
	__forceinline static void work(const __m128i*& pSrc, const uint8_t base, const uint8_t is3, __m128i& sum, __m128i& one)
	{
		__m128i src = load_unaligned_128(pSrc);
		if (is3) {
			sum = Split_3_1(src);
			one = _mm_srli_si128(sum, 8);
			++pSrc;
		}else {
			__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
			sum = _mm_add_epi16(twoPixels0, _mm_srli_si128(twoPixels0, 8));
			one = _mm_unpackhi_epi8(src, _mm_setzero_si128());
			OffsetPtr(pSrc, 12);
		}
	}
};

class CollectSumAndNext1_4or3
{
public:
	__forceinline static void work(const __m128i*& pSrc, const uint8_t base, const uint8_t is4, __m128i& sum, __m128i& one)
	{
		if (is4) {
			sum = CollectSum_4(pSrc);
			++pSrc;
			one = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*(const int*)pSrc), _mm_setzero_si128());
			OffsetPtr(pSrc, 4);
		}else {
			sum = Split_3_1(load_unaligned_128(pSrc));
			one = _mm_srli_si128(sum, 8);
			++pSrc;
		}
	}
};

class CollectSumAndNext1_5or4
{
public:
	__forceinline static void work(const __m128i*& pSrc, const uint8_t base, const uint8_t is5, __m128i& sum, __m128i& one)
	{
		sum = CollectSum_4(pSrc);
		++pSrc;
		if (is5) {
			__m128i plus = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
			sum = _mm_adds_epu16(sum, plus);
			one = _mm_srli_si128(plus, 8);
			OffsetPtr(pSrc, 8);
		}else {
			one = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*(const int*)pSrc), _mm_setzero_si128());
			OffsetPtr(pSrc, 4);
		}
	}
};

// greater than 4
class CollectSumAndNext1_X
{
public:
	__forceinline static void work(const __m128i*& pSrc, const uint8_t count, const uint16_t bit, __m128i& sum, __m128i& one)
	{
		sum = CollectSum_4(pSrc, count >> 2);
		const uint16_t remain = count & 0x03;
		switch (remain) {
		case 0:
			one = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*(const int*)pSrc), _mm_setzero_si128());
			break;
		case 1:
			{
				__m128i plus = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
				sum = _mm_adds_epu16(sum, plus);
				one = _mm_srli_si128(plus, 8);
			}
			break;
		case 2:
			{
				__m128i src = load_unaligned_128(pSrc);
				__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
				__m128i plus = _mm_add_epi16(twoPixels0, _mm_srli_si128(twoPixels0, 8));
				sum = _mm_adds_epu16(sum, plus);
				one = _mm_unpackhi_epi8(src, _mm_setzero_si128());
			}
			break;
		case 3:
			{
				__m128i plus = Split_3_1(load_unaligned_128(pSrc));
				sum = _mm_adds_epu16(sum, plus);
				one = _mm_srli_si128(plus, 8);
			}
			break;
		default:
			__assume(false);
		}
		OffsetPtr(pSrc, (remain+1)*4);
	}
};

// greater than 4
class CollectSumAndNext1_X_1or0
{
public:
	__forceinline static void work(const __m128i*& pSrc, const uint8_t count, const uint16_t is1, __m128i& sum, __m128i& one)
	{
		sum = CollectSum_4(pSrc, count >> 2);
		const uint16_t remain = count & 0x03;
		if (is1) {
			__m128i plus = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
			sum = _mm_adds_epu16(sum, plus);
			one = _mm_srli_si128(plus, 8);
			OffsetPtr(pSrc, 8);
		}else {
			one = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*(const int*)pSrc), _mm_setzero_si128());
			OffsetPtr(pSrc, 4);
		}
	}
};

// greater than 4
class CollectSumAndNext1_X_2or1
{
public:
	__forceinline static void work(const __m128i*& pSrc, const uint8_t count, const uint16_t is2, __m128i& sum, __m128i& one)
	{
		sum = CollectSum_4(pSrc, count >> 2);
		if (is2) {
			__m128i src = load_unaligned_128(pSrc);
			__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
			__m128i plus = _mm_add_epi16(twoPixels0, _mm_srli_si128(twoPixels0, 8));
			sum = _mm_adds_epu16(sum, plus);
			one = _mm_unpackhi_epi8(src, _mm_setzero_si128());
			OffsetPtr(pSrc, 12);
		}else {
			__m128i plus = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
			sum = _mm_adds_epu16(sum, plus);
			one = _mm_srli_si128(plus, 8);
			OffsetPtr(pSrc, 8);
		}
	}
};

// greater than 4
class CollectSumAndNext1_X_3or2
{
public:
	__forceinline static void work(const __m128i*& pSrc, const uint8_t count, const uint16_t is3, __m128i& sum, __m128i& one)
	{
		sum = CollectSum_4(pSrc, count >> 2);
		if (is3) {
			__m128i plus = Split_3_1(load_unaligned_128(pSrc));
			sum = _mm_adds_epu16(sum, plus);
			one = _mm_srli_si128(plus, 8);
			++pSrc;
		}else {
			__m128i src = load_unaligned_128(pSrc);
			__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
			__m128i plus = _mm_add_epi16(twoPixels0, _mm_srli_si128(twoPixels0, 8));
			sum = _mm_adds_epu16(sum, plus);
			one = _mm_unpackhi_epi8(src, _mm_setzero_si128());
			OffsetPtr(pSrc, 12);
		}
	}
};

// greater than 4
class CollectSumAndNext1_X_0or3
{
public:
	__forceinline static void work(const __m128i*& pSrc, const uint8_t count, const uint16_t is0, __m128i& sum, __m128i& one)
	{
		sum = CollectSum_4(pSrc, count >> 2);
		if (is0) {
			one = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*(const int*)pSrc), _mm_setzero_si128());
			OffsetPtr(pSrc, 4);
		}else {
			__m128i plus = Split_3_1(load_unaligned_128(pSrc));
			sum = _mm_adds_epu16(sum, plus);
			one = _mm_srli_si128(plus, 8);
			++pSrc;
		}
	}
};

template <typename BodyCollectSumAndNext1T, typename HeadCollectSumAndNext1T, typename TailCollectSumT>
class LineAveragingReducer_RatioAny_Basic : public LineAveragingReducer_RatioAny_Base
{
private:
	const uint32_t* pBodyCountBits_;
	const uint16_t* pTailTargetRatios_;

public:
	ILineAveragingReducer_Impl;
	uint8_t maxBodyCount_;

	LineAveragingReducer_RatioAny_Basic(const uint32_t* pBodyCountBits, const uint16_t* pTailTargetRatios)
		:
		pBodyCountBits_(pBodyCountBits),
		pTailTargetRatios_(pTailTargetRatios)
	{
	}
	
	template <typename T>
	void iterate(const __m128i* srcBuff, __m128i* tmpBuff)
	{
		const __m128i* pSrc = srcBuff;
		__m128i* pTmp = tmpBuff;
		
		const uint16_t srcRatio = srcRatio_;
		const uint16_t targetRatio = targetRatio_;
		const uint16_t srcWidth = srcWidth_;
		const uint16_t endLoopCnt = srcRatio / targetRatio;
		const uint16_t remainderRatio = srcRatio % targetRatio;
		const __m128i remainderDividedByTargetRatio = make_vec_ratios(remainderRatio << 16, targetRatio);

		const uint16_t blockCount = srcWidth / srcRatio;
		for (uint16_t blockIdx=0; blockIdx<blockCount; ++blockIdx) {
			__m128i col2;
			// head
			{
				__m128i col;
				HeadCollectSumAndNext1T::work(pSrc, endLoopCnt, endLoopCnt - maxBodyCount_, col, col2);
				__m128i tmpBase = _mm_loadl_epi64(pTmp);
				// srcRatio data straddles targetRatio's tail and next head
				__m128i col2a = _mm_mulhi_epu16(col2, remainderDividedByTargetRatio);
				col = _mm_add_epi16(col, col2a);
				col2 = _mm_sub_epi16(col2, col2a);
				_mm_storel_epi64(pTmp, T::work(tmpBase, col));
				OffsetPtr(pTmp, 8);
			}
			// body
			{
				const uint16_t bodyLoopCount = targetRatio - 2;
				const uint16_t bodyLoopCountQuotient = bodyLoopCount / 32;
				const uint16_t bodyLoopCountRemainder = bodyLoopCount % 32;
				const uint32_t* pWorkBodyCountBits = pBodyCountBits_;
				const uint16_t* pWorkTailTargetRatios = pTailTargetRatios_;
				const uint16_t bodyCountBase = maxBodyCount_ - 1;
				for (uint16_t i=0; i<bodyLoopCountQuotient; ++i) {
					uint32_t bodyCountBits = *pWorkBodyCountBits++;
#if 1
					// loop unrolled version
					for (uint16_t j=0; j<32/4; ++j) {
						__m128i tailTargetRatios = _mm_loadl_epi64((const __m128i*)pWorkTailTargetRatios);
						OffsetPtr(pWorkTailTargetRatios, 8);
						__m128i collected;
						__m128i tailTargetRatio;
						__m128i col = col2;
						__m128i col2a;

						/// 1
						BodyCollectSumAndNext1T::work(pSrc, bodyCountBase+(bodyCountBits & 1), bodyCountBits & 1, collected, col2);
						bodyCountBits >>= 1;
						__m128i tmpBase = load_unaligned_128(pTmp);
						tailTargetRatio = _mm_shufflelo_epi16(tailTargetRatios, _MM_SHUFFLE(0,0,0,0));
						col = _mm_add_epi16(col, collected);
						col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
						col = _mm_add_epi16(col, col2a);
						_mm_storel_epi64(pTmp, T::work(tmpBase, col));
						OffsetPtr(pTmp, 8);
						col = _mm_sub_epi16(col2, col2a);
						/// 2
						tailTargetRatio = _mm_shufflelo_epi16(tailTargetRatios, _MM_SHUFFLE(1,1,1,1));
						BodyCollectSumAndNext1T::work(pSrc, bodyCountBase+(bodyCountBits & 1), bodyCountBits & 1, collected, col2);
						bodyCountBits >>= 1;
						col = _mm_add_epi16(col, collected);
						col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
						col = _mm_add_epi16(col, col2a);
						_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), col));
						OffsetPtr(pTmp, 8);
						col = _mm_sub_epi16(col2, col2a);
						/// 3
						BodyCollectSumAndNext1T::work(pSrc, bodyCountBase+(bodyCountBits & 1), bodyCountBits & 1, collected, col2);
						bodyCountBits >>= 1;
						tmpBase = load_unaligned_128(pTmp);
						tailTargetRatio = _mm_shufflelo_epi16(tailTargetRatios, _MM_SHUFFLE(2,2,2,2));
						col = _mm_add_epi16(col, collected);
						col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
						col = _mm_add_epi16(col, col2a);
						_mm_storel_epi64(pTmp, T::work(tmpBase, col));
						OffsetPtr(pTmp, 8);
						col = _mm_sub_epi16(col2, col2a);
						/// 4
						tailTargetRatio = _mm_shufflelo_epi16(tailTargetRatios, _MM_SHUFFLE(3,3,3,3));
						BodyCollectSumAndNext1T::work(pSrc, bodyCountBase+(bodyCountBits & 1), bodyCountBits & 1, collected, col2);
						bodyCountBits >>= 1;
						col = _mm_add_epi16(col, collected);
						col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
						col = _mm_add_epi16(col, col2a);
						_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), col));
						OffsetPtr(pTmp, 8);
						col2 = _mm_sub_epi16(col2, col2a);

					}
#else
					for (uint16_t j=0; j<32; ++j) {
						__m128i collected;
						__m128i col = col2; // head
						BodyCollectSumAndNext1T::work(pSrc, bodyCountBase, bodyCountBits & 1, collected, col2);
						bodyCountBits >>= 1;
						col = _mm_add_epi16(col, collected);
						__m128i tailTargetRatio = set1_epi16_low(*pWorkTailTargetRatios++);
						__m128i col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
						col = _mm_add_epi16(col, col2a);
						_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
						OffsetPtr(pTmp, 8);
						col2 = _mm_sub_epi16(col2, col2a);
					}
#endif
				}
				{
					uint32_t bodyCountBits = *pWorkBodyCountBits;
					for (uint16_t i=0; i<bodyLoopCountRemainder; ++i) {
						__m128i collected;
						__m128i col = col2; // head
						BodyCollectSumAndNext1T::work(pSrc, bodyCountBase+(bodyCountBits & 1), bodyCountBits & 1, collected, col2);
						__m128i tmpBase = _mm_loadl_epi64(pTmp);
						bodyCountBits >>= 1;
						col = _mm_add_epi16(col, collected);
						__m128i tailTargetRatio = set1_epi16_low(*pWorkTailTargetRatios++);
						__m128i col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
						col = _mm_add_epi16(col, col2a);
						_mm_storel_epi64(pTmp, T::work(tmpBase, col));
						OffsetPtr(pTmp, 8);
						col2 = _mm_sub_epi16(col2, col2a);
					}
				}
			}
			// tail
			{
				__m128i col = col2;
				col = _mm_add_epi16(col, TailCollectSumT::work(pSrc, endLoopCnt, endLoopCnt - maxBodyCount_));
				OffsetPtr(pSrc, endLoopCnt * 4);
				_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
				OffsetPtr(pTmp, 8);
			}
		}
	}

};

class LineAveragingReducer_RatioAny_MoreThanHalf : public LineAveragingReducer_RatioAny_Base
{
private:
	const uint8_t* pBodyCounts_;
	const uint16_t* pTailTargetRatios_;

public:
	ILineAveragingReducer_Impl;

	LineAveragingReducer_RatioAny_MoreThanHalf(const uint8_t* pBodyCounts, const uint16_t* pTailTargetRatios)
		:
		pBodyCounts_(pBodyCounts),
		pTailTargetRatios_(pTailTargetRatios)
	{
	}
	
	template <typename T>
	void iterate(const __m128i* srcBuff, __m128i* tmpBuff)
	{
		const __m128i* pSrc = srcBuff;
		__m128i* pTmp = tmpBuff;

		const uint16_t srcRatio = srcRatio_;
		const uint16_t targetRatio = targetRatio_;
		const uint16_t srcWidth = srcWidth_;
		const uint16_t remainderRatio = srcRatio - targetRatio;
		const __m128i remainderDividedByTargetRatio = make_vec_ratios(remainderRatio << 16, targetRatio);

		const uint16_t blockCount = srcWidth/srcRatio;
		for (uint16_t blockIdx=0; blockIdx<blockCount; ++blockIdx) {
			__m128i col2;
			// head
			{
				__m128i col;
				CollectSumAndNext1_2or1::work(pSrc, 1, 0, col, col2);
				__m128i tmpBase = _mm_loadl_epi64(pTmp);
				// srcRatio data straddles targetRatio's tail and next head
				__m128i col2a = _mm_mulhi_epu16(col2, remainderDividedByTargetRatio);
				col = _mm_add_epi16(col, col2a);
				col2 = _mm_sub_epi16(col2, col2a);
				_mm_storel_epi64(pTmp, T::work(tmpBase, col));
				OffsetPtr(pTmp, 8);
			}
			// body
			{
				__m128i src = load_unaligned_128(pSrc);
//				_mm_prefetch((const char*)(pSrc+10), _MM_HINT_NTA);
				const uint16_t limit = targetRatio - 2;
				const uint16_t loopCount = limit >> 1;
				const uint16_t* pIntBodyCounts = (const uint16_t*) pBodyCounts_;
				const uint32_t* pIntTailTargetRatios = (const uint32_t*) pTailTargetRatios_;
				for (uint16_t i=0; i<loopCount; ++i) {
					__m128i tmpBase = load_unaligned_128(pTmp);
					const __m128i ratiosOrg = _mm_cvtsi32_si128(*pIntTailTargetRatios++);
					const __m128i ratiosTmp = _mm_shufflelo_epi16(ratiosOrg, _MM_SHUFFLE(1,1,0,0));
					const __m128i ratios = _mm_shuffle_epi32(ratiosTmp, _MM_SHUFFLE(1,1,0,0));
					const uint16_t bodyCountPair = *pIntBodyCounts++;
					switch (bodyCountPair) {
					case 0:
						{
							OffsetPtr(pSrc, 8);
							__m128i nextHalf = _mm_loadl_epi64((const __m128i*)((char*)pSrc+8));
							__m128i wrk = _mm_unpacklo_epi8(src, _mm_setzero_si128());
							__m128i multiplied = _mm_mulhi_epu16(wrk, ratios);
							__m128i subtracted = _mm_sub_epi16(wrk, multiplied);
							__m128i colFirst = _mm_add_epi16(col2, multiplied);
							_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
							OffsetPtr(pTmp, 8);
							__m128i colSecond = _mm_add_epi16(subtracted, _mm_srli_si128(multiplied, 8));
							_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
							OffsetPtr(pTmp, 8);
							col2 = _mm_srli_si128(subtracted, 8);
							src = _mm_unpacklo_epi64(_mm_srli_si128(src, 8), nextHalf);
						}
						break;
					case 1:
						{
							OffsetPtr(pSrc, 12);
							__m128i collected = _mm_unpacklo_epi8(src, _mm_setzero_si128());
							__m128i remain = _mm_unpacklo_epi8(_mm_srli_si128(src, 4), _mm_setzero_si128());
							src = load_unaligned_128(pSrc);
							__m128i multiplied = _mm_mulhi_epu16(remain, ratios);	// col2 * tail/target
							__m128i subtracted = _mm_sub_epi16(remain, multiplied);
							__m128i col = _mm_add_epi16(col2, collected);
							__m128i colFirst = _mm_add_epi16(col, multiplied);
							_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
							OffsetPtr(pTmp, 8);
							__m128i colSecond = _mm_add_epi16(subtracted, _mm_srli_si128(multiplied,8));
							_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
							OffsetPtr(pTmp, 8);
							col2 = _mm_srli_si128(subtracted,8);
						}
						break;
					case 0x100:
						{
							OffsetPtr(pSrc, 12);
							__m128i shuffled = _mm_shuffle_epi32(src, _MM_SHUFFLE(2,0,1,1));
							src = load_unaligned_128(pSrc);
							__m128i collecteds = _mm_unpacklo_epi8(shuffled, _mm_setzero_si128());
							__m128i nexts = _mm_unpackhi_epi8(shuffled, _mm_setzero_si128());
							__m128i multiplied = _mm_mulhi_epu16(nexts, ratios);	// col2 * tail/target
							__m128i subtracted = _mm_sub_epi16(nexts, multiplied);
							__m128i colFirst = _mm_add_epi16(col2, multiplied);
							_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
							OffsetPtr(pTmp, 8);
							__m128i sums = _mm_add_epi16(collecteds, multiplied);
							__m128i colSecond = _mm_add_epi16(subtracted, _mm_srli_si128(sums, 8));
							_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
							OffsetPtr(pTmp, 8);
							col2 = _mm_srli_si128(subtracted, 8);
						}
						break;
					case 0x101:
						{
							++pSrc;
							__m128i shuffled = _mm_shuffle_epi32(src, _MM_SHUFFLE(3,1,2,0));
							src = load_unaligned_128(pSrc);
							__m128i collecteds = _mm_unpacklo_epi8(shuffled, _mm_setzero_si128());
							__m128i nexts = _mm_unpackhi_epi8(shuffled, _mm_setzero_si128());
							__m128i multiplied = _mm_mulhi_epu16(nexts, ratios);	// col2 * tail/target
							__m128i subtracted = _mm_sub_epi16(nexts, multiplied);
							__m128i sums = _mm_add_epi16(collecteds, multiplied);
							__m128i colFirst = _mm_add_epi16(col2, sums);
							_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
							OffsetPtr(pTmp, 8);
							__m128i colSecond = _mm_add_epi16(subtracted, _mm_srli_si128(sums, 8));
							_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
							OffsetPtr(pTmp, 8);
							col2 = _mm_srli_si128(subtracted, 8);
						}
						break;
					default:
						__assume(false);
					}
				}
				if (limit & 1) {
					const uint8_t bodyCount = pBodyCounts_[limit-1];
					__m128i col = col2; // head
					col2 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
					if (bodyCount) {
						col = _mm_add_epi16(col, col2);
						col2 = _mm_srli_si128(col2, 8);
					}
					OffsetPtr(pSrc, (1+bodyCount)*4);
					__m128i tailTargetRatio = set1_epi16_low(pTailTargetRatios_[limit-1]);
					__m128i col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
					col2 = _mm_sub_epi16(col2, col2a);
					col = _mm_add_epi16(col, col2a);
					_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
					OffsetPtr(pTmp, 8);
				}
			}
			// tail
			{
				__m128i col = col2;
				col = _mm_add_epi16(col, CollectSum_1(pSrc));
				OffsetPtr(pSrc, 4);
				_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
				OffsetPtr(pTmp, 8);
			}
		}
	}
};

class LineAveragingReducer_RatioAny_MoreThanHalf_01_001 : public LineAveragingReducer_RatioAny_Base
{
private:
	const uint32_t* pBodyCountPatternBits_;
	const uint16_t* pTailTargetRatios_;

public:
	uint16_t bodyCountPatternBitsCount_;

	ILineAveragingReducer_Impl;

	LineAveragingReducer_RatioAny_MoreThanHalf_01_001(const uint32_t* pBodyCountPatternBits, const uint16_t* pTailTargetRatios)
		:
		pBodyCountPatternBits_(pBodyCountPatternBits),
		pTailTargetRatios_(pTailTargetRatios)
	{
	}
	
	template <typename T>
	void iterate(const __m128i* srcBuff, __m128i* tmpBuff)
	{
		const __m128i* pSrc = srcBuff;
		__m128i* pTmp = tmpBuff;

		const uint16_t srcRatio = srcRatio_;
		const uint16_t targetRatio = targetRatio_;
		const uint16_t srcWidth = srcWidth_;
		const uint16_t remainderRatio = srcRatio - targetRatio;
		const __m128i remainderDividedByTargetRatio = make_vec_ratios(remainderRatio << 16, targetRatio);

		const uint16_t blockCount = srcWidth/srcRatio;
		for (uint16_t blockIdx=0; blockIdx<blockCount; ++blockIdx) {
			__m128i col2;
			// head
			{
				__m128i col;
				CollectSumAndNext1_2or1::work(pSrc, 1, 0, col, col2);
				// srcRatio data straddles targetRatio's tail and next head
				__m128i col2a = _mm_mulhi_epu16(col2, remainderDividedByTargetRatio);
				col = _mm_add_epi16(col, col2a);
				col2 = _mm_sub_epi16(col2, col2a);
				_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
				OffsetPtr(pTmp, 8);
			}
			// body
			{
				const uint32_t* pWorkBits = pBodyCountPatternBits_;
				const uint16_t* pWorkTailTargetRatios = pTailTargetRatios_;

				const uint16_t bodyLoopCount = bodyCountPatternBitsCount_ - 1;
				const uint16_t bodyLoopCountQuotient = bodyLoopCount / 32;
				const uint16_t bodyLoopCountRemainder = bodyLoopCount % 32;
				const uint16_t remainLoopCount = (bodyLoopCountRemainder > 1) ? 1 : 0;
				const uint16_t patternLoopCount = bodyLoopCountQuotient + remainLoopCount;
				for (uint16_t i=0; i<patternLoopCount; ++i) {
					uint32_t bits = *pWorkBits++;
					uint16_t innerLoopCount = 32/2;
					if (i == patternLoopCount-1 && remainLoopCount) {
						innerLoopCount = bodyLoopCountRemainder / 2;
					}
					for (uint16_t j=0; j<innerLoopCount; ++j) {
						const uint16_t pattern = bits & 0x3;
						bits >>= 2;

						__m128i abcd = load_unaligned_128(pSrc++);
						__m128i tmpBase = load_unaligned_128(pTmp);
						__m128i ratiosSrc = _mm_loadl_epi64((const __m128i*)pWorkTailTargetRatios);
						OffsetPtr(pWorkTailTargetRatios, 8);
						ratiosSrc = _mm_unpacklo_epi16(ratiosSrc, ratiosSrc);
						const __m128i ratios01 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(1,1,0,0));

						switch (pattern) {
						case 0: // 01 01
							{
								__m128i ef = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128()); // preload
								OffsetPtr(pSrc, 8);

								__m128i acbd = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(3,1,2,0));
								__m128i ac = _mm_unpacklo_epi8(acbd, _mm_setzero_si128());
								__m128i bd = _mm_unpackhi_epi8(acbd, _mm_setzero_si128());
								__m128i ac_multiplied = _mm_mulhi_epu16(ac, ratios01);
								__m128i ac_subtracted = _mm_sub_epi16(ac, ac_multiplied);
								
								__m128i colFirst = _mm_add_epi16(col2, ac_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
								OffsetPtr(pTmp, 8);
								
								__m128i colSecond = _mm_add_epi16(_mm_add_epi16(ac_subtracted, bd), _mm_srli_si128(ac_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
								OffsetPtr(pTmp, 8);
								tmpBase = load_unaligned_128(pTmp);

								const __m128i ratios23 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(3,3,2,2));
								__m128i df = _mm_unpackhi_epi64(bd, ef);
								__m128i df_multiplied = _mm_mulhi_epu16(df, ratios23);
								__m128i df_subtracted = _mm_sub_epi16(df, df_multiplied);
								
								__m128i colThird = _mm_add_epi16(_mm_srli_si128(ac_subtracted, 8), df_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
								OffsetPtr(pTmp, 8);
								
								__m128i colFourth = _mm_add_epi16(_mm_add_epi16(df_subtracted, ef), _mm_srli_si128(df_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colFourth));
								OffsetPtr(pTmp, 8);
								
								col2 = _mm_srli_si128(df_subtracted, 8);
							}
							break;
						case 1: // 001 01
							{
								__m128i efgh = load_unaligned_128(pSrc);	// preload
								OffsetPtr(pSrc, 12);

								__m128i ab = _mm_unpacklo_epi8(abcd, _mm_setzero_si128());
								__m128i cd = _mm_unpackhi_epi8(abcd, _mm_setzero_si128());
								__m128i ab_multiplied = _mm_mulhi_epu16(ab, ratios01);
								__m128i ab_subtracted = _mm_sub_epi16(ab, ab_multiplied);
								
								__m128i colFirst = _mm_add_epi16(col2, ab_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
								OffsetPtr(pTmp, 8);
								
								__m128i colSecond = _mm_add_epi16(ab_subtracted, _mm_srli_si128(ab_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
								OffsetPtr(pTmp, 8);
								tmpBase = load_unaligned_128(pTmp);
								
								// target is d only
								const __m128i ratios32 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(2,2,3,3));
								__m128i cd_multiplied = _mm_mulhi_epu16(cd, ratios32);
								__m128i cd_subtracted = _mm_sub_epi16(cd, cd_multiplied);
								
								__m128i colThird = _mm_add_epi16(_mm_srli_si128(_mm_add_epi16(ab_subtracted, cd_multiplied), 8), cd);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
								OffsetPtr(pTmp, 8);
								
								const __m128i ratios4 = set1_epi16_low(*pWorkTailTargetRatios++);
								__m128i ratio34 = _mm_unpacklo_epi64(ratios32, ratios4);
								__m128i egfh = _mm_shuffle_epi32(efgh, _MM_SHUFFLE(3,1,2,0));
								__m128i eg = _mm_unpacklo_epi8(egfh, _mm_setzero_si128());
								__m128i fh = _mm_unpackhi_epi8(egfh, _mm_setzero_si128());
								__m128i eg_multiplied = _mm_mulhi_epu16(eg, ratio34);
								__m128i eg_subtracted = _mm_sub_epi16(eg, eg_multiplied);
								
								__m128i colFourth = _mm_add_epi16(_mm_srli_si128(cd_subtracted, 8), eg_multiplied);
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colFourth));
								OffsetPtr(pTmp, 8);
								tmpBase = _mm_loadl_epi64(pTmp);
								
								__m128i colFifth = _mm_add_epi16(_mm_add_epi16(eg_subtracted, fh), _mm_srli_si128(eg_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFifth));
								OffsetPtr(pTmp, 8);
								
								col2 = _mm_srli_si128(eg_subtracted, 8);
							}
							break;
						case 2: // 01 001
							{
								__m128i efgh = load_unaligned_128(pSrc);	// preload
								OffsetPtr(pSrc, 12);
								
								__m128i acbd = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(3,1,2,0));
								__m128i ac = _mm_unpacklo_epi8(acbd, _mm_setzero_si128());
								__m128i bd = _mm_unpackhi_epi8(acbd, _mm_setzero_si128());
								__m128i ac_multiplied = _mm_mulhi_epu16(ac, ratios01);
								__m128i ac_subtracted = _mm_sub_epi16(ac, ac_multiplied);
								
								__m128i colFirst = _mm_add_epi16(col2, ac_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
								OffsetPtr(pTmp, 8);
								
								__m128i colSecond = _mm_add_epi16(_mm_add_epi16(ac_subtracted, bd), _mm_srli_si128(ac_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
								OffsetPtr(pTmp, 8);
								tmpBase = load_unaligned_128(pTmp);
								
								// target is d only
								__m128i ratios32 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(2,2,3,3));
								__m128i bd_multiplied = _mm_mulhi_epu16(bd, ratios32);
								__m128i bd_subtracted = _mm_sub_epi16(bd, bd_multiplied);

								__m128i colThird = _mm_srli_si128(_mm_add_epi16(ac_subtracted, bd_multiplied), 8);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
								OffsetPtr(pTmp, 8);
								
								__m128i egfh = _mm_shuffle_epi32(efgh, _MM_SHUFFLE(3,1,2,0));
								__m128i eg = _mm_unpacklo_epi8(egfh, _mm_setzero_si128());

								__m128i ratios4 = set1_epi16_low(*pWorkTailTargetRatios++);
								__m128i ratios34 = _mm_unpacklo_epi64(ratios32, ratios4);
								__m128i eg_multiplied = _mm_mulhi_epu16(eg, ratios34);
								__m128i eg_subtracted = _mm_sub_epi16(eg, eg_multiplied);
								col2 = _mm_srli_si128(eg_subtracted, 8);
								
								__m128i colFourth = _mm_add_epi16(_mm_srli_si128(bd_subtracted, 8), eg_multiplied);
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colFourth));
								OffsetPtr(pTmp, 8);
								
								tmpBase = _mm_loadl_epi64(pTmp);
								__m128i fh = _mm_unpackhi_epi8(egfh, _mm_setzero_si128());
								__m128i colFifth = _mm_add_epi16(eg_subtracted, _mm_add_epi16(fh, _mm_srli_si128(eg_multiplied, 8)));
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFifth));
								OffsetPtr(pTmp, 8);
								
							}
							break;
						case 3: // 001 001
							{
								__m128i efgh = load_unaligned_128(pSrc++); // preload

								__m128i ab = _mm_unpacklo_epi8(abcd, _mm_setzero_si128());
								__m128i cd = _mm_unpackhi_epi8(abcd, _mm_setzero_si128());
								__m128i ab_multiplied = _mm_mulhi_epu16(ab, ratios01);
								__m128i ab_subtracted = _mm_sub_epi16(ab, ab_multiplied);
								
								__m128i colFirst = _mm_add_epi16(col2, ab_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
								OffsetPtr(pTmp, 8);
								
								__m128i colSecond = _mm_add_epi16(ab_subtracted, _mm_srli_si128(ab_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
								OffsetPtr(pTmp, 8);
								tmpBase = load_unaligned_128(pTmp);
								
								__m128i gefh = _mm_shuffle_epi32(efgh, _MM_SHUFFLE(3,1,0,2));
								__m128i ge = _mm_unpacklo_epi8(gefh, _mm_setzero_si128());
								__m128i fh = _mm_unpackhi_epi8(gefh, _mm_setzero_si128());
								
								__m128i de = _mm_unpackhi_epi64(cd, ge);
								const __m128i ratios23 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(3,3,2,2));
								__m128i de_multiplied = _mm_mulhi_epu16(de, ratios23);
								__m128i de_subtracted = _mm_sub_epi16(de, de_multiplied);

								__m128i colThird = _mm_add_epi16(_mm_srli_si128(ab_subtracted, 8), _mm_add_epi16(cd, de_multiplied));
								_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
								OffsetPtr(pTmp, 8);
								
								__m128i colForth = _mm_add_epi16(de_subtracted, _mm_srli_si128(de_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colForth));
								OffsetPtr(pTmp, 8);
								tmpBase = load_unaligned_128(pTmp);
								
								const __m128i ratiosOrg = _mm_cvtsi32_si128(*(const int*)pWorkTailTargetRatios);
								OffsetPtr(pWorkTailTargetRatios, 4);
								const __m128i ratiosTmp = _mm_shufflelo_epi16(ratiosOrg, _MM_SHUFFLE(1,1,0,0));
								const __m128i ratios45 = _mm_shuffle_epi32(ratiosTmp, _MM_SHUFFLE(1,1,0,0));
								__m128i fh_multiplied = _mm_mulhi_epu16(fh, ratios45);
								__m128i fh_subtracted = _mm_sub_epi16(fh, fh_multiplied);
								
								__m128i colFifth = _mm_add_epi16(_mm_srli_si128(de_subtracted, 8), fh_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFifth));
								OffsetPtr(pTmp, 8);
								
								__m128i colSixth = _mm_add_epi16(_mm_add_epi16(fh_subtracted, ge), _mm_srli_si128(fh_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSixth));
								OffsetPtr(pTmp, 8);
								
								col2 = _mm_srli_si128(fh_subtracted, 8);
							}
							break;
						default:
							__assume(false);
						}
					}
				}
				if (bodyLoopCountRemainder & 1) {
					__m128i ratiosSrc = _mm_loadl_epi64((const __m128i*)pWorkTailTargetRatios);
					ratiosSrc = _mm_unpacklo_epi16(ratiosSrc, ratiosSrc);
					const __m128i ratios01 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(1,1,0,0));
					__m128i abcd = load_unaligned_128(pSrc++);
					__m128i tmpBase = load_unaligned_128(pTmp);
					if (*pBodyCountPatternBits_ & 2) { // 0010
						__m128i abdc = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(2,3,1,0));
						__m128i ab = _mm_unpacklo_epi8(abdc, _mm_setzero_si128());
						__m128i dc = _mm_unpackhi_epi8(abdc, _mm_setzero_si128());
						__m128i ab_multiplied = _mm_mulhi_epu16(ab, ratios01);
						__m128i ab_subtracted = _mm_sub_epi16(ab, ab_multiplied);

						__m128i colFirst = _mm_add_epi16(col2, ab_multiplied);
						_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
						OffsetPtr(pTmp, 8);
						
						__m128i colSecond = _mm_add_epi16(ab_subtracted, _mm_srli_si128(ab_multiplied, 8));
						_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
						OffsetPtr(pTmp, 8);
						tmpBase = load_unaligned_128(pTmp);
						
						__m128i ef = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
						OffsetPtr(pSrc, 4);
						const __m128i ratios23 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(3,3,2,2));
						__m128i colThird = _mm_srli_si128(_mm_add_epi16(ab_subtracted, dc), 8);
						
						__m128i de = _mm_unpacklo_epi64(dc, ef);
						__m128i de_multiplied = _mm_mulhi_epu16(de, ratios23);
						colThird = _mm_add_epi16(colThird, de_multiplied);
						_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
						OffsetPtr(pTmp, 8);
						
						__m128i de_subtracted = _mm_sub_epi16(de, de_multiplied);

						__m128i colFourth = _mm_add_epi16(de_subtracted, _mm_srli_si128(de_multiplied, 8));
						_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colFourth));
						OffsetPtr(pTmp, 8);

						col2 = _mm_srli_si128(de_subtracted, 8);
						
					}else { // 010
						__m128i acdb = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(1,3,2,0));
						__m128i ac = _mm_unpacklo_epi8(acdb, _mm_setzero_si128());
						__m128i db = _mm_unpackhi_epi8(acdb, _mm_setzero_si128());
						__m128i ac_multiplied = _mm_mulhi_epu16(ac, ratios01);
						__m128i ac_subtracted = _mm_sub_epi16(ac, ac_multiplied);
						
						__m128i colFirst = _mm_add_epi16(col2, ac_multiplied);
						_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
						OffsetPtr(pTmp, 8);
						
						const __m128i ratios23 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(3,3,2,2));
						__m128i db_multiplied = _mm_mulhi_epu16(db, ratios23);
						__m128i db_subtracted = _mm_sub_epi16(db, db_multiplied);
						
						__m128i colSecond = _mm_add_epi16(ac_subtracted, _mm_srli_si128(_mm_add_epi16(db, ac_multiplied), 8));
						_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
						OffsetPtr(pTmp, 8);
						tmpBase = _mm_loadl_epi64(pTmp);
						
						__m128i colThird = _mm_add_epi16(_mm_srli_si128(ac_subtracted, 8), db_multiplied);
						_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
						OffsetPtr(pTmp, 8);
						
						col2 = db_subtracted;
					}
				}else { // 0
					__m128i tmpBase = _mm_loadl_epi64(pTmp);
					__m128i ab = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
					OffsetPtr(pSrc, 4);
					__m128i ratios = set1_epi16_low(*pWorkTailTargetRatios);
					
					__m128i a_multiplied = _mm_mulhi_epu16(ab, ratios);
					__m128i a_subtracted = _mm_sub_epi16(ab, a_multiplied);

					__m128i colFirst = _mm_add_epi16(col2, a_multiplied);
					_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
					OffsetPtr(pTmp, 8);
					
					col2 = a_subtracted;
				}
			}
			// tail
			{
				__m128i col = col2;
				col = _mm_add_epi16(col, CollectSum_1(pSrc));
				OffsetPtr(pSrc, 4);
				_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
				OffsetPtr(pTmp, 8);
			}
		}
	}
};

class LineAveragingReducer_RatioAny_MoreThanHalf_10_110 : public LineAveragingReducer_RatioAny_Base
{
private:
	const uint32_t* pBodyCountPatternBits_;
	const uint16_t* pTailTargetRatios_;

public:
	uint16_t bodyCountPatternBitsCount_;

	ILineAveragingReducer_Impl;

	LineAveragingReducer_RatioAny_MoreThanHalf_10_110(const uint32_t* pBodyCountPatternBits, const uint16_t* pTailTargetRatios)
		:
		pBodyCountPatternBits_(pBodyCountPatternBits),
		pTailTargetRatios_(pTailTargetRatios)
	{
	}
	
	template <typename T>
	void iterate(const __m128i* srcBuff, __m128i* tmpBuff)
	{
		const __m128i* pSrc = srcBuff;
		__m128i* pTmp = tmpBuff;

		const uint16_t srcRatio = srcRatio_;
		const uint16_t targetRatio = targetRatio_;
		const uint16_t srcWidth = srcWidth_;
		const uint16_t remainderRatio = srcRatio - targetRatio;
		const __m128i remainderDividedByTargetRatio = make_vec_ratios(remainderRatio << 16, targetRatio);

		const uint16_t blockCount = srcWidth/srcRatio;
		for (uint16_t blockIdx=0; blockIdx<blockCount; ++blockIdx) {
			__m128i col2;
			// head
			{
				__m128i col;
				CollectSumAndNext1_2or1::work(pSrc, 1, 0, col, col2);
				// srcRatio data straddles targetRatio's tail and next head
				__m128i col2a = _mm_mulhi_epu16(col2, remainderDividedByTargetRatio);
				col = _mm_add_epi16(col, col2a);
				col2 = _mm_sub_epi16(col2, col2a);
				_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
				OffsetPtr(pTmp, 8);
			}
			// body
			{
				const uint32_t* pWorkBits = pBodyCountPatternBits_;
				const uint16_t* pWorkTailTargetRatios = pTailTargetRatios_;

				const uint16_t bodyLoopCount = bodyCountPatternBitsCount_ - 1;
				const uint16_t bodyLoopCountQuotient = bodyLoopCount / 32;
				const uint16_t bodyLoopCountRemainder = bodyLoopCount % 32;
				const uint16_t remainLoopCount = (bodyLoopCountRemainder > 1) ? 1 : 0;
				const uint16_t patternLoopCount = bodyLoopCountQuotient + remainLoopCount;
				for (uint16_t i=0; i<patternLoopCount; ++i) {
					uint32_t bits = *pWorkBits++;
					uint16_t innerLoopCount = 32/2;
					if (i == patternLoopCount-1 && remainLoopCount) {
						innerLoopCount = bodyLoopCountRemainder / 2;
					}
					for (uint16_t j=0; j<innerLoopCount; ++j) {
						__m128i tmpBase = load_unaligned_128(pTmp);
						__m128i abcd = load_unaligned_128(pSrc++);
						const uint16_t pattern = bits & 0x3;
						bits >>= 2;
						__m128i ratiosSrc = _mm_loadl_epi64((const __m128i*)pWorkTailTargetRatios);
						OffsetPtr(pWorkTailTargetRatios, 8);
						ratiosSrc = _mm_unpacklo_epi16(ratiosSrc, ratiosSrc);
						const __m128i ratios01 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(1,1,0,0));

						switch (pattern) {
						case 0: // 10 10
							{
								__m128i adbc = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(2,1,3,0));
								__m128i ad = _mm_unpacklo_epi8(adbc, _mm_setzero_si128());
								__m128i bc = _mm_unpackhi_epi8(adbc, _mm_setzero_si128());
								__m128i bc_multiplied = _mm_mulhi_epu16(bc, ratios01);
								__m128i bc_subtracted = _mm_sub_epi16(bc, bc_multiplied);
								
								__m128i colFirst = _mm_add_epi16(_mm_add_epi16(col2, ad), bc_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
								OffsetPtr(pTmp, 8);
								
								__m128i colSecond = _mm_add_epi16(bc_subtracted, _mm_srli_si128(bc_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
								OffsetPtr(pTmp, 8);
								tmpBase = load_unaligned_128(pTmp);
								
								__m128i ef = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
								OffsetPtr(pSrc, 8);
								const __m128i ratios23 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(3,3,2,2));
								__m128i ef_multiplied = _mm_mulhi_epu16(ef, ratios23);
								__m128i ef_subtracted = _mm_sub_epi16(ef, ef_multiplied);
								
								__m128i colThird = _mm_srli_si128(_mm_add_epi16(bc_subtracted, ad), 8);
								colThird = _mm_add_epi16(colThird, ef_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
								OffsetPtr(pTmp, 8);
								
								__m128i colFourth = _mm_add_epi16(ef_subtracted, _mm_srli_si128(ef_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colFourth));
								OffsetPtr(pTmp, 8);
								
								col2 = _mm_srli_si128(ef_subtracted, 8);
							}
							break;
						case 1: // 110 10
							{
								__m128i efgh = load_unaligned_128(pSrc++);	// preload
								
								__m128i acbd = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(3,1,2,0));
								__m128i ac = _mm_unpacklo_epi8(acbd, _mm_setzero_si128());
								__m128i bd = _mm_unpackhi_epi8(acbd, _mm_setzero_si128());
								__m128i bd_multiplied = _mm_mulhi_epu16(bd, ratios01);
								__m128i bd_subtracted = _mm_sub_epi16(bd, bd_multiplied);
								
								__m128i sum_ac_bd_multiplied = _mm_add_epi16(ac, bd_multiplied);
								__m128i colFirst = _mm_add_epi16(col2, sum_ac_bd_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
								OffsetPtr(pTmp, 8);
								
								__m128i colSecond = _mm_add_epi16(bd_subtracted, _mm_srli_si128(sum_ac_bd_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
								OffsetPtr(pTmp, 8);
								tmpBase = load_unaligned_128(pTmp);
								
								__m128i egfh = _mm_shuffle_epi32(efgh, _MM_SHUFFLE(3,1,2,0));
								__m128i eg = _mm_unpacklo_epi8(egfh, _mm_setzero_si128());
								__m128i fh = _mm_unpackhi_epi8(egfh, _mm_setzero_si128());
								const __m128i ratios23 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(3,3,2,2));
								__m128i eg_multiplied = _mm_mulhi_epu16(eg, ratios23);
								__m128i eg_subtracted = _mm_sub_epi16(eg, eg_multiplied);
								
								__m128i colThird = _mm_add_epi16(_mm_srli_si128(bd_subtracted, 8), eg_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
								OffsetPtr(pTmp, 8);

								__m128i colFourth = _mm_add_epi16(eg_subtracted, fh);
								colFourth = _mm_add_epi16(colFourth, _mm_srli_si128(eg_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colFourth));
								OffsetPtr(pTmp, 8);
								
								const __m128i ratios5 = set1_epi16_low(*pWorkTailTargetRatios++);
								tmpBase = _mm_loadl_epi64(pTmp);
								__m128i h = _mm_srli_si128(fh, 8);
								__m128i h_multiplied = _mm_mulhi_epu16(h, ratios5);
								__m128i h_subtracted = _mm_sub_epi16(h, h_multiplied);
								
								__m128i colFifth = _mm_add_epi16(_mm_srli_si128(eg_subtracted, 8), h_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFifth));
								OffsetPtr(pTmp, 8);
								
								col2 = h_subtracted;
							}
							break;
						case 2: // 10 110
							{
								__m128i efgh = load_unaligned_128(pSrc++); // preload

								__m128i adbc = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(2,1,3,0));
								__m128i ad = _mm_unpacklo_epi8(adbc, _mm_setzero_si128());
								__m128i bc = _mm_unpackhi_epi8(adbc, _mm_setzero_si128());
								__m128i bc_multiplied = _mm_mulhi_epu16(bc, ratios01);
								__m128i bc_subtracted = _mm_sub_epi16(bc, bc_multiplied);
								
								__m128i colFirst = _mm_add_epi16(col2, _mm_add_epi16(ad, bc_multiplied));
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
								OffsetPtr(pTmp, 8);
								
								__m128i colSecond = _mm_add_epi16(bc_subtracted, _mm_srli_si128(bc_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
								OffsetPtr(pTmp, 8);
								tmpBase = load_unaligned_128(pTmp);
								
								__m128i gehf = _mm_shuffle_epi32(efgh, _MM_SHUFFLE(1,3,0,2));
								__m128i ge = _mm_unpacklo_epi8(gehf, _mm_setzero_si128());
								__m128i hf = _mm_unpackhi_epi8(gehf, _mm_setzero_si128());
								const __m128i ratios23 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(2,2,3,3));
								__m128i ge_multiplied = _mm_mulhi_epu16(ge, ratios23);
								__m128i ge_subtracted = _mm_sub_epi16(ge, ge_multiplied);
								
								__m128i colThird = _mm_srli_si128(_mm_add_epi16(_mm_add_epi16(bc_subtracted, ad), ge_multiplied), 8);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
								OffsetPtr(pTmp, 8);
								
								__m128i colFourth = _mm_add_epi16(_mm_srli_si128(_mm_add_epi16(ge_subtracted, hf), 8), ge_multiplied);
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colFourth));
								OffsetPtr(pTmp, 8);
								
								const __m128i ratios5 = set1_epi16_low(*pWorkTailTargetRatios++);
								tmpBase = _mm_loadl_epi64(pTmp);
								__m128i hf_multiplied = _mm_mulhi_epu16(hf, ratios5);
								__m128i hf_subtracted = _mm_sub_epi16(hf, hf_multiplied);
								
								__m128i colFifth = _mm_add_epi16(ge_subtracted, hf_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFifth));
								OffsetPtr(pTmp, 8);
								
								col2 = hf_subtracted;
							}
							break;
						case 3: // 110 110
							{
								__m128i efgh = load_unaligned_128(pSrc++); // preload

								__m128i acbd = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(3,1,2,0));
								__m128i ac = _mm_unpacklo_epi8(acbd, _mm_setzero_si128());
								__m128i bd = _mm_unpackhi_epi8(acbd, _mm_setzero_si128());
								__m128i bd_multiplied = _mm_mulhi_epu16(bd, ratios01);
								__m128i bd_subtracted = _mm_sub_epi16(bd, bd_multiplied);
								
								__m128i sum_ac_bd_multiplied = _mm_add_epi16(ac, bd_multiplied);
								__m128i colFirst = _mm_add_epi16(col2, sum_ac_bd_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
								OffsetPtr(pTmp, 8);
								
								__m128i colSecond = _mm_add_epi16(bd_subtracted, _mm_srli_si128(sum_ac_bd_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
								OffsetPtr(pTmp, 8);
								tmpBase = load_unaligned_128(pTmp);
								
								__m128i egfh = _mm_shuffle_epi32(efgh, _MM_SHUFFLE(3,1,2,0));
								__m128i eg = _mm_unpacklo_epi8(egfh, _mm_setzero_si128());
								__m128i fh = _mm_unpackhi_epi8(egfh, _mm_setzero_si128());
								const __m128i ratios23 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(3,3,2,2));
								__m128i eg_multiplied = _mm_mulhi_epu16(eg, ratios23);
								__m128i eg_subtracted = _mm_sub_epi16(eg, eg_multiplied);
								
								__m128i colThird = _mm_add_epi16(_mm_srli_si128(bd_subtracted, 8), eg_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
								OffsetPtr(pTmp, 8);
								
								__m128i sum_eg_subtracted_fh = _mm_add_epi16(eg_subtracted, fh);

								__m128i colForth = _mm_add_epi16(sum_eg_subtracted_fh, _mm_srli_si128(eg_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colForth));
								OffsetPtr(pTmp, 8);
								tmpBase = load_unaligned_128(pTmp);
								
								__m128i ij = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
								OffsetPtr(pSrc, 8);
								const __m128i ratiosOrg = _mm_cvtsi32_si128(*(const int*)pWorkTailTargetRatios);
								OffsetPtr(pWorkTailTargetRatios, 4);
								const __m128i ratiosTmp = _mm_shufflelo_epi16(ratiosOrg, _MM_SHUFFLE(1,1,0,0));
								const __m128i ratios45 = _mm_shuffle_epi32(ratiosTmp, _MM_SHUFFLE(1,1,0,0));
								__m128i ij_multiplied = _mm_mulhi_epu16(ij, ratios45);
								__m128i ij_subtracted = _mm_sub_epi16(ij, ij_multiplied);
								
								__m128i colFifth = _mm_add_epi16(_mm_srli_si128(sum_eg_subtracted_fh, 8), ij_multiplied);
								_mm_storel_epi64(pTmp, T::work(tmpBase, colFifth));
								OffsetPtr(pTmp, 8);
								
								__m128i colSixth = _mm_add_epi16(ij_subtracted, _mm_srli_si128(ij_multiplied, 8));
								_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSixth));
								OffsetPtr(pTmp, 8);
								
								col2 = _mm_srli_si128(ij_subtracted, 8);
							}
							break;
						default:
							__assume(false);
						}
					}
				}
				if (bodyLoopCountRemainder & 1) {
					__m128i tmpBase = load_unaligned_128(pTmp);
					__m128i ratiosSrc = _mm_loadl_epi64((const __m128i*)pWorkTailTargetRatios);
					__m128i abcd = load_unaligned_128(pSrc++);
					ratiosSrc = _mm_unpacklo_epi16(ratiosSrc, ratiosSrc);
					if (*pBodyCountPatternBits_ & 2) { // 1101
						__m128i efgh = load_unaligned_128(pSrc); // preload
						OffsetPtr(pSrc, 12);
						
						__m128i acbd = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(3,1,2,0));
						__m128i ac = _mm_unpacklo_epi8(acbd, _mm_setzero_si128());
						__m128i bd = _mm_unpackhi_epi8(acbd, _mm_setzero_si128());
						const __m128i ratios01 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(1,1,0,0));
						__m128i bd_multiplied = _mm_mulhi_epu16(bd, ratios01);
						__m128i bd_subtracted = _mm_sub_epi16(bd, bd_multiplied);

						__m128i sum_ac_bd_multiplied = _mm_add_epi16(ac, bd_multiplied);
						__m128i colFirst = _mm_add_epi16(col2, sum_ac_bd_multiplied);
						_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
						OffsetPtr(pTmp, 8);
						
						__m128i colSecond = _mm_add_epi16(bd_subtracted, _mm_srli_si128(sum_ac_bd_multiplied, 8));
						_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
						OffsetPtr(pTmp, 8);
						tmpBase = load_unaligned_128(pTmp);
						
						__m128i egfh = _mm_shuffle_epi32(efgh, _MM_SHUFFLE(3,1,2,0));
						__m128i eg = _mm_unpacklo_epi8(egfh, _mm_setzero_si128());
						__m128i fh = _mm_unpackhi_epi8(egfh, _mm_setzero_si128());
						const __m128i ratios23 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(3,3,2,2));
						__m128i eg_multiplied = _mm_mulhi_epu16(eg, ratios23);
						__m128i eg_subtracted = _mm_sub_epi16(eg, eg_multiplied);
						
						__m128i colThird = _mm_add_epi16(_mm_srli_si128(bd_subtracted, 8), eg_multiplied);
						_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
						OffsetPtr(pTmp, 8);

						__m128i colFourth = _mm_add_epi16(eg_subtracted, fh);
						colFourth = _mm_add_epi16(colFourth, _mm_srli_si128(eg_multiplied, 8));
						_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colFourth));
						OffsetPtr(pTmp, 8);

						col2 = _mm_srli_si128(eg_subtracted, 8);
						
					}else { // 101
						__m128i ef = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128()); // preload
						OffsetPtr(pSrc, 4);
						
						__m128i adbc = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(2,1,3,0));
						__m128i ad = _mm_unpacklo_epi8(adbc, _mm_setzero_si128());
						__m128i bc = _mm_unpackhi_epi8(adbc, _mm_setzero_si128());
						const __m128i ratios01 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(1,1,0,0));
						__m128i bc_multiplied = _mm_mulhi_epu16(bc, ratios01);
						__m128i bc_subtracted = _mm_sub_epi16(bc, bc_multiplied);
						
						__m128i colFirst = _mm_add_epi16(_mm_add_epi16(col2, ad), bc_multiplied);
						_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
						OffsetPtr(pTmp, 8);
						
						__m128i colSecond = _mm_add_epi16(bc_subtracted, _mm_srli_si128(bc_multiplied, 8));
						_mm_storel_epi64(pTmp, T::work(_mm_srli_si128(tmpBase, 8), colSecond));
						OffsetPtr(pTmp, 8);
						tmpBase = _mm_loadl_epi64(pTmp);
						
						const __m128i ratios23 = _mm_shuffle_epi32(ratiosSrc, _MM_SHUFFLE(3,3,2,2));
						__m128i ef_multiplied = _mm_mulhi_epu16(ef, ratios23);
						__m128i ef_subtracted = _mm_sub_epi16(ef, ef_multiplied);
						
						__m128i colThird = _mm_srli_si128(_mm_add_epi16(bc_subtracted, ad), 8);
						colThird = _mm_add_epi16(colThird, ef_multiplied);
						_mm_storel_epi64(pTmp, T::work(tmpBase, colThird));
						OffsetPtr(pTmp, 8);
						
						col2 = ef_subtracted;
					}
				}else { // 1
					__m128i ab = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
					OffsetPtr(pSrc, 8);
					__m128i ratios = set1_epi16_low(*pWorkTailTargetRatios);
					__m128i tmpBase = _mm_loadl_epi64(pTmp);
					
					__m128i b = _mm_srli_si128(ab, 8);
					__m128i b_multiplied = _mm_mulhi_epu16(b, ratios);
					__m128i b_subtracted = _mm_sub_epi16(b, b_multiplied);

					__m128i colFirst = _mm_add_epi16(_mm_add_epi16(col2, ab), b_multiplied);
					_mm_storel_epi64(pTmp, T::work(tmpBase, colFirst));
					OffsetPtr(pTmp, 8);
					
					col2 = b_subtracted;
				}
			}
			// tail
			{
				__m128i col = col2;
				col = _mm_add_epi16(col, CollectSum_1(pSrc));
				OffsetPtr(pSrc, 4);
				_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
				OffsetPtr(pTmp, 8);
			}
		}
	}
};

// 横 1 : N 比の縮小比率特化版
class LineAveragingReducer_Ratio1NX : public ILineAveragingReducer
{
public:
	uint16_t srcRatio_;
	uint16_t srcWidth_;
	
	ILineAveragingReducer_Impl;
	
	/*
	倍率の分だけデータを必要とする
	
	2 = 2,2
	3 = 3,1+2, 2+1,3
	4 = 4,4
	5 = 4+1,3+2, 2+3,1+4
	6 = 4+2,2+4
	7 = 4+3,1+4+2, 2+4+1,3+4
	8 = 4+4,4+4
	9 = 4+4+1,3+4+2, 2+4+3,1+4+4
	10 = 4 + 4 + 2, 2 + 4 + 4
	11 = 4 + 4 + 3, 1 + 4 + 4 + 2, 2 + 4 + 4 + 1, 3 + 4 + 4
	12 = 4 + 4 + 4, 4 + 4 + 4
	13 = 4 + 4 + 4 + 1, 3 + 4 + 4 + 2, 2 + 4 + 4 + 3, 1 + 4 + 4 + 4
	14 = 4 + 4 + 4 + 2, 2 + 4 + 4 + 4
	15 = 4 + 4 + 4 + 3, 1 + 4 + 4 + 4 + 2, 2 + 4 + 4 + 4 + 1, 3 + 4 + 4 + 4
	16 = 4 + 4 + 4 + 4, 4 + 4 + 4 + 4
	17 = 4 + 4 + 4 + 4 + 1, 3 + 4 + 4 + 4 + 2, 2 + 4 + 4 + 4 + 3, 1 + 4 + 4 + 4 + 4
	*/
	template <typename T>
	void iterate(const __m128i* srcBuff, __m128i* tmpBuff)
	{
		const __m128i* pSrc = srcBuff;
		__m128i* pTmp = tmpBuff;
		const uint16_t srcRatio = srcRatio_;
		const uint16_t srcWidth = srcWidth_;
		switch (srcRatio) {
		case 1:
			{
				const uint16_t loopCount = srcWidth / 4 + ((srcWidth % 4) ? 1 : 0);
				for (uint16_t i=0; i<loopCount; ++i) {
					__m128i fourPixels = srcBuff[i];
					__m128i twoPixels0 = _mm_unpacklo_epi8(fourPixels, _mm_setzero_si128());
					__m128i twoPixels1 = _mm_unpackhi_epi8(fourPixels, _mm_setzero_si128());
					tmpBuff[i*2+0] = T::work(tmpBuff[i*2+0], twoPixels0);
					tmpBuff[i*2+1] = T::work(tmpBuff[i*2+1], twoPixels1);
				}
			}
			break;
		case 2:
			// eats 4 src pixels at once and produces 2 tmp pixels
			{
				const uint16_t loopCount = srcWidth / 4;
				const uint16_t loopRemain = srcWidth % 4;
				const uint16_t remainLoopCount = loopRemain / 2;
				const uint16_t remainRemain = loopRemain % 2;
				__m128i tmp = *pTmp;
				__m128i src = *pSrc;
				for (uint16_t i=0; i<loopCount; ++i) {
					__m128i nSrc = *(pSrc+1);
					__m128i nTmp = *(pTmp+1);
					*pTmp = T::work(tmp, Scale2(src));
					src = nSrc;
					tmp = nTmp;
					++pSrc;
					++pTmp;
				}
				for (uint16_t i=0; i<remainLoopCount; ++i) {
					_mm_storel_epi64(pTmp, T::work(tmp, Split_2_0(src)));
					OffsetPtr(pSrc, 8);
					OffsetPtr(pTmp, 8);
				}
				// TODO: remaining 1 pixels
			}
			break;
		case 3:
			{
				// eats 12 src pixels at once and produces 4 tmp pixels
				__m128i result0, result1;
				const uint16_t loopCount = srcWidth / 12;
				const uint16_t remain = srcWidth % 12;
				const uint16_t remainLoopCount = remain / 3;
				const uint16_t remainRemain = remain % 3;
				for (uint16_t i=0; i<loopCount; ++i) {
					Scale3(pSrc, result0, result1);
					pTmp[0] = T::work(pTmp[0], result0);
					pTmp[1] = T::work(pTmp[1], result1);
					pSrc += 3;
					pTmp += 2;
				}
				for (uint16_t i=0; i<remainLoopCount; ++i) {
					_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), Split_3_1(_mm_loadu_si128(pSrc))));
					OffsetPtr(pSrc, 12);
					OffsetPtr(pTmp, 8);
				}
				// TODO: remaining 1 to 2 pixels
			}
			break;
		case 4:
			{
				// eats 8 src pixels at once and produces 2 tmp pixels
				const uint16_t loopCount = srcWidth / 8;
				const uint16_t remain = srcWidth % 8;
				const uint16_t remainLoopCount = remain / 4;
				const uint16_t remainRemain = remain % 4;
				for (uint16_t i=0; i<loopCount; ++i) {
					*pTmp = T::work(*pTmp, Scale4(pSrc));
					pSrc += 2;
					++pTmp;
				}
				for (uint16_t i=0; i<remainLoopCount; ++i) {
					_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), Collect_4(*pSrc)));
					++pSrc;
					OffsetPtr(pTmp, 8);
				}
				// TODO: remaining 1 to 3 pixels
			}
			break;
		default:
			{
				const uint16_t quotient = srcRatio / 4;
				const uint16_t remainderRatio = srcRatio % 4;
				switch (remainderRatio) {
				case 0:
					{
						const uint16_t stride = 8 * quotient;
						const uint16_t loopCount = srcWidth / stride;
						const uint16_t remainCount = srcWidth % stride;
						const uint16_t remainLoopCount = remainCount / srcRatio;
						const uint16_t remainRemain = remainCount % srcRatio;
						for (uint16_t i=0; i<loopCount; ++i) {
							__m128i tmp;
							__m128i left = CollectSum_4_aligned(pSrc, quotient);
							__m128i right = CollectSum_4_aligned(pSrc, quotient);
							right = _mm_slli_si128(right, 8);
							__m128i result = _mm_add_epi16(left, right);
							*pTmp = T::work(*pTmp, result);
							++pTmp;
						}
						for (uint16_t i=0; i<remainLoopCount; ++i) {
							__m128i col = CollectSum_4_aligned(pSrc, quotient);
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
							OffsetPtr(pTmp, 8);
						}
						// TODO: remaining 1 to srcRatio-1 pixels
					}
					break;
				case 1:
					{
						__m128i tmp;
						__m128i straddling1, straddling2, straddling3;
						__m128i left, right, result;
						const uint16_t stride = srcRatio * 4;
						const uint16_t loopCount = srcWidth / stride;
						const uint16_t remainCount = srcWidth % stride;
						const uint16_t remainLoopCount = remainCount / srcRatio;
						const uint16_t remainRemain = remainCount % srcRatio;
						for (uint16_t i=0; i<loopCount; ++i) {
							// 1 left
							left = CollectSum_4_aligned(pSrc, quotient);
							straddling1 = Split_1_3(*pSrc);
							++pSrc;
							// 2 right
							right = _mm_setzero_si128();
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								right = _mm_add_epi16(right, tmp);
							}
							straddling2 = Split_2_2(*pSrc);
							++pSrc;
							result = _mm_add_epi16(left, straddling1);
							tmp = _mm_add_epi16(right, straddling2);
							tmp = _mm_slli_si128(tmp, 8);
							result = _mm_add_epi16(result, tmp);
							*pTmp = T::work(*pTmp, result);
							++pTmp;

							// 3 left
							left = _mm_srli_si128(straddling2, 8);
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								left = _mm_add_epi16(left, tmp);
							}
							straddling3 = Split_3_1(*pSrc);
							++pSrc;
							// 4 right
							right = CollectSum_4_aligned(pSrc, quotient);
							result = _mm_add_epi16(left, straddling3);
							right = _mm_slli_si128(right, 8);
							result = _mm_add_epi16(result, right);
							*pTmp = T::work(*pTmp, result);
							++pTmp;
						}
						for (uint16_t i=0; i<remainLoopCount; ++i) {
							__m128i col = CollectSum_4(pSrc, quotient);
							col = _mm_add_epi16(col, CollectSum_1(pSrc));
							OffsetPtr(pSrc, 4);
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
							OffsetPtr(pTmp, 8);
						}
						// TODO: remaining 1 to srcRatio-1 pixels
					}
					break;
				case 2:
					{
						__m128i left, right, result;
						__m128i tmp;
						__m128i straddling;
						const uint16_t stride = srcRatio * 2;
						const uint16_t loopCount = srcWidth / stride;
						const uint16_t remainCount = srcWidth % stride;
						const uint16_t remainLoopCount = remainCount / srcRatio;
						const uint16_t remainRemain = remainCount % srcRatio;
						for (uint16_t i=0; i<loopCount; ++i) {
							// left
							left = CollectSum_4_aligned(pSrc, quotient);
							straddling = Split_2_2(*pSrc);
							++pSrc;
							// right
							right = CollectSum_4_aligned(pSrc, quotient);
							result = _mm_add_epi16(left, straddling);
							right = _mm_slli_si128(right, 8);
							result = _mm_add_epi16(result, right);
							*pTmp = T::work(*pTmp, result);
							++pTmp;
						}
						assert(remainLoopCount < 2);
						if (remainLoopCount) {
							__m128i col = CollectSum_4(pSrc, quotient);
							col = _mm_add_epi16(col, CollectSum_2(pSrc));
							OffsetPtr(pSrc, 8);
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
							OffsetPtr(pTmp, 8);
						}
						// TODO: remaining 1 to srcRatio-1 pixels
					}
					break;
				case 3:
					{
						__m128i tmp;
						__m128i straddling1, straddling2, straddling3;
						__m128i left, right, result;
						const uint16_t stride = srcRatio * 4;
						const uint16_t loopCount = srcWidth / stride;
						const uint16_t remainCount = srcWidth % stride;
						const uint16_t remainLoopCount = remainCount / srcRatio;
						const uint16_t remainRemain = remainCount % srcRatio;
						for (uint16_t i=0; i<loopCount; ++i) {
							// 1 left
							left = CollectSum_4_aligned(pSrc, quotient);
							straddling1 = Split_3_1(*pSrc);
							++pSrc;
							// 2 right
							right = CollectSum_4_aligned(pSrc, quotient);
							straddling2 = Split_2_2(*pSrc);
							++pSrc;
							result = _mm_add_epi16(left, straddling1);
							tmp = _mm_add_epi16(right, straddling2);
							tmp = _mm_slli_si128(tmp, 8);
							result = _mm_add_epi16(result, tmp);
							*pTmp = T::work(*pTmp, result);
							++pTmp;

							// 3 left
							left = _mm_srli_si128(straddling2, 8);
							left = _mm_add_epi16(left, CollectSum_4_aligned(pSrc, quotient));
							straddling3 = Split_1_3(*pSrc);
							++pSrc;
							// 4 right
							right = CollectSum_4_aligned(pSrc, quotient);
							result = _mm_add_epi16(left, straddling3);
							right = _mm_slli_si128(right, 8);
							result = _mm_add_epi16(result, right);
							*pTmp = T::work(*pTmp, result);
							++pTmp;
						}
						for (uint16_t i=0; i<remainLoopCount; ++i) {
							__m128i col = CollectSum_4(pSrc, quotient);
							col = _mm_add_epi16(col, CollectSum_3(pSrc));
							OffsetPtr(pSrc, 12);
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
							OffsetPtr(pTmp, 8);
						}
						// TODO: remaining 1 to srcRatio-1 pixels
					}
					break;
				default:
					__assume(false);
				}
			}
			break;
		}
	}

};

class AveragingReducer
{
public:
	AveragingReducer();
	void Setup(const AveragingReduceParams* pParams, uint16_t parts);
	void Process(uint16_t part);
	
private:
	const AveragingReduceParams* pParams_;
	uint16_t parts_;
	static const uint16_t TABLE_COUNT = 8192;
	uint8_t bodyCounts_[ TABLE_COUNT ];
	uint32_t bodyCountBits_[ TABLE_COUNT / 32 ];
	uint32_t bodyCountPatternBits_[ TABLE_COUNT / 32 ];

	uint16_t tailTargetRatios_[ TABLE_COUNT ];
	
	ILineAveragingReducer* pLineAveragingReducer;
	LineAveragingReducer_Ratio1NX lar_Ratio1NX;
	LineAveragingReducer_RatioAny_Basic<CollectSumAndNext1_X_3or2, CollectSumAndNext1_X_0or3, CollectSum_X_0or3> lar_RatioAny_Basic_5up_3;
	LineAveragingReducer_RatioAny_Basic<CollectSumAndNext1_X_2or1, CollectSumAndNext1_X_3or2, CollectSum_X_3or2> lar_RatioAny_Basic_5up_2;
	LineAveragingReducer_RatioAny_Basic<CollectSumAndNext1_X_1or0, CollectSumAndNext1_X_2or1, CollectSum_X_2or1> lar_RatioAny_Basic_5up_1;
	LineAveragingReducer_RatioAny_Basic<CollectSumAndNext1_X_0or3, CollectSumAndNext1_X_1or0, CollectSum_X_1or0> lar_RatioAny_Basic_5up_0;
	LineAveragingReducer_RatioAny_Basic<CollectSumAndNext1_4or3, CollectSumAndNext1_5or4, CollectSum_5or4> lar_RatioAny_Basic_4;
	LineAveragingReducer_RatioAny_Basic<CollectSumAndNext1_3or2, CollectSumAndNext1_4or3, CollectSum_4or3> lar_RatioAny_Basic_3;
	LineAveragingReducer_RatioAny_Basic<CollectSumAndNext1_2or1, CollectSumAndNext1_3or2, CollectSum_3or2> lar_RatioAny_Basic_2;
	LineAveragingReducer_RatioAny_Basic<CollectSumAndNext1_1or0, CollectSumAndNext1_2or1, CollectSum_2or1> lar_RatioAny_Basic_1;
	LineAveragingReducer_RatioAny_MoreThanHalf lar_RatioAny_MoreThanHalf;
	LineAveragingReducer_RatioAny_MoreThanHalf_10_110 lar_RatioAny_MoreThanHalf_10_110;
	LineAveragingReducer_RatioAny_MoreThanHalf_01_001 lar_RatioAny_MoreThanHalf_01_001;

	enum BodyCountPatternType
	{
		BodyCountPatternType_0001_00001,
		BodyCountPatternType_001_0001,
		BodyCountPatternType_01_001,
		BodyCountPatternType_10_110,
		BodyCountPatternType_110_1110,
		BodyCountPatternType_1110_11110,
		BodyCountPatternType_Other,
	};
	
	static void FindRepeatCounts(const uint8_t* srcBytes, const uint16_t srcLen, uint8_t checkNum, uint8_t* repeatCountRecords, uint16_t& repeatCountRecordsCount)
	{
		uint8_t repeatCount = 0;
		for (size_t i=0; i<srcLen; ++i) {
			uint8_t curVal = srcBytes[i];
			if (curVal == checkNum) {
				++repeatCount;
				if (i == srcLen - 1) {
					repeatCountRecords[repeatCountRecordsCount++] = repeatCount;
				}
			}else {
				repeatCountRecords[repeatCountRecordsCount++] = repeatCount;
				repeatCount = 0;
			}
		}
	}

	static void BinarizeBytes(const uint8_t* srcBytes, const uint16_t srcLen, uint32_t* targetBits, uint8_t onValue)
	{
		const uint8_t* workBytes = srcBytes;
		uint32_t* workBits = targetBits;
		for (uint16_t i=0; i<srcLen/32; ++i) {
			uint32_t& bits = *workBits++;
			bits = 0;
			for (uint16_t j=0; j<32; ++j) {
				bits |= (*workBytes++ == onValue) ? (1 << j) : 0;
			}
		}
		{
			uint32_t& bits = *workBits++;
			bits = 0;
			for (uint16_t i=0; i<srcLen%32; ++i) {
				bits |= (*workBytes++ == onValue) ? (1 << i) : 0;
			}
		}
	}

	static uint16_t RatioAny_PreCalculate(
		uint8_t* bodyCounts,
		uint32_t* bodyCountBits,
		BodyCountPatternType& bodyCountPatternType,
		uint32_t* bodyCountPatternBits,
		uint16_t& bodyCountPatternBitsCount,
		uint8_t& maxBodyCount,
		uint16_t* tailTargetRatios,
		uint16_t srcRatio,
		uint16_t targetRatio
		)
	{
		const uint16_t remainderRatio = srcRatio % targetRatio;
		uint16_t tailRatio = remainderRatio;
		const uint16_t limit = targetRatio - 2;
		uint16_t bodyCountSum = 0;
		const double invTargetRatio = 65536.0 / targetRatio;
		maxBodyCount = 0;
		for (uint16_t i=0; i<limit; ++i) {
			const uint16_t headRatio = targetRatio - tailRatio;
			const uint16_t withoutHead = srcRatio - headRatio;
			const uint8_t bodyCount = withoutHead / targetRatio;
			tailRatio = withoutHead % targetRatio;
			bodyCounts[i] = bodyCount;
			bodyCountSum += bodyCount;
			maxBodyCount = std::max<uint8_t>(maxBodyCount, bodyCount);
			tailTargetRatios[i] = (uint16_t)(invTargetRatio * tailRatio + 0.5);
		}
		BinarizeBytes(bodyCounts, limit, bodyCountBits, maxBodyCount);
		
		bodyCountPatternType = BodyCountPatternType_Other;
		if (maxBodyCount == 1 && targetRatio > 32) {
			uint32_t frontPattern = bodyCountBits[0];
			uint8_t baseCount = 0;
			if (bodyCounts[0] == 0) {
				if ((frontPattern & 0x0F) == 0x08) {
					bodyCountPatternType = BodyCountPatternType_0001_00001;
					baseCount = 3;
				}else if ((frontPattern & 0x07) == 0x04) {
					bodyCountPatternType = BodyCountPatternType_001_0001;
					baseCount = 2;
				}else if ((frontPattern & 0x03) == 0x02) {
					bodyCountPatternType = BodyCountPatternType_01_001;
					baseCount = 1;
				}
			}else {
				if ((frontPattern & 0x03) == 0x01) {
					bodyCountPatternType = BodyCountPatternType_10_110;
					baseCount = 1;
				}else if ((frontPattern & 0x07) == 0x03) {
					bodyCountPatternType = BodyCountPatternType_110_1110;
					baseCount = 2;
				}else if ((frontPattern & 0x0F) == 0x07) {
					bodyCountPatternType = BodyCountPatternType_1110_11110;
					baseCount = 3;
				}
			}
			if (bodyCountPatternType != BodyCountPatternType_Other) {
				uint8_t bodyCountRepeatRecords[8192];
				uint16_t bodyCountRepeatRecordsCount = 0;
				FindRepeatCounts(bodyCounts, limit, bodyCounts[0], bodyCountRepeatRecords, bodyCountRepeatRecordsCount);
				BinarizeBytes(bodyCountRepeatRecords, bodyCountRepeatRecordsCount, bodyCountPatternBits, baseCount+1);
				bodyCountPatternBitsCount = bodyCountRepeatRecordsCount;
			}
		}
		
#ifdef _DEBUG
		static TCHAR buff[20480];
		buff[0] = 0;
		TCHAR buff2[32];
		for (uint16_t i=0; i<limit; ++i) {
			_tcscat(buff, _itot(bodyCounts[i], buff2, 10));
			_tcscat(buff, _T(" "));
		}
		TRACE(_T("(%04d %04d) %s\n"), srcRatio, targetRatio, buff);
#endif
		return bodyCountSum;
	}
	
	void Process_Ratio1NX(ILineAveragingReducer& lineReducer, uint16_t part);
	void Process_RatioAny(ILineAveragingReducer& lineReducer, uint16_t part);
	
};

AveragingReducer::AveragingReducer()
	:
	lar_RatioAny_Basic_5up_3(bodyCountBits_, tailTargetRatios_),
	lar_RatioAny_Basic_5up_2(bodyCountBits_, tailTargetRatios_),
	lar_RatioAny_Basic_5up_1(bodyCountBits_, tailTargetRatios_),
	lar_RatioAny_Basic_5up_0(bodyCountBits_, tailTargetRatios_),
	lar_RatioAny_Basic_4(bodyCountBits_, tailTargetRatios_),
	lar_RatioAny_Basic_3(bodyCountBits_, tailTargetRatios_),
	lar_RatioAny_Basic_2(bodyCountBits_, tailTargetRatios_),
	lar_RatioAny_Basic_1(bodyCountBits_, tailTargetRatios_),
	lar_RatioAny_MoreThanHalf(bodyCounts_, tailTargetRatios_),
	lar_RatioAny_MoreThanHalf_10_110(bodyCountPatternBits_, tailTargetRatios_),
	lar_RatioAny_MoreThanHalf_01_001(bodyCountPatternBits_, tailTargetRatios_)
{
}

void AveragingReducer::Setup(const AveragingReduceParams* pParams, uint16_t parts)
{
	assert(pParams);
	this->pParams_ = pParams;
	this->parts_ = parts;
	
	const AveragingReduceParams& params = *pParams;
	//	assert((params.srcWidth % 4) == 0);
	assert(params.srcLineOffsetBytes % 16 == 0);
	assert(params.targetLineOffsetBytes % 16 == 0);
	assert(params.heightRatioTarget <= params.heightRatioSource);
	assert(params.widthRatioTarget <= params.widthRatioSource);
	
	if (params.widthRatioTarget == params.widthRatioSource && params.heightRatioTarget == params.heightRatioSource) {
		pLineAveragingReducer = 0;
	}else if ((params.widthRatioSource % params.widthRatioTarget) == 0) {
		lar_Ratio1NX.srcRatio_ = params.widthRatioSource;
		lar_Ratio1NX.srcWidth_ = params.srcWidth;
		pLineAveragingReducer = &lar_Ratio1NX;
	}else {
		uint8_t maxBodyCount;
		BodyCountPatternType bodyCountPatternType = BodyCountPatternType_Other;
		uint16_t bodyCountPatternBitsCount = 0;
		RatioAny_PreCalculate(
			bodyCounts_,
			bodyCountBits_,
			bodyCountPatternType,
			bodyCountPatternBits_,
			bodyCountPatternBitsCount,
			maxBodyCount,
			tailTargetRatios_,
			params.widthRatioSource,
			params.widthRatioTarget
		);
		if (params.widthRatioSource > params.widthRatioTarget * 2) {
			if (params.widthRatioTarget == 2 && (params.widthRatioSource / params.widthRatioTarget) < 4) {
				maxBodyCount = (params.widthRatioSource / params.widthRatioTarget) - 1;
			}
			switch (maxBodyCount) {
			case 4:
				lar_RatioAny_Basic_4.srcRatio_ = params.widthRatioSource;
				lar_RatioAny_Basic_4.targetRatio_ = params.widthRatioTarget;
				lar_RatioAny_Basic_4.srcWidth_ = params.srcWidth;
				lar_RatioAny_Basic_4.maxBodyCount_ = maxBodyCount;
				pLineAveragingReducer = &lar_RatioAny_Basic_4;
				break;
			case 3:
				lar_RatioAny_Basic_3.srcRatio_ = params.widthRatioSource;
				lar_RatioAny_Basic_3.targetRatio_ = params.widthRatioTarget;
				lar_RatioAny_Basic_3.srcWidth_ = params.srcWidth;
				lar_RatioAny_Basic_3.maxBodyCount_ = maxBodyCount;
				pLineAveragingReducer = &lar_RatioAny_Basic_3;
				break;
			case 2:
				lar_RatioAny_Basic_2.srcRatio_ = params.widthRatioSource;
				lar_RatioAny_Basic_2.targetRatio_ = params.widthRatioTarget;
				lar_RatioAny_Basic_2.srcWidth_ = params.srcWidth;
				lar_RatioAny_Basic_2.maxBodyCount_ = maxBodyCount;
				pLineAveragingReducer = &lar_RatioAny_Basic_2;
				break;
			case 1:
				lar_RatioAny_Basic_1.srcRatio_ = params.widthRatioSource;
				lar_RatioAny_Basic_1.targetRatio_ = params.widthRatioTarget;
				lar_RatioAny_Basic_1.srcWidth_ = params.srcWidth;
				lar_RatioAny_Basic_1.maxBodyCount_ = maxBodyCount;
				pLineAveragingReducer = &lar_RatioAny_Basic_1;
				break;
			default:
				if (params.widthRatioTarget == 2) {
					maxBodyCount = (params.widthRatioSource / params.widthRatioTarget) - 1;
				}
				switch (maxBodyCount % 4) {
				case 3:
					lar_RatioAny_Basic_5up_3.srcRatio_ = params.widthRatioSource;
					lar_RatioAny_Basic_5up_3.targetRatio_ = params.widthRatioTarget;
					lar_RatioAny_Basic_5up_3.srcWidth_ = params.srcWidth;
					lar_RatioAny_Basic_5up_3.maxBodyCount_ = maxBodyCount;
					pLineAveragingReducer = &lar_RatioAny_Basic_5up_3;
					break;
				case 2:
					lar_RatioAny_Basic_5up_2.srcRatio_ = params.widthRatioSource;
					lar_RatioAny_Basic_5up_2.targetRatio_ = params.widthRatioTarget;
					lar_RatioAny_Basic_5up_2.srcWidth_ = params.srcWidth;
					lar_RatioAny_Basic_5up_2.maxBodyCount_ = maxBodyCount;
					pLineAveragingReducer = &lar_RatioAny_Basic_5up_2;
					break;
				case 1:
					lar_RatioAny_Basic_5up_1.srcRatio_ = params.widthRatioSource;
					lar_RatioAny_Basic_5up_1.targetRatio_ = params.widthRatioTarget;
					lar_RatioAny_Basic_5up_1.srcWidth_ = params.srcWidth;
					lar_RatioAny_Basic_5up_1.maxBodyCount_ = maxBodyCount;
					pLineAveragingReducer = &lar_RatioAny_Basic_5up_1;
					break;
				case 0:
					lar_RatioAny_Basic_5up_0.srcRatio_ = params.widthRatioSource;
					lar_RatioAny_Basic_5up_0.targetRatio_ = params.widthRatioTarget;
					lar_RatioAny_Basic_5up_0.srcWidth_ = params.srcWidth;
					lar_RatioAny_Basic_5up_0.maxBodyCount_ = maxBodyCount;
					pLineAveragingReducer = &lar_RatioAny_Basic_5up_0;
					break;
				}
				break;
			}

		}else {
			switch (bodyCountPatternType) {
			case BodyCountPatternType_10_110:
				lar_RatioAny_MoreThanHalf_10_110.srcRatio_ = params.widthRatioSource;
				lar_RatioAny_MoreThanHalf_10_110.targetRatio_ = params.widthRatioTarget;
				lar_RatioAny_MoreThanHalf_10_110.srcWidth_ = params.srcWidth;
				lar_RatioAny_MoreThanHalf_10_110.bodyCountPatternBitsCount_ = bodyCountPatternBitsCount;
				pLineAveragingReducer = &lar_RatioAny_MoreThanHalf_10_110;
				break;
			case BodyCountPatternType_01_001:
				lar_RatioAny_MoreThanHalf_01_001.srcRatio_ = params.widthRatioSource;
				lar_RatioAny_MoreThanHalf_01_001.targetRatio_ = params.widthRatioTarget;
				lar_RatioAny_MoreThanHalf_01_001.srcWidth_ = params.srcWidth;
				lar_RatioAny_MoreThanHalf_01_001.bodyCountPatternBitsCount_ = bodyCountPatternBitsCount;
				pLineAveragingReducer = &lar_RatioAny_MoreThanHalf_01_001;
				break;
			case BodyCountPatternType_Other:
			default:
				lar_RatioAny_MoreThanHalf.srcRatio_ = params.widthRatioSource;
				lar_RatioAny_MoreThanHalf.targetRatio_ = params.widthRatioTarget;
				lar_RatioAny_MoreThanHalf.srcWidth_ = params.srcWidth;
				pLineAveragingReducer = &lar_RatioAny_MoreThanHalf;
				break;
			}
		}
	}
}

template <typename LineReducerT>
void ReadLines(LineReducerT& lineReducer, const __m128i*& pSrc, __m128i* pTarget, ptrdiff_t offsetBytes, uint16_t addReadCount)
{
	lineReducer.fillRead(pSrc, pTarget);
	OffsetPtr(pSrc, offsetBytes);
	for (uint16_t i=0; i<addReadCount; ++i) {
		lineReducer.addRead(pSrc, pTarget);
		OffsetPtr(pSrc, offsetBytes);
	}
}

template <typename LineReducerT>
void AddLines(LineReducerT& lineReducer, const __m128i*& pSrc, __m128i* pTarget, ptrdiff_t offsetBytes, uint16_t addReadCount)
{
	for (uint16_t i=0; i<addReadCount; ++i) {
		lineReducer.addRead(pSrc, pTarget);
		OffsetPtr(pSrc, offsetBytes);
	}
}

// 縦 1:N
void AveragingReducer::Process_Ratio1NX(ILineAveragingReducer& lineReducer, uint16_t part)
{
	const AveragingReduceParams& params = *pParams_;
	
	const uint32_t xLoopCount = getTargetLineArrayCount(params);
	const uint16_t yLoopCount = params.srcHeight / params.heightRatioSource;
	__m128i* targetLine = params.targetBuff;
	const __m128i* srcLine = params.srcBuff;
	__m128i* tmpBuff = params.tmpBuff;
	const __m128i invertRatioSource = make_vec_ratios(params.widthRatioTarget << 16, params.widthRatioSource * params.heightRatioSource);
	
	if (part >= yLoopCount) {
		return;
	}
	const uint16_t useParts = min(parts_, yLoopCount);
	const uint16_t partLen = yLoopCount / useParts;
	const uint16_t yStart = partLen * part;
	const uint16_t yEnd = (part == useParts-1) ? yLoopCount : (yStart + partLen);
	OffsetPtr(srcLine, params.srcLineOffsetBytes * yStart * params.heightRatioSource);
	OffsetPtr(targetLine, params.targetLineOffsetBytes * yStart);
	tmpBuff += xLoopCount * 2 * part;
	
	for (uint16_t y=yStart; y<yEnd; ++y) {
		// 最初のライン set to temp
		// 次のラインから最後の前のラインまで plusEqual to temp
		ReadLines(lineReducer, srcLine, tmpBuff, params.srcLineOffsetBytes, params.heightRatioSource - 1u);
		// store
		for (uint16_t x=0; x<xLoopCount; ++x) {
			__m128i p0 = tmpBuff[x*2+0];
			__m128i p1 = tmpBuff[x*2+1];
			p0 = _mm_mulhi_epu16(p0, invertRatioSource);
			p1 = _mm_mulhi_epu16(p1, invertRatioSource);
			// 16 -> 8
			_mm_stream_si128(targetLine+x, _mm_packus_epi16(p0, p1));
		}
		OffsetPtr(targetLine, params.targetLineOffsetBytes);
	}
	const uint16_t remain = params.srcHeight % params.heightRatioSource;
}

void StoreToTarget(
	uint16_t count,
	__m128i* targetBuff,
	const __m128i* tmpBuffBody, __m128i* tmpBuffEnd,
	__m128i hEndTargetRatio, __m128i hvTargetSrcRatio
	)
{
	for (uint16_t i=0; i<count; ++i) {
		__m128i endPixels0 = tmpBuffEnd[i*2+0];
		__m128i endPixelsA = _mm_mulhi_epu16(endPixels0, hEndTargetRatio);
		tmpBuffEnd[i*2+0] = _mm_sub_epi16(endPixels0, endPixelsA);

		__m128i endPixels1 = tmpBuffEnd[i*2+1];
		__m128i endPixelsB = _mm_mulhi_epu16(endPixels1, hEndTargetRatio);
		tmpBuffEnd[i*2+1] = _mm_sub_epi16(endPixels1, endPixelsB);

		__m128i twoPixels0 = _mm_adds_epu16(tmpBuffBody[i*2+0], endPixelsA);
		__m128i twoPixels1 = _mm_adds_epu16(tmpBuffBody[i*2+1], endPixelsB);
		__m128i p0 = _mm_mulhi_epu16(twoPixels0, hvTargetSrcRatio);
		__m128i p1 = _mm_mulhi_epu16(twoPixels1, hvTargetSrcRatio);
		
		// 16 -> 8
		_mm_stream_si128(targetBuff+i, _mm_packus_epi16(p0, p1));
	}
}

// creates next head pixels
// saves (head+body + tail) * ratio
void StoreToTarget(
	uint16_t count,
	__m128i* targetBuff, const __m128i* tmpBuffBody, __m128i* tmpBuffEnd,
	uint32_t endRatio, uint32_t targetRatio, __m128i hvTargetSrcRatio
	)
{
	const __m128i hEndTargetRatio = make_vec_ratios(endRatio << 16, targetRatio);
	StoreToTarget(count, targetBuff, tmpBuffBody, tmpBuffEnd, hEndTargetRatio, hvTargetSrcRatio);
}

// saves (head+body+tail) * ratio
void StoreToTarget(
	uint16_t count,
	__m128i* targetBuff,
	__m128i* tmpBuff,
	__m128i hvTargetSrcRatio
	)
{
	for (uint16_t i=0; i<count; ++i) {
		__m128i p0 = _mm_mulhi_epu16(tmpBuff[i*2+0], hvTargetSrcRatio);
		__m128i p1 = _mm_mulhi_epu16(tmpBuff[i*2+1], hvTargetSrcRatio);
		
		// 16 -> 8
		_mm_stream_si128(targetBuff+i, _mm_packus_epi16(p0, p1));
	}
}

// just creates next head pixels
void StoreToTarget(
	uint16_t count,
	__m128i* tmpBuffEnd,
	uint32_t endRatio, uint32_t targetRatio
	)
{
	const __m128i hEndTargetRatio = make_vec_ratios(endRatio << 16, targetRatio);
	for (uint16_t i=0; i<count; ++i) {
		__m128i endPixels0 = tmpBuffEnd[i*2+0];
		__m128i endPixelsA = _mm_mulhi_epu16(endPixels0, hEndTargetRatio);
		tmpBuffEnd[i*2+0] = _mm_sub_epi16(endPixels0, endPixelsA);

		__m128i endPixels1 = tmpBuffEnd[i*2+1];
		__m128i endPixelsB = _mm_mulhi_epu16(endPixels1, hEndTargetRatio);
		tmpBuffEnd[i*2+1] = _mm_sub_epi16(endPixels1, endPixelsB);
	}
}


// 縦 自由比率縮小
// thread化するとしたら、bodyのtargetRatioのloopを分割か。。
// targetRatioが2の場合に内側のbodyのループが存在しないので分割出来ない。その場合はsrcRatioが小さい場合は外側のループが分割出来るので問題無い。大きい場合は縮小比率自体が大きいので
void AveragingReducer::Process_RatioAny(ILineAveragingReducer& lineReducer, uint16_t part)
{
	const AveragingReduceParams& params = *pParams_;
	
	const __m128i* pSrc = params.srcBuff;
	__m128i* pTarget = params.targetBuff;
	
	const size_t denominator = params.widthRatioTarget * params.srcWidth;
	size_t tmpLineOffsetBytes = ((denominator / params.widthRatioSource) + (denominator % params.widthRatioSource)) * 4 * 2;
	tmpLineOffsetBytes += 16 - (tmpLineOffsetBytes % 16);
	tmpLineOffsetBytes += 256 - (tmpLineOffsetBytes % 256);
		
	__m128i* tmpBuffHead = params.tmpBuff;
	__m128i* tmpBuffTail = params.tmpBuff;
	OffsetPtr(tmpBuffTail, tmpLineOffsetBytes);
	
	const uint16_t srcRatio = params.heightRatioSource;
	const uint16_t targetRatio = params.heightRatioTarget;
	
	const uint16_t remainderRatio = srcRatio % targetRatio;
	const uint16_t endBodyCnt = (srcRatio - targetRatio - remainderRatio) / targetRatio;
	const uint32_t hvTargetRatio = targetRatio * params.widthRatioTarget;
	const uint32_t hvSrcRatio = srcRatio * params.widthRatioSource;
	const __m128i hRemainderTargetRatio = make_vec_ratios(remainderRatio << 16, targetRatio);
	const __m128i hvTargetSrcRatio = _mm_set1_epi16((65536.0 * hvTargetRatio) / hvSrcRatio + 0.5);
	uint16_t headRatio;
	
	const uint16_t storeCount = getTargetLineArrayCount(params);

	OffsetPtr(tmpBuffHead, tmpLineOffsetBytes*2*part);
	OffsetPtr(tmpBuffTail, tmpLineOffsetBytes*2*part);
	const uint16_t blockCount = params.srcHeight / srcRatio;
	const uint16_t bodyCount = targetRatio - 2u;
	if (parts_ > 1 && blockCount == 1 && bodyCount >= 4) {
		const uint16_t partLen = bodyCount / parts_;
		const uint16_t bodyStart = part * partLen;
		const uint16_t bodyEnd = (part == parts_ - 1) ? bodyCount : (bodyStart + partLen);
		{
			// head
			if (bodyStart == 0) {
				// head + body
				ReadLines(lineReducer, pSrc, tmpBuffHead, params.srcLineOffsetBytes, endBodyCnt);
				// srcRatio data straddles targetRatio's tail and next head
				lineReducer.fillRead(pSrc, tmpBuffTail);
				OffsetPtr(pSrc, params.srcLineOffsetBytes);
				headRatio = targetRatio - remainderRatio;
				StoreToTarget(storeCount, pTarget, tmpBuffHead, tmpBuffTail, hRemainderTargetRatio, hvTargetSrcRatio);
				OffsetPtr(pTarget, params.targetLineOffsetBytes);
				std::swap(tmpBuffHead, tmpBuffTail);
			}else {
				headRatio = targetRatio - remainderRatio;
				OffsetPtr(pSrc, params.srcLineOffsetBytes * (1 + endBodyCnt + 1));
				OffsetPtr(pTarget, params.targetLineOffsetBytes);
			}
			// body
			{
				const uint16_t withoutHead = srcRatio - headRatio;
				uint16_t innerBodyCnt = withoutHead / targetRatio;
				uint16_t tailRatio = withoutHead % targetRatio;
				if (bodyStart != 0) {
					// skip
					for (uint16_t i=0; i<bodyStart-1; ++i) {
						// body tail and next head
						OffsetPtr(pSrc, params.srcLineOffsetBytes * (innerBodyCnt+1));
						OffsetPtr(pTarget, params.targetLineOffsetBytes);
						headRatio = targetRatio - tailRatio;
						const uint16_t withoutHead = srcRatio - headRatio;
						innerBodyCnt = withoutHead / targetRatio;
						tailRatio = withoutHead % targetRatio;
						std::swap(tmpBuffHead, tmpBuffTail);
					}
					// body
					OffsetPtr(pSrc, params.srcLineOffsetBytes * innerBodyCnt);
					// tail and next head
					lineReducer.fillRead(pSrc, tmpBuffTail);
					OffsetPtr(pSrc, params.srcLineOffsetBytes);
					StoreToTarget(storeCount, tmpBuffTail, tailRatio, targetRatio);
					OffsetPtr(pTarget, params.targetLineOffsetBytes);
					headRatio = targetRatio - tailRatio;
					const uint16_t withoutHead = srcRatio - headRatio;
					innerBodyCnt = withoutHead / targetRatio;
					tailRatio = withoutHead % targetRatio;
					std::swap(tmpBuffHead, tmpBuffTail);
				}
				for (uint16_t i=bodyStart; i<bodyEnd; ++i) {
					// body
					AddLines(lineReducer, pSrc, tmpBuffHead, params.srcLineOffsetBytes, innerBodyCnt);
					// tail and next head
					lineReducer.fillRead(pSrc, tmpBuffTail);
					OffsetPtr(pSrc, params.srcLineOffsetBytes);
					StoreToTarget(storeCount, pTarget, tmpBuffHead, tmpBuffTail, tailRatio, targetRatio, hvTargetSrcRatio);
					OffsetPtr(pTarget, params.targetLineOffsetBytes);
					
					headRatio = targetRatio - tailRatio;
					const uint16_t withoutHead = srcRatio - headRatio;
					innerBodyCnt = withoutHead / targetRatio;
					tailRatio = withoutHead % targetRatio;
					std::swap(tmpBuffHead, tmpBuffTail);
				}
			}
			// tail
			if (bodyEnd == bodyCount) {
				// body + tail
				AddLines(lineReducer, pSrc, tmpBuffHead, params.srcLineOffsetBytes, endBodyCnt+1);
				StoreToTarget(storeCount, pTarget, tmpBuffHead, hvTargetSrcRatio);
				OffsetPtr(pTarget, params.targetLineOffsetBytes);
			}
		}
	}else {
		const uint16_t useParts = min(parts_, blockCount);
		if (useParts <= part) {
			return;
		}
		const uint16_t partLen = blockCount / useParts;
		const uint16_t blockStart = partLen * part;
		const uint16_t blockEnd = (part == useParts-1) ? blockCount : (blockStart + partLen);
		OffsetPtr(pSrc, params.srcLineOffsetBytes * blockStart * srcRatio);
		OffsetPtr(pTarget, params.targetLineOffsetBytes * blockStart * targetRatio);
		for (uint16_t bi=blockStart; bi<blockEnd; ++bi) {
			// head
			{
				// head + body
				ReadLines(lineReducer, pSrc, tmpBuffHead, params.srcLineOffsetBytes, endBodyCnt);
				// srcRatio data straddles targetRatio's tail and next head
				lineReducer.fillRead(pSrc, tmpBuffTail);
				OffsetPtr(pSrc, params.srcLineOffsetBytes);
				headRatio = targetRatio - remainderRatio;
				StoreToTarget(storeCount, pTarget, tmpBuffHead, tmpBuffTail, hRemainderTargetRatio, hvTargetSrcRatio);
				OffsetPtr(pTarget, params.targetLineOffsetBytes);
				std::swap(tmpBuffHead, tmpBuffTail);
			}
			// body
			{
				const uint16_t withoutHead = srcRatio - headRatio;
				uint16_t innerBodyCnt = withoutHead / targetRatio;
				uint16_t tailRatio = withoutHead % targetRatio;
				for (uint16_t i=0; i<bodyCount; ++i) {
					// body
					AddLines(lineReducer, pSrc, tmpBuffHead, params.srcLineOffsetBytes, innerBodyCnt);
					// tail and next head
					lineReducer.fillRead(pSrc, tmpBuffTail);
					OffsetPtr(pSrc, params.srcLineOffsetBytes);
					StoreToTarget(storeCount, pTarget, tmpBuffHead, tmpBuffTail, tailRatio, targetRatio, hvTargetSrcRatio);
					OffsetPtr(pTarget, params.targetLineOffsetBytes);
					
					headRatio = targetRatio - tailRatio;
					const uint16_t withoutHead = srcRatio - headRatio;
					innerBodyCnt = withoutHead / targetRatio;
					tailRatio = withoutHead % targetRatio;
					std::swap(tmpBuffHead, tmpBuffTail);
				}
			}	
			// tail
			{
				// body + tail
				AddLines(lineReducer, pSrc, tmpBuffHead, params.srcLineOffsetBytes, endBodyCnt+1);
				StoreToTarget(storeCount, pTarget, tmpBuffHead, hvTargetSrcRatio);
				OffsetPtr(pTarget, params.targetLineOffsetBytes);
			}
		}
	}
}

void AveragingReducer::Process(uint16_t part)
{
	assert(part < parts_);
	const AveragingReduceParams& params = *pParams_;
	if (params.widthRatioTarget == params.widthRatioSource && params.heightRatioTarget == params.heightRatioSource) {
		const __m128i* pSrc = params.srcBuff;
		__m128i* pTarget = params.targetBuff;
		if (part >= params.srcHeight) {
			return;
		}
		const uint16_t useParts = min(parts_, params.srcHeight);
		const uint16_t partLen = params.srcHeight / useParts;
		const uint16_t yStart = partLen * part;
		const uint16_t yEnd = (part == useParts-1) ? params.srcHeight : (yStart + partLen);
		OffsetPtr(pSrc, params.srcLineOffsetBytes * yStart);
		OffsetPtr(pTarget, params.targetLineOffsetBytes * yStart);
		for (uint16_t y=yStart; y<yEnd; ++y) {
			memcpy(pTarget, pSrc, params.srcWidth*4);
			OffsetPtr(pSrc, params.srcLineOffsetBytes);
			OffsetPtr(pTarget, params.targetLineOffsetBytes);
		}
	}else {
		if ((params.heightRatioSource % params.heightRatioTarget) == 0) {
			Process_Ratio1NX(*pLineAveragingReducer, part);
		}else {
			Process_RatioAny(*pLineAveragingReducer, part);
		}
	}
}

AveragingReducerCaller::AveragingReducerCaller()
	:
	pImpl_(new AveragingReducer())
{
}

AveragingReducerCaller::~AveragingReducerCaller()
{
	delete pImpl_;
}

void AveragingReducerCaller::Setup(const AveragingReduceParams* pParams, uint16_t parts)
{
	pImpl_->Setup(pParams, parts);
}

void AveragingReducerCaller::Process(uint16_t part)
{
	pImpl_->Process(part);
}

}	// namespace intrinsics_sse2_inout4b

}	// namespace gl

