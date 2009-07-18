#include "AveragingReducer_intrinsics_sse2_inout4b.h"

#include "AveragingReducer_intrinsics.h"

#include "common.h"
#include "arrayutil.h"

#ifdef _DEBUG
#include <tchar.h>
#endif

/*
reference

Visual C++ Language Reference Compiler Intrinsics
http://msdn.microsoft.com/ja-jp/library/26td21ds.aspx
http://msdn.microsoft.com/ja-jp/library/x8zs5twb.aspx
http://www.icnet.ne.jp/~nsystem/simd_tobira/index.html

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
	__m128i ints = _mm_cvttpd_epi32(ratios);
	__m128i shorts = _mm_shufflelo_epi16(ints, 0);
	return _mm_unpacklo_epi64(shorts, shorts);
}

void StoreToTarget(
	uint16_t count,
	__m128i* targetBuff,
	const __m128i* tmpBuffBody, const __m128i* tmpBuffEnd,
	uint32_t endRatio, uint32_t targetRatio, __m128i hvTargetSrcRatio
	)
{
	const __m128i hEndTargetRatio = make_vec_ratios(endRatio << 16, targetRatio);
	for (uint16_t i=0; i<count; ++i) {
		__m128i endPixels0 = tmpBuffEnd[i*2+0];
		__m128i endPixels1 = tmpBuffEnd[i*2+1];
		endPixels0 = _mm_mulhi_epu16(endPixels0, hEndTargetRatio);
		endPixels1 = _mm_mulhi_epu16(endPixels1, hEndTargetRatio);
		__m128i twoPixels0 = _mm_adds_epu16(tmpBuffBody[i*2+0], endPixels0);
		__m128i twoPixels1 = _mm_adds_epu16(tmpBuffBody[i*2+1], endPixels1);
		__m128i p0 = _mm_mulhi_epu16(twoPixels0, hvTargetSrcRatio);
		__m128i p1 = _mm_mulhi_epu16(twoPixels1, hvTargetSrcRatio);
		
		// 16 -> 8
		targetBuff[i] = _mm_packus_epi16(p0, p1);
	}
}

void StoreToTarget(
	uint16_t count,
	__m128i* targetBuff,
	const __m128i* tmpBuffBody, const __m128i* tmpBuffHead, const __m128i* tmpBuffTail,
	uint32_t headRatio, uint32_t tailRatio, uint32_t targetRatio, __m128i hvTargetSrcRatio
	)
{
	const __m128i hHeadTargetRatio = make_vec_ratios(headRatio << 16, targetRatio);
	const __m128i hTailTargetRatio = make_vec_ratios(tailRatio << 16, targetRatio);
	for (uint16_t i=0; i<count; ++i) {
		__m128i headPixels0 = tmpBuffHead[i*2+0];
		__m128i headPixels1 = tmpBuffHead[i*2+1];
		headPixels0 = _mm_mulhi_epu16(headPixels0, hHeadTargetRatio);
		headPixels1 = _mm_mulhi_epu16(headPixels1, hHeadTargetRatio);
		
		__m128i tailPixels0 = tmpBuffTail[i*2+0];
		__m128i tailPixels1 = tmpBuffTail[i*2+1];
		tailPixels0 = _mm_mulhi_epu16(tailPixels0, hTailTargetRatio);
		tailPixels1 = _mm_mulhi_epu16(tailPixels1, hTailTargetRatio);
		
		__m128i bodyPixels0 = tmpBuffBody[i*2+0];
		__m128i bodyPixels1 = tmpBuffBody[i*2+1];
		
		__m128i twoPixels0 = _mm_adds_epu16(_mm_adds_epu16(headPixels0, bodyPixels0), tailPixels0);
		__m128i twoPixels1 = _mm_adds_epu16(_mm_adds_epu16(headPixels1, bodyPixels1), tailPixels1);
		
		__m128i p0 = _mm_mulhi_epu16(twoPixels0, hvTargetSrcRatio);
		__m128i p1 = _mm_mulhi_epu16(twoPixels1, hvTargetSrcRatio);
		
		// 16 -> 8
		__m128i result = _mm_packus_epi16(p0, p1);
		targetBuff[i] = result;
	}
}

void StoreToTarget(
	uint16_t count,
	__m128i* targetBuff,
	const __m128i* tmpBuffHead, const __m128i* tmpBuffTail,
	uint32_t headRatio, uint32_t tailRatio, uint32_t targetRatio, __m128i hvTargetSrcRatio
	)
{
	const __m128i hHeadTargetRatio = make_vec_ratios(headRatio << 16, targetRatio);
	const __m128i hTailTargetRatio = make_vec_ratios(tailRatio << 16, targetRatio);
	for (uint16_t i=0; i<count; ++i) {
		__m128i headPixels0 = tmpBuffHead[i*2+0];
		__m128i headPixels1 = tmpBuffHead[i*2+1];
		headPixels0 = _mm_mulhi_epu16(headPixels0, hHeadTargetRatio);
		headPixels1 = _mm_mulhi_epu16(headPixels1, hHeadTargetRatio);
		
		__m128i tailPixels0 = tmpBuffTail[i*2+0];
		__m128i tailPixels1 = tmpBuffTail[i*2+1];
		tailPixels0 = _mm_mulhi_epu16(tailPixels0, hTailTargetRatio);
		tailPixels1 = _mm_mulhi_epu16(tailPixels1, hTailTargetRatio);
		
		__m128i twoPixels0 = _mm_adds_epu16(headPixels0, tailPixels0);
		__m128i twoPixels1 = _mm_adds_epu16(headPixels1, tailPixels1);
		
		__m128i p0 = _mm_mulhi_epu16(twoPixels0, hvTargetSrcRatio);
		__m128i p1 = _mm_mulhi_epu16(twoPixels1, hvTargetSrcRatio);
		
		// 16 -> 8
		__m128i result = _mm_packus_epi16(p0, p1);
		targetBuff[i] = result;
	}
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
__forceinline __m128i Scale2(const __m128i* pSrc)
{
	__m128i fourPixels0 = pSrc[0];

/*
	12345678abcdefgh
	↓
	1234abcd5678efgh
	↓
	 1 2 3 4 a b c d
	        +
	 5 6 7 8 e f g h
*/
	__m128i shuffled0 = _mm_shuffle_epi32(fourPixels0, _MM_SHUFFLE(3, 1, 2, 0));
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

