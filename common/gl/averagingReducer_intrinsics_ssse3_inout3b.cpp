#include "AveragingReducer_intrinsics_ssse3_inout3b.h"

#include "AveragingReducer_intrinsics.h"

#include "common.h"
#include "arrayutil.h"

#ifdef _DEBUG
#include <tchar.h>
#include "trace.h"
#endif

/*
reference

Visual C++ Language Reference Compiler Intrinsics
http://msdn.microsoft.com/ja-jp/library/26td21ds.aspx
http://msdn.microsoft.com/ja-jp/library/x8zs5twb.aspx
http://msdn.microsoft.com/ja-jp/library/bb892952.aspx

*/

namespace gl
{

namespace intrinsics_ssse3_inout3b
{

uint16_t getTargetLineArrayCount(const AveragingReduceParams& params)
{
	const uint16_t newWidth = (params.srcWidth * params.widthRatioTarget) / params.widthRatioSource;
	return newWidth / 3 + ((newWidth % 3) ? 1 : 0);
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

// TODO: 24
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
// TODO: 24
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

// 8bit * 4 を集計、16bit * 4 で返す
// TODO: 24
__forceinline __m128i CollectSum_1(const __m128i* pSrc)
{
	__m128i ret = _mm_loadl_epi64(pSrc);
	ret = _mm_unpacklo_epi8(ret, _mm_setzero_si128());
	return ret;
}

// TODO: 24
__forceinline __m128i CollectSum_2(const __m128i* pSrc)
{
	__m128i m0 = _mm_loadl_epi64(pSrc);
	m0 = _mm_unpacklo_epi8(m0, _mm_setzero_si128());
	__m128i m1 = _mm_srli_si128(m0, 8);
	__m128i ret = _mm_add_epi16(m0, m1);
	return ret;
}

// TODO: 24
__forceinline __m128i CollectSum_3(const __m128i* pSrc)
{
	return Split_3_1(load_unaligned_128(pSrc));
}


// TODO: 24
// greater than 4
class CollectSum
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

// TODO: 24
// greater than 4
class CollectSumAndNext1
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

class LineAveragingReducer : public LineAveragingReducer_RatioAny_Base
{
private:
	const uint32_t* pBodyCountBits_;
	const uint16_t* pTailTargetRatios_;

public:
	ILineAveragingReducer_Impl;
	uint8_t maxBodyCount_;

	LineAveragingReducer(const uint32_t* pBodyCountBits, const uint16_t* pTailTargetRatios)
		:
		pBodyCountBits_(pBodyCountBits),
		pTailTargetRatios_(pTailTargetRatios)
	{
	}
	
	// TODO: 24
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
				CollectSumAndNext1::work(pSrc, endLoopCnt, endLoopCnt - maxBodyCount_, col, col2);
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
					for (uint16_t j=0; j<32; ++j) {
						__m128i collected;
						__m128i col = col2; // head
						CollectSumAndNext1::work(pSrc, bodyCountBase, bodyCountBits & 1, collected, col2);
						bodyCountBits >>= 1;
						col = _mm_add_epi16(col, collected);
						__m128i tailTargetRatio = set1_epi16_low(*pWorkTailTargetRatios++);
						__m128i col2a = _mm_mulhi_epu16(col2, tailTargetRatio);	// col2 * tail/target
						col = _mm_add_epi16(col, col2a);
						_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
						OffsetPtr(pTmp, 8);
						col2 = _mm_sub_epi16(col2, col2a);
					}
				}
				{
					uint32_t bodyCountBits = *pWorkBodyCountBits;
					for (uint16_t i=0; i<bodyLoopCountRemainder; ++i) {
						__m128i collected;
						__m128i col = col2; // head
						CollectSumAndNext1::work(pSrc, bodyCountBase+(bodyCountBits & 1), bodyCountBits & 1, collected, col2);
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
				col = _mm_add_epi16(col, CollectSum::work(pSrc, endLoopCnt, endLoopCnt - maxBodyCount_));
				OffsetPtr(pSrc, endLoopCnt * 4);
				_mm_storel_epi64(pTmp, T::work(_mm_loadl_epi64(pTmp), col));
				OffsetPtr(pTmp, 8);
			}
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
	
	uint16_t tailTargetRatios_[ TABLE_COUNT ];
	
	ILineAveragingReducer* pLineAveragingReducer;
	LineAveragingReducer lar;
	
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
	
	void Process(ILineAveragingReducer& lineReducer, uint16_t part);
	
};

AveragingReducer::AveragingReducer()
	:
	lar(bodyCountBits_, tailTargetRatios_)
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
	}else {
		uint8_t maxBodyCount;
		RatioAny_PreCalculate(
			bodyCounts_,
			bodyCountBits_,
			maxBodyCount,
			tailTargetRatios_,
			params.widthRatioSource,
			params.widthRatioTarget
		);
		pLineAveragingReducer = &lar;
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

// TODO: 24
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

// TODO: 24
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

// TODO: 24
// 縦 自由比率縮小
void AveragingReducer::Process(ILineAveragingReducer& lineReducer, uint16_t part)
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
		Process(*pLineAveragingReducer, part);
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

}	// namespace intrinsics_ssse3_inout3b

}	// namespace gl