// [横2加算, 1]
/*
123456789abcdefg
↓
12349abc5678defg
↓
 1 2 3 4 9 a b c
        +
 5 6 7 8 d e f g
*/
__forceinline __m128i Split_2_1(__m128i src)
{
	__m128i shuffled = _mm_shuffle_epi32(src, _MM_SHUFFLE(3, 1, 2, 0));
	__m128i twoPixels0 = _mm_unpacklo_epi8(shuffled, _mm_setzero_si128());
	__m128i twoPixels1 = _mm_unpackhi_epi8(shuffled, _mm_setzero_si128());
	twoPixels1 = _mm_move_epi64(twoPixels1);
	__m128i result = _mm_add_epi16(twoPixels0, twoPixels1);
	
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

__forceinline __m128i CollectSum(const __m128i* pSrc, const uint16_t count)
{
	assert(count > 0);
	switch (count) {
	case 1:
		return CollectSum_1(pSrc);
	case 2:
		return CollectSum_2(pSrc);
	case 3:
		return CollectSum_3(pSrc);
	case 4:
		return CollectSum_4(pSrc);
	default:
		{
			__m128i ret = CollectSum_4(pSrc);
			++pSrc;
			const uint16_t n4 = count / 4;
			const uint16_t remain = count % 4;
			for (uint16_t i=1; i<n4; ++i) {
				ret = _mm_adds_epu16(ret, CollectSum_4(pSrc));
				++pSrc;
			}
			switch (remain) {
			case 1:
				ret = _mm_adds_epu16(ret, CollectSum_1(pSrc));
				break;
			case 2:
				ret = _mm_adds_epu16(ret, CollectSum_2(pSrc));
				break;
			case 3:
				ret = _mm_adds_epu16(ret, CollectSum_3(pSrc));
				break;
			}
			return ret;
		}
	}
}

class LineAveragingReducer_Ratio_2_3 : public ILineAveragingReducer
{
public:
	ILineAveragingReducer_Impl;
	
	template <typename T>
	__forceinline void iterate(const __m128i* srcBuff, __m128i* tmpBuff)
	{
		const __m128i* pSrc = srcBuff;
		__m128i* pTmp = tmpBuff;
		
		const uint16_t loopLen = srcWidth / 12;
		const uint16_t remain = srcWidth % 12;
		
		// read 12 pixels write 8 pixels per loop
		for (uint16_t i=0; i<loopLen; ++i) {
			__m128i sp0123 = pSrc[0];
			__m128i sp01 = _mm_unpacklo_epi8(sp0123, _mm_setzero_si128());
			__m128i sp23 = _mm_unpackhi_epi8(sp0123, _mm_setzero_si128());
			__m128i sp1 = _mm_srli_si128(sp01, 8);
			__m128i sp1half = _mm_srli_epi16(sp1, 1);
			__m128i sp1rest = _mm_sub_epi16(sp1, sp1half);
			__m128i tp0 = _mm_add_epi16(sp01, sp1half);
			__m128i tp1 = _mm_add_epi16(sp1rest, sp23);
			pTmp[0] = T::work(pTmp[0], _mm_unpacklo_epi64(tp0, tp1));

			__m128i sp4567 = pSrc[1];
			__m128i sp45 = _mm_unpacklo_epi8(sp4567, _mm_setzero_si128());
			__m128i sp4 = _mm_slli_si128(sp45, 8);
			__m128i sp4half = _mm_srli_epi16(sp4, 1);
			__m128i sp4rest = _mm_sub_epi16(sp4, sp4half);
			__m128i tp2 = _mm_add_epi16(sp23, sp4half);
			__m128i tp3 = _mm_add_epi16(sp4rest, sp45);
			pTmp[1] = T::work(pTmp[1], _mm_unpackhi_epi64(tp2, tp3));
			
			__m128i sp89ab = pSrc[2];
			__m128i sp67 = _mm_unpackhi_epi8(sp4567, _mm_setzero_si128());
			__m128i sp89 = _mm_unpacklo_epi8(sp89ab, _mm_setzero_si128());
			__m128i sp7 = _mm_srli_si128(sp67, 8);
			__m128i sp7half = _mm_srli_epi16(sp7, 1);
			__m128i sp7rest = _mm_sub_epi16(sp7, sp7half);
			__m128i tp4 = _mm_add_epi16(sp67, sp7half);
			__m128i tp5 = _mm_add_epi16(sp7rest, sp89);
			pTmp[2] = T::work(pTmp[2], _mm_unpacklo_epi64(tp4, tp5));
			
			__m128i spab = _mm_unpackhi_epi8(sp89ab, _mm_setzero_si128());
			__m128i spa = _mm_slli_si128(spab, 8);
			__m128i spahalf = _mm_srli_epi16(spa, 1);
			__m128i sparest = _mm_sub_epi16(spa, spahalf);
			__m128i tp6 = _mm_add_epi16(sp89, spahalf);
			__m128i tp7 = _mm_add_epi16(sparest, spab);
			pTmp[3] = T::work(pTmp[3], _mm_unpackhi_epi64(tp6, tp7));

			pSrc += 3;
			pTmp += 4;
		}
		
		const uint16_t remainLoopLen = remain / 3;
		const uint16_t remainRemain = remain % 3;

		// read 3 pixels write 2 pixels per loop
		for (uint16_t i=0; i<remainLoopLen; ++i) {
			__m128i sp01 = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
			OffsetPtr(pSrc, 8);
			__m128i sp23 = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
			OffsetPtr(pSrc, 4);
			__m128i sp1 = _mm_srli_si128(sp01, 8);
			__m128i sp1half = _mm_srli_epi16(sp1, 1);
			__m128i sp1rest = _mm_sub_epi16(sp1, sp1half);
			__m128i tp0 = _mm_add_epi16(sp01, sp1half);
			__m128i tp1 = _mm_add_epi16(sp1rest, sp23);
			_mm_storeu_si128(pTmp, T::work(_mm_loadu_si128(pTmp), _mm_unpacklo_epi64(tp0, tp1)));

		}

	}
	
	uint16_t srcWidth;
};

class LineAveragingReducer_RatioAny_Base : public ILineAveragingReducer
{
public:
	LineAveragingReducer_RatioAny_Base(const uint8_t* pBodyCounts, const uint16_t* pTailTargetRatios)
		:
		pBodyCounts(pBodyCounts),
		pTailTargetRatios(pTailTargetRatios)
	{
	}
	
	uint16_t targetRatio_;
	uint16_t srcRatio_;
	uint16_t srcWidth_;
	const uint8_t* pBodyCounts;
	const uint16_t* pTailTargetRatios;
};

__forceinline __m128i set1_epi16_low(int v)
{
	return _mm_shufflelo_epi16(_mm_cvtsi32_si128(v), 0);
}


class CollectSumAndNext1
{
public:
	__forceinline static void work(const __m128i* pSrc, const uint16_t count, __m128i& sum, __m128i& one)
	{
		const uint16_t lowBits = count & 0x03;
		const uint16_t highBits = count & ~0x03;
		if (highBits) {
				sum = CollectSum_4(pSrc);
				++pSrc;
				const uint16_t n4 = highBits >> 2;
				const uint16_t remain = lowBits;
				for (uint16_t i=1; i<n4; ++i) {
					sum = _mm_adds_epu16(sum, CollectSum_4(pSrc));
					++pSrc;
				}
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
				}

		}else {
			switch (lowBits) {
			case 1:
		//		sum = _mm_cvtepu8_epi16(_mm_loadl_epi64(pSrc));
				sum = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
				one = _mm_srli_si128(sum, 8);
				break;
			case 2:
				{
					__m128i src = load_unaligned_128(pSrc);
					__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
					sum = _mm_add_epi16(twoPixels0, _mm_srli_si128(twoPixels0, 8));
					one = _mm_unpackhi_epi8(src, _mm_setzero_si128());
				}
				break;
			case 3:
				sum = Split_3_1(load_unaligned_128(pSrc));
				one = _mm_srli_si128(sum, 8);
				break;
			}
		}
	}
};

class CollectSumAndNext1_1
{
public:
	__forceinline static void work(const __m128i* pSrc, const uint8_t count, __m128i& sum, __m128i& one)
	{
		switch (count) {
		case 0:
			one = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*(const int*)pSrc), _mm_setzero_si128());
			break;
		case 1:
	//		sum = _mm_cvtepu8_epi16(_mm_loadl_epi64(pSrc));
			sum = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
			one = _mm_srli_si128(sum, 8);
			break;
		}
	}
};

class CollectSumAndNext1_2
{
public:
	__forceinline static void work(const __m128i* pSrc, const uint8_t count, __m128i& sum, __m128i& one)
	{
		switch (count) {
		case 1:
	//		sum = _mm_cvtepu8_epi16(_mm_loadl_epi64(pSrc));
			sum = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
			one = _mm_srli_si128(sum, 8);
			break;
		case 2:
			{
				__m128i src = load_unaligned_128(pSrc);
				__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
				sum = _mm_add_epi16(twoPixels0, _mm_srli_si128(twoPixels0, 8));
				one = _mm_unpackhi_epi8(src, _mm_setzero_si128());
			}
			break;
		}
	}
};

class CollectSumAndNext1_3
{
public:
	__forceinline static void work(const __m128i* pSrc, const uint8_t count, __m128i& sum, __m128i& one)
	{
		switch (count) {
		case 2:
			{
				__m128i src = load_unaligned_128(pSrc);
				__m128i twoPixels0 = _mm_unpacklo_epi8(src, _mm_setzero_si128());
				sum = _mm_add_epi16(twoPixels0, _mm_srli_si128(twoPixels0, 8));
				one = _mm_unpackhi_epi8(src, _mm_setzero_si128());
			}
			break;
		case 3:
			sum = Split_3_1(load_unaligned_128(pSrc));
			one = _mm_srli_si128(sum, 8);
			break;
		}
	}
};

class CollectSumAndNext1_4
{
public:
	__forceinline static void work(const __m128i* pSrc, const uint8_t count, __m128i& sum, __m128i& one)
	{
		switch (count) {
		case 3:
			sum = Split_3_1(load_unaligned_128(pSrc));
			one = _mm_srli_si128(sum, 8);
			break;
		case 4:
			sum = CollectSum_4(pSrc);
			++pSrc;
			one = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*(const int*)pSrc), _mm_setzero_si128());
			break;
		}
	}
};

class CollectSumAndNext1_X
{
public:
	__forceinline static void work(const __m128i* pSrc, const uint16_t count, __m128i& sum, __m128i& one)
	{
		sum = CollectSum_4(pSrc);
		++pSrc;
		const uint16_t n4 = count >> 2;
		for (uint16_t i=1; i<n4; ++i) {
			sum = _mm_adds_epu16(sum, CollectSum_4(pSrc));
			++pSrc;
		}
		switch (count & 0x03) {
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
		}
	}
};

class LineAveragingReducer_RatioAny_Basic : public LineAveragingReducer_RatioAny_Base
{
public:
	ILineAveragingReducer_Impl;
	uint8_t maxBodyCount;

	LineAveragingReducer_RatioAny_Basic(const uint8_t* pBodyCounts, const uint16_t* pTailTargetRatios)
		:
		LineAveragingReducer_RatioAny_Base(pBodyCounts, pTailTargetRatios)
	{
	}

	template <typename T, typename CollectSumAndNext1T>
	void iterate2(const __m128i* srcBuff, __m128i* tmpBuff)
	{
		const __m128i* pSrc = srcBuff;
		__m128i* pTmp = tmpBuff;
		
		const uint16_t srcRatio = srcRatio_;
		const uint16_t targetRatio = targetRatio_;
		const uint16_t srcWidth = srcWidth_;
		const uint16_t endLoopCnt = srcRatio / targetRatio;
		const uint16_t remainder = srcRatio % targetRatio;
		const __m128i remainderDividedByTargetRatio = make_vec_ratios(remainder << 16, targetRatio);

		const uint16_t blockCount = srcWidth / srcRatio;
		for (uint16_t blockIdx=0; blockIdx<blockCount; ++blockIdx) {
			__m128i col2;
			// head
			{
				__m128i col;
				CollectSumAndNext1::work(pSrc, endLoopCnt, col, col2);
				OffsetPtr(pSrc, (endLoopCnt+1) * 4);
				// srcRatio data straddles targetRatio's tail and next head
				__m128i col2a = _mm_mulhi_epu16(col2, remainderDividedByTargetRatio);
				col = _mm_add_epi16(col, col2a);
				col2 = _mm_sub_epi16(col2, col2a);
				_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
				OffsetPtr(pTmp, 8);
			}
			// body
			{
				const uint16_t bodyLoopCount = targetRatio - 2;
#if 0
				// loop unrolled version
				const uint16_t loopCount = bodyLoopCount / 4;
				const uint16_t remain = bodyLoopCount % 4;
				const uint32_t* pIntBodyCounts = (const uint32_t*) pBodyCounts;
				const __m128i* pVecTailTargetRatios = (const __m128i*)pTailTargetRatios;
				for (uint16_t i=0; i<loopCount; ++i) {
					const uint32_t bodyCounts = pIntBodyCounts[i];
					__m128i tailTargetRatios = _mm_loadl_epi64(pVecTailTargetRatios);
					OffsetPtr(pVecTailTargetRatios, 8);
					__m128i collected;
					uint8_t bodyCount;
					__m128i tailTargetRatio;
					__m128i col = col2;
					__m128i col2a;

					/// 1
					bodyCount = bodyCounts & 0xFF;
					tailTargetRatio = _mm_shufflelo_epi16(tailTargetRatios, _MM_SHUFFLE(0,0,0,0));
					CollectSumAndNext1T::work(pSrc, bodyCount, collected, col2);
					OffsetPtr(pSrc, (bodyCount+1) * 4);
					col = _mm_add_epi16(col, collected);
					col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
					col = _mm_add_epi16(col, col2a);
					_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
					OffsetPtr(pTmp, 8);
					col = _mm_sub_epi16(col2, col2a);
					/// 2
					bodyCount = (bodyCounts >> 8) & 0xFF;
					tailTargetRatio = _mm_shufflelo_epi16(tailTargetRatios, _MM_SHUFFLE(1,1,1,1));
					CollectSumAndNext1T::work(pSrc, bodyCount, collected, col2);
					OffsetPtr(pSrc, (bodyCount+1) * 4);
					col = _mm_add_epi16(col, collected);
					col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
					col = _mm_add_epi16(col, col2a);
					_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
					OffsetPtr(pTmp, 8);
					col = _mm_sub_epi16(col2, col2a);
					/// 3
					bodyCount = (bodyCounts >> 16) & 0xFF;
					tailTargetRatio = _mm_shufflelo_epi16(tailTargetRatios, _MM_SHUFFLE(2,2,2,2));
					CollectSumAndNext1T::work(pSrc, bodyCount, collected, col2);
					OffsetPtr(pSrc, (bodyCount+1) * 4);
					col = _mm_add_epi16(col, collected);
					col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
					col = _mm_add_epi16(col, col2a);
					_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
					OffsetPtr(pTmp, 8);
					col = _mm_sub_epi16(col2, col2a);
					/// 4
					bodyCount = bodyCounts >> 24;
					tailTargetRatio = _mm_shufflelo_epi16(tailTargetRatios, _MM_SHUFFLE(3,3,3,3));
					CollectSumAndNext1T::work(pSrc, bodyCount, collected, col2);
					OffsetPtr(pSrc, (bodyCount+1) * 4);
					col = _mm_add_epi16(col, collected);
					col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
					col = _mm_add_epi16(col, col2a);
					_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
					OffsetPtr(pTmp, 8);
					col2 = _mm_sub_epi16(col2, col2a);

				}
				for (uint16_t i=0; i<remain; ++i) {
					const uint16_t bodyCount = pBodyCounts[loopCount*4+i];
					__m128i collected;
					__m128i col = col2; // head
					CollectSumAndNext1T::work(pSrc, bodyCount, collected, col2);
					col = _mm_add_epi16(col, collected);
					OffsetPtr(pSrc, (bodyCount+1) * 4);
					__m128i tailTargetRatio = set1_epi16_low(pTailTargetRatios[loopCount*4+i]);
					__m128i col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
					col = _mm_add_epi16(col, col2a);
					_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
					OffsetPtr(pTmp, 8);
					col2 = _mm_sub_epi16(col2, col2a);
				}
#else
				for (uint16_t i=0; i<bodyLoopCount; ++i) {
					const uint16_t bodyCount = pBodyCounts[i];
					__m128i collected;
					__m128i col = col2; // head
					CollectSumAndNext1T::work(pSrc, bodyCount, collected, col2);
					col = _mm_add_epi16(col, collected);
					OffsetPtr(pSrc, (bodyCount+1) * 4);
					__m128i tailTargetRatio = set1_epi16_low(pTailTargetRatios[i]);
					__m128i col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
					col = _mm_add_epi16(col, col2a);
					_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
					OffsetPtr(pTmp, 8);
					col2 = _mm_sub_epi16(col2, col2a);
				}
#endif
			}
			// tail
			{
				__m128i col = col2;
				col = _mm_add_epi16(col, CollectSum(pSrc, endLoopCnt));
				OffsetPtr(pSrc, endLoopCnt * 4);
				_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
				OffsetPtr(pTmp, 8);
			}
		}
	}
	
	template <typename T>
	__forceinline void iterate(const __m128i* srcBuff, __m128i* tmpBuff)
	{
		switch (maxBodyCount) {
		case 4:
			iterate2<T, CollectSumAndNext1_4>(srcBuff, tmpBuff);
			break;
		case 3:
			iterate2<T, CollectSumAndNext1_3>(srcBuff, tmpBuff);
			break;
		case 2:
			iterate2<T, CollectSumAndNext1_2>(srcBuff, tmpBuff);
			break;
		case 1:
			iterate2<T, CollectSumAndNext1_1>(srcBuff, tmpBuff);
			break;
		default:
			iterate2<T, CollectSumAndNext1_X>(srcBuff, tmpBuff);
			break;
		}
	}

};

class LineAveragingReducer_RatioAny_MoreThanHalf : public LineAveragingReducer_RatioAny_Base
{
public:
	ILineAveragingReducer_Impl;

	LineAveragingReducer_RatioAny_MoreThanHalf(const uint8_t* pBodyCounts, const uint16_t* pTailTargetRatios)
		:
		LineAveragingReducer_RatioAny_Base(pBodyCounts, pTailTargetRatios)
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
		const uint16_t remainder = srcRatio - targetRatio;
		const __m128i remainderDividedByTargetRatio = make_vec_ratios(remainder << 16, targetRatio);

		const uint16_t blockCount = srcWidth/srcRatio;
		for (uint16_t blockIdx=0; blockIdx<blockCount; ++blockIdx) {
			__m128i col2;
			// head
			{
				__m128i col;
				CollectSumAndNext1_2::work(pSrc, 1, col, col2);
				OffsetPtr(pSrc, 8);
				// srcRatio data straddles targetRatio's tail and next head
				__m128i col2a = _mm_mulhi_epu16(col2, remainderDividedByTargetRatio);
				col = _mm_add_epi16(col, col2a);
				col2 = _mm_sub_epi16(col2, col2a);
				_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
				OffsetPtr(pTmp, 8);
			}
			// body
			{
				const uint16_t limit = targetRatio - 2;
				const uint16_t loopCount = limit >> 1;
				const uint16_t* pIntBodyCounts = (const uint16_t*) pBodyCounts;
				const uint32_t* pIntTailTargetRatios = (const uint32_t*) pTailTargetRatios;
				for (uint16_t i=0; i<loopCount; ++i) {
					const __m128i ratiosOrg = _mm_cvtsi32_si128(pIntTailTargetRatios[i]);
					const __m128i ratiosTmp = _mm_shufflelo_epi16(ratiosOrg, _MM_SHUFFLE(1,1,0,0));
					const __m128i ratios = _mm_shuffle_epi32(ratiosTmp, _MM_SHUFFLE(1,1,0,0));
					const uint16_t bodyCountPair = pIntBodyCounts[i];
					switch (bodyCountPair) {
					case 0:
						{
							__m128i src = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
							OffsetPtr(pSrc, 8);
							__m128i multiplied = _mm_mulhi_epu16(src, ratios);
							__m128i subtracted = _mm_sub_epi16(src, multiplied);
							__m128i colFirst = _mm_add_epi16(col2, multiplied);
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), colFirst));
							OffsetPtr(pTmp, 8);
							__m128i colSecond = _mm_add_epi16(subtracted, _mm_srli_si128(multiplied, 8));
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), colSecond));
							OffsetPtr(pTmp, 8);
							col2 = _mm_srli_si128(subtracted, 8);
						}
						break;
					case 1:
						{
							__m128i src = load_unaligned_128(pSrc);
							OffsetPtr(pSrc, 12);
							__m128i collected = _mm_unpacklo_epi8(src, _mm_setzero_si128());
							__m128i remain = _mm_unpacklo_epi8(_mm_srli_si128(src,4), _mm_setzero_si128());
							__m128i multiplied = _mm_mulhi_epu16(remain, ratios);	// col2 * tail/target
							__m128i subtracted = _mm_sub_epi16(remain, multiplied);
							__m128i col = _mm_add_epi16(col2, collected);
							__m128i colFirst = _mm_add_epi16(col, multiplied);
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), colFirst));
							OffsetPtr(pTmp, 8);
							__m128i colSecond = _mm_add_epi16(subtracted, _mm_srli_si128(multiplied,8));
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), colSecond));
							OffsetPtr(pTmp, 8);
							col2 = _mm_srli_si128(subtracted,8);
						}
						break;
					case 0x100:
						{
							__m128i src = load_unaligned_128(pSrc);
							OffsetPtr(pSrc, 12);
							__m128i shuffled = _mm_shuffle_epi32(src, _MM_SHUFFLE(2,0,1,1));
							__m128i collecteds = _mm_unpacklo_epi8(shuffled, _mm_setzero_si128());
							__m128i nexts = _mm_unpackhi_epi8(shuffled, _mm_setzero_si128());
							__m128i multiplied = _mm_mulhi_epu16(nexts, ratios);	// col2 * tail/target
							__m128i subtracted = _mm_sub_epi16(nexts, multiplied);
							__m128i colFirst = _mm_add_epi16(col2, multiplied);
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), colFirst));
							OffsetPtr(pTmp, 8);
							__m128i sums = _mm_add_epi16(collecteds, multiplied);
							__m128i colSecond = _mm_add_epi16(subtracted, _mm_srli_si128(sums, 8));
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), colSecond));
							OffsetPtr(pTmp, 8);
							col2 = _mm_srli_si128(subtracted, 8);
						}
						break;
					case 0x101:
						{
							__m128i src = load_unaligned_128(pSrc);
							++pSrc;
							__m128i shuffled = _mm_shuffle_epi32(src, _MM_SHUFFLE(3,1,2,0));
							__m128i collecteds = _mm_unpacklo_epi8(shuffled, _mm_setzero_si128());
							__m128i nexts = _mm_unpackhi_epi8(shuffled, _mm_setzero_si128());
							__m128i multiplied = _mm_mulhi_epu16(nexts, ratios);	// col2 * tail/target
							__m128i subtracted = _mm_sub_epi16(nexts, multiplied);
							__m128i sums = _mm_add_epi16(collecteds, multiplied);
							__m128i colFirst = _mm_add_epi16(col2, sums);
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), colFirst));
							OffsetPtr(pTmp, 8);
							__m128i colSecond = _mm_add_epi16(subtracted, _mm_srli_si128(sums, 8));
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), colSecond));
							OffsetPtr(pTmp, 8);
							col2 = _mm_srli_si128(subtracted, 8);
						}
						break;
					}
				}
				if (limit & 1) {
					const uint8_t bodyCount = pBodyCounts[limit-1];
					__m128i col = col2; // head
					col2 = _mm_unpacklo_epi8(_mm_loadl_epi64(pSrc), _mm_setzero_si128());
					if (bodyCount) {
						col = _mm_add_epi16(col, col2);
						col2 = _mm_srli_si128(col2, 8);
					}
					OffsetPtr(pSrc, (1+bodyCount)*4);
					__m128i tailTargetRatio = set1_epi16_low(pTailTargetRatios[limit-1]);
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
			// eat 4 src pixels at once and produce 2 tmp pixels
			{
				const uint16_t loopCount = srcWidth / 4;
				const uint16_t loopRemain = srcWidth % 4;
				const uint16_t remainLoopCount = loopRemain / 2;
				const uint16_t remainRemain = loopRemain % 2;
				for (uint16_t i=0; i<loopCount; ++i) {
					*pTmp = T::work(*pTmp, Scale2(pSrc));
					++pSrc;
					++pTmp;
				}
				for (uint16_t i=0; i<remainLoopCount; ++i) {
					_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), Split_2_0(_mm_loadl_epi64(pSrc))));
					OffsetPtr(pSrc, 8);
					OffsetPtr(pTmp, 8);
				}
				// TODO: remaining 1 pixels
			}
			break;
		case 3:
			{
				// eat 12 src pixels at once and produce 4 tmp pixels
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
				// eat 8 src pixels at once and produce 2 tmp pixels
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
				const uint16_t remainder = srcRatio % 4;
				switch (remainder) {
				case 0:
					{
						const uint16_t stride = 8 * quotient;
						const uint16_t loopCount = srcWidth / stride;
						const uint16_t remainCount = srcWidth % stride;
						const uint16_t remainLoopCount = remainCount / srcRatio;
						const uint16_t remainRemain = remainCount % srcRatio;
						for (uint16_t i=0; i<loopCount; ++i) {
							__m128i tmp;
							// left
							__m128i left = _mm_setzero_si128();
							left = Collect_4(*pSrc);
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								left = _mm_add_epi16(left, tmp);
							}
							// right
							__m128i right = _mm_setzero_si128();
							right = Collect_4(*pSrc);
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								right = _mm_add_epi16(right, tmp);
							}
							right = _mm_slli_si128(right, 8);
							// set
							__m128i result = _mm_add_epi16(left, right);
							*pTmp = T::work(*pTmp, result);
							++pTmp;
						}
						for (uint16_t i=0; i<remainLoopCount; ++i) {
							__m128i col = Collect_4(*pSrc);
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								col = _mm_add_epi16(col, Collect_4(*pSrc));
								++pSrc;
							}
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
							left = Collect_4(*pSrc);
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								left = _mm_add_epi16(left, tmp);
							}
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
							right = Collect_4(*pSrc);
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								right = _mm_add_epi16(right, tmp);
							}
							result = _mm_add_epi16(left, straddling3);
							right = _mm_slli_si128(right, 8);
							result = _mm_add_epi16(result, right);
							*pTmp = T::work(*pTmp, result);
							++pTmp;
						}
						for (uint16_t i=0; i<remainLoopCount; ++i) {
							__m128i col = Collect_4(_mm_loadu_si128(pSrc));
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								col = _mm_add_epi16(col, Collect_4(_mm_loadu_si128(pSrc)));
								++pSrc;
							}
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
							left = Collect_4(*pSrc);
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								left = _mm_add_epi16(left, tmp);
							}
							straddling = Split_2_2(*pSrc);
							++pSrc;
							// right
							right = Collect_4(*pSrc);
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								right = _mm_add_epi16(right, tmp);
							}

							result = _mm_add_epi16(left, straddling);
							right = _mm_slli_si128(right, 8);
							result = _mm_add_epi16(result, right);
							*pTmp = T::work(*pTmp, result);
							++pTmp;
						}
						assert(remainLoopCount < 2);
						if (remainLoopCount) {
							__m128i col = Collect_4(*pSrc);
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								col = _mm_add_epi16(col, Collect_4(*pSrc));
								++pSrc;
							}
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
							left = Collect_4(*pSrc);
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								left = _mm_add_epi16(left, tmp);
							}
							straddling1 = Split_3_1(*pSrc);
							++pSrc;
							// 2 right
							right = Collect_4(*pSrc);
							++pSrc;
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
							tmp = Collect_4(*pSrc);
							++pSrc;
							left = _mm_add_epi16(left, tmp);
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								left = _mm_add_epi16(left, tmp);
							}
							straddling3 = Split_1_3(*pSrc);
							++pSrc;
							// 4 right
							right = Collect_4(*pSrc);
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								tmp = Collect_4(*pSrc);
								++pSrc;
								right = _mm_add_epi16(right, tmp);
							}
							result = _mm_add_epi16(left, straddling3);
							right = _mm_slli_si128(right, 8);
							result = _mm_add_epi16(result, right);
							*pTmp = T::work(*pTmp, result);
							++pTmp;
						}
						for (uint16_t i=0; i<remainLoopCount; ++i) {
							__m128i col = Collect_4(_mm_loadu_si128(pSrc));
							++pSrc;
							for (uint16_t j=0; j<quotient-1u; ++j) {
								col = _mm_add_epi16(col, Collect_4(_mm_loadu_si128(pSrc)));
								++pSrc;
							}
							col = _mm_add_epi16(col, CollectSum_3(pSrc));
							OffsetPtr(pSrc, 12);
							_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
							OffsetPtr(pTmp, 8);
						}
						// TODO: remaining 1 to srcRatio-1 pixels
					}
					break;
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
	static uint16_t RatioAny_PreCalculate(
		uint8_t* bodyCounts,
		uint8_t& maxBodyCount,
		uint16_t* tailTargetRatios,
		uint16_t srcRatio,
		uint16_t targetRatio
		)
	{
		const uint16_t remainder = srcRatio % targetRatio;
		uint16_t tailRatio = remainder;
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
#ifdef _DEBUG
		static TCHAR buff[20480];
		buff[0] = 0;
		TCHAR buff2[32];
		for (uint16_t i=0; i<limit; ++i) {
			_tcscat(buff, _itot(bodyCounts[i], buff2, 10));
			_tcscat(buff, _T(" "));
		}
	//	TRACE("(%04d %04d) %s\n", srcRatio, targetRatio, buff);
#endif
		return bodyCountSum;
	}

	void Process_Ratio1NX(ILineAveragingReducer& lineReducer, uint16_t part);
	void Process_RatioAny(ILineAveragingReducer& lineReducer, uint16_t part);
	
	const AveragingReduceParams* pParams;
	uint16_t parts;
	static const uint16_t TABLE_COUNT = 8192;
	uint8_t bodyCounts[ TABLE_COUNT ];
	uint16_t tailTargetRatios[ TABLE_COUNT ];
	
	ILineAveragingReducer* pLineAveragingReducer;
	LineAveragingReducer_Ratio1NX lar_Ratio1NX;
	LineAveragingReducer_Ratio_2_3 lar_Ratio_2_3;
	LineAveragingReducer_RatioAny_Basic lar_RatioAny_Basic;
	LineAveragingReducer_RatioAny_MoreThanHalf lar_RatioAny_MoreThanHalf;
};

AveragingReducer::AveragingReducer()
	:
	lar_RatioAny_Basic(bodyCounts, tailTargetRatios),
	lar_RatioAny_MoreThanHalf(bodyCounts, tailTargetRatios)
{
}

void AveragingReducer::Setup(const AveragingReduceParams* pParams, uint16_t parts)
{
	assert(pParams);
	this->pParams = pParams;
	this->parts = parts;
	
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
	}else if (params.widthRatioTarget == 2 && params.widthRatioSource == 3) {
		lar_Ratio_2_3.srcWidth = params.srcWidth;
		pLineAveragingReducer = &lar_Ratio_2_3;
	}else {
		uint8_t maxBodyCount;
		RatioAny_PreCalculate(bodyCounts, maxBodyCount, tailTargetRatios, params.widthRatioSource, params.widthRatioTarget);
		if (params.widthRatioSource > params.widthRatioTarget * 2) {
			lar_RatioAny_Basic.srcRatio_ = params.widthRatioSource;
			lar_RatioAny_Basic.targetRatio_ = params.widthRatioTarget;
			lar_RatioAny_Basic.srcWidth_ = params.srcWidth;
			lar_RatioAny_Basic.maxBodyCount = maxBodyCount;
			pLineAveragingReducer = &lar_RatioAny_Basic;
		}else {
			lar_RatioAny_MoreThanHalf.srcRatio_ = params.widthRatioSource;
			lar_RatioAny_MoreThanHalf.targetRatio_ = params.widthRatioTarget;
			lar_RatioAny_MoreThanHalf.srcWidth_ = params.srcWidth;
			pLineAveragingReducer = &lar_RatioAny_MoreThanHalf;
		}
	}
}

template <typename LineReducerT>
void Read(LineReducerT& lineReducer, const __m128i*& pSrc, __m128i* pTarget, ptrdiff_t offsetBytes, uint16_t addReadCount)
{
	lineReducer.fillRead(pSrc, pTarget);
	OffsetPtr(pSrc, offsetBytes);
	for (uint16_t i=0; i<addReadCount; ++i) {
		lineReducer.addRead(pSrc, pTarget);
		OffsetPtr(pSrc, offsetBytes);
	}
}

// 縦 1:N
void AveragingReducer::Process_Ratio1NX(ILineAveragingReducer& lineReducer, uint16_t part)
{
	const AveragingReduceParams& params = *pParams;
	
	const uint32_t xLoopCount = getTargetLineArrayCount(*pParams);
	const uint16_t yLoopCount = params.srcHeight / params.heightRatioSource;
	__m128i* targetLine = params.targetBuff;
	const __m128i* srcLine = params.srcBuff;
	__m128i* tmpBuff = params.tmpBuff;
	const __m128i invertRatioSource = make_vec_ratios(params.widthRatioTarget << 16, params.widthRatioSource * params.heightRatioSource);
	
	if (part >= yLoopCount) {
		return;
	}
	const uint16_t useParts = min(parts, yLoopCount);
	const uint16_t partLen = yLoopCount / useParts;
	const uint16_t yStart = partLen * part;
	const uint16_t yEnd = (part == useParts-1) ? yLoopCount : (yStart + partLen);
	OffsetPtr(srcLine, params.srcLineOffsetBytes * yStart * params.heightRatioSource);
	OffsetPtr(targetLine, params.targetLineOffsetBytes * yStart);
	tmpBuff += xLoopCount * 2 * part;
	
	for (uint16_t y=yStart; y<yEnd; ++y) {
		// 最初のライン set to temp
		// 次のラインから最後の前のラインまで plusEqual to temp
		Read(lineReducer, srcLine, tmpBuff, params.srcLineOffsetBytes, params.heightRatioSource-1u);
		// store
		for (uint16_t x=0; x<xLoopCount; ++x) {
			__m128i p0 = tmpBuff[x*2+0];
			__m128i p1 = tmpBuff[x*2+1];
			p0 = _mm_mulhi_epu16(p0, invertRatioSource);
			p1 = _mm_mulhi_epu16(p1, invertRatioSource);
			// 16 -> 8
			targetLine[x] = _mm_packus_epi16(p0, p1);
		}
		OffsetPtr(targetLine, params.targetLineOffsetBytes);
	}
	const uint16_t remain = params.srcHeight % params.heightRatioSource;
}

// 縦 自由比率縮小
// thread化するとしたら、bodyのtargetRatioのloopを分割か。。
// targetRatioが2の場合に内側のbodyのループが存在しないので分割出来ない。その場合はsrcRatioが小さい場合は外側のループが分割出来るので問題無い。大きい場合は縮小比率自体が大きいので
void AveragingReducer::Process_RatioAny(ILineAveragingReducer& lineReducer, uint16_t part)
{
	const AveragingReduceParams& params = *pParams;
	
	const __m128i* pSrc = params.srcBuff;
	__m128i* pTarget = params.targetBuff;
	
	const size_t denominator = params.widthRatioTarget * params.srcWidth * 4 * 2;
	size_t tmpLineOffsetBytes = (denominator / params.widthRatioSource) + (denominator % params.widthRatioSource);
	tmpLineOffsetBytes += 16 - (tmpLineOffsetBytes % 16);
	
	__m128i* tmpBuffBody = params.tmpBuff;
	__m128i* tmpBuffHead = params.tmpBuff;
	__m128i* tmpBuffTail = params.tmpBuff;
	OffsetPtr(tmpBuffHead, tmpLineOffsetBytes);
	OffsetPtr(tmpBuffTail, tmpLineOffsetBytes*2);
	
	const uint16_t srcRatio = params.heightRatioSource;
	const uint16_t targetRatio = params.heightRatioTarget;
	
	const uint16_t remainder = srcRatio % targetRatio;
	const uint16_t endBodyCnt = (srcRatio - targetRatio - remainder) / targetRatio;
	const uint32_t hvTargetRatio = targetRatio * params.widthRatioTarget;
	const uint32_t hvSrcRatio = srcRatio * params.widthRatioSource;
	const __m128i hvTargetSrcRatio = _mm_set1_epi16((65536.0 * hvTargetRatio) / hvSrcRatio + 0.5);
	uint16_t headRatio;
	
	const uint16_t storeCount = getTargetLineArrayCount(*pParams);

	OffsetPtr(tmpBuffBody, tmpLineOffsetBytes*3*part);
	OffsetPtr(tmpBuffHead, tmpLineOffsetBytes*3*part);
	OffsetPtr(tmpBuffTail, tmpLineOffsetBytes*3*part);
	const uint16_t blockCount = params.srcHeight / srcRatio;
	const uint16_t bodyCount = targetRatio - 2u;
	if (parts > 1 && blockCount == 1 && bodyCount >= 4) {
		const uint16_t partLen = bodyCount / parts;
		const uint16_t bodyStart = part * partLen;
		const uint16_t bodyEnd = (part == parts-1) ? bodyCount : (bodyStart + partLen);
		{
			// head
			if (bodyStart == 0) {
				// head + body
				Read(lineReducer, pSrc, tmpBuffBody, params.srcLineOffsetBytes, endBodyCnt);
				// srcRatio data straddles targetRatio's tail and next head
				lineReducer.fillRead(pSrc, tmpBuffHead);
				OffsetPtr(pSrc, params.srcLineOffsetBytes);
				headRatio = targetRatio - remainder;
				StoreToTarget(storeCount, pTarget, tmpBuffBody, tmpBuffHead, remainder, targetRatio, hvTargetSrcRatio);
				OffsetPtr(pTarget, params.targetLineOffsetBytes);
			}else {
				headRatio = targetRatio - remainder;
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
					OffsetPtr(pTarget, params.targetLineOffsetBytes);
					headRatio = targetRatio - tailRatio;
					const uint16_t withoutHead = srcRatio - headRatio;
					innerBodyCnt = withoutHead / targetRatio;
					tailRatio = withoutHead % targetRatio;
					std::swap(tmpBuffHead, tmpBuffTail);
				}
				for (uint16_t i=bodyStart; i<bodyEnd; ++i) {
					// body
					if (innerBodyCnt) {
						Read(lineReducer, pSrc, tmpBuffBody, params.srcLineOffsetBytes, innerBodyCnt-1u);
						// tail and next head
						lineReducer.fillRead(pSrc, tmpBuffTail);
						OffsetPtr(pSrc, params.srcLineOffsetBytes);
						StoreToTarget(storeCount, pTarget, tmpBuffBody, tmpBuffHead, tmpBuffTail, headRatio, tailRatio, targetRatio, hvTargetSrcRatio);
						OffsetPtr(pTarget, params.targetLineOffsetBytes);
					}else {
						// tail and next head
						lineReducer.fillRead(pSrc, tmpBuffTail);
						OffsetPtr(pSrc, params.srcLineOffsetBytes);
						StoreToTarget(storeCount, pTarget, tmpBuffHead, tmpBuffTail, headRatio, tailRatio, targetRatio, hvTargetSrcRatio);
						OffsetPtr(pTarget, params.targetLineOffsetBytes);
					}

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
				Read(lineReducer, pSrc, tmpBuffBody, params.srcLineOffsetBytes, endBodyCnt);
				StoreToTarget(storeCount, pTarget, tmpBuffBody, tmpBuffHead, headRatio, targetRatio, hvTargetSrcRatio);
				OffsetPtr(pTarget, params.targetLineOffsetBytes);
			}
		}
	}else {
		const uint16_t useParts = min(parts, blockCount);
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
				Read(lineReducer, pSrc, tmpBuffBody, params.srcLineOffsetBytes, endBodyCnt);
				// srcRatio data straddles targetRatio's tail and next head
				lineReducer.fillRead(pSrc, tmpBuffHead);
				OffsetPtr(pSrc, params.srcLineOffsetBytes);
				headRatio = targetRatio - remainder;
				StoreToTarget(storeCount, pTarget, tmpBuffBody, tmpBuffHead, remainder, targetRatio, hvTargetSrcRatio);
				OffsetPtr(pTarget, params.targetLineOffsetBytes);
			}
			// body
			{
				const uint16_t withoutHead = srcRatio - headRatio;
				uint16_t innerBodyCnt = withoutHead / targetRatio;
	//			assert(innerBodyCnt > 0);
				uint16_t tailRatio = withoutHead % targetRatio;
				for (uint16_t i=0; i<bodyCount; ++i) {
					// body
					if (innerBodyCnt) {
						Read(lineReducer, pSrc, tmpBuffBody, params.srcLineOffsetBytes, innerBodyCnt-1u);
						// tail and next head
						lineReducer.fillRead(pSrc, tmpBuffTail);
						OffsetPtr(pSrc, params.srcLineOffsetBytes);
						StoreToTarget(storeCount, pTarget, tmpBuffBody, tmpBuffHead, tmpBuffTail, headRatio, tailRatio, targetRatio, hvTargetSrcRatio);
						OffsetPtr(pTarget, params.targetLineOffsetBytes);
					}else {
						// tail and next head
						lineReducer.fillRead(pSrc, tmpBuffTail);
						OffsetPtr(pSrc, params.srcLineOffsetBytes);
						StoreToTarget(storeCount, pTarget, tmpBuffHead, tmpBuffTail, headRatio, tailRatio, targetRatio, hvTargetSrcRatio);
						OffsetPtr(pTarget, params.targetLineOffsetBytes);
					}

					headRatio = targetRatio - tailRatio;
					const uint16_t withoutHead = srcRatio - headRatio;
					innerBodyCnt = withoutHead / targetRatio;
	//				assert(innerBodyCnt > 0);
					tailRatio = withoutHead % targetRatio;
					std::swap(tmpBuffHead, tmpBuffTail);
				}
			}	
			// tail
			{
				// body + tail
				Read(lineReducer, pSrc, tmpBuffBody, params.srcLineOffsetBytes, endBodyCnt);
				StoreToTarget(storeCount, pTarget, tmpBuffBody, tmpBuffHead, headRatio, targetRatio, hvTargetSrcRatio);
				OffsetPtr(pTarget, params.targetLineOffsetBytes);
			}
		}
	}
}

void AveragingReducer::Process(uint16_t part)
{
	assert(part < parts);
	const AveragingReduceParams& params = *pParams;
	if (params.widthRatioTarget == params.widthRatioSource && params.heightRatioTarget == params.heightRatioSource) {
		const __m128i* pSrc = params.srcBuff;
		__m128i* pTarget = params.targetBuff;
		if (part >= params.srcHeight) {
			return;
		}
		const uint16_t useParts = min(parts, params.srcHeight);
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

