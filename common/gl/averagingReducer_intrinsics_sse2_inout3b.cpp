#include "AveragingReducer_intrinsics_sse2_inout3b.h"

#include "common.h"
#include "arrayutil.h"

#include "AveragingReducer_intrinsics.h"

namespace gl
{

namespace intrinsics_sse2_inout3b
{

// 縦方向縮小比率 1 : N、横方向縮小比率 1 : 1
void AveragingReduce_Ratio1NX(
	const size_t heightRatioSource, const size_t width, 
	const __m128i* srcBuff, const size_t srcHeight, const int srcLineOffsetBytes,
	__m128i* targetBuff, const int targetLineOffsetBytes,
	__m128i* tmpBuff
	)
{
	assert(heightRatioSource < 256);
	assert(srcLineOffsetBytes % 16 == 0);		// 128ビットで割り切れる数
	assert(targetLineOffsetBytes % 16 == 0);	// 128ビットで割り切れる数
	
	assert(width % 16 == 0);					// 横16ドット単位、そうでないと割り切れず処理が面倒
	
	const size_t outerLoopCount = srcHeight / heightRatioSource;
	const int srcLineOffset = srcLineOffsetBytes / 16;
	const int targetLineOffset = targetLineOffsetBytes / 16;
	const size_t xCount = width / 16 * 3;
	
	const __m128i* pSrc = srcBuff;
	__m128i* pTarget = targetBuff;
	
	// 割算で使う比率の数字の逆数を16bit固定小数点数形式で用意
	__m128i invertRatioSource = _mm_set1_epi16(RoundValue(65536.0 / heightRatioSource));
	
	for (size_t y=0; y<outerLoopCount; ++y) {
		for (size_t i=0; i<heightRatioSource; ++i) {
			for (size_t x=0; x<xCount; ++x) {
				const __m128i* srcBase = pSrc + x;
				__m128i* tmpBase = tmpBuff + 2*x;
				__m128i s1 = _mm_load_si128(srcBase+0);
				__m128i tmp1 = _mm_load_si128(tmpBase+0);
				__m128i tmp2 = _mm_load_si128(tmpBase+1);
				__m128i loSrc = _mm_unpacklo_epi8(s1, _mm_setzero_si128());
				__m128i hiSrc = _mm_unpackhi_epi8(s1, _mm_setzero_si128());
				__m128i result1 = _mm_add_epi16(tmp1, loSrc);
				__m128i result2 = _mm_add_epi16(tmp2, hiSrc);
				_mm_store_si128(tmpBase+0, result1);
				_mm_store_si128(tmpBase+1, result2);
			}
			pSrc += srcLineOffset;
		}
		
		for (size_t x=0; x<xCount; ++x) {
			__m128i* tmpBase = tmpBuff + 2*x;
			__m128i* targetBase = pTarget + x;
			__m128i tmp1 = _mm_load_si128(tmpBase+0);
			__m128i tmp2 = _mm_load_si128(tmpBase+1);
			// zero fill for next time
			_mm_store_si128(tmpBase+0, _mm_setzero_si128());
			_mm_store_si128(tmpBase+1, _mm_setzero_si128());
			
			// retrieve upper 16 bits. lower bits are fractions.
			__m128i newTmp1 = _mm_mulhi_epu16(tmp1, invertRatioSource);
			__m128i newTmp2 = _mm_mulhi_epu16(tmp2, invertRatioSource);
			
			// 16bitから8bitに飽和変換
			__m128i newTmp = _mm_packus_epi16(newTmp1, newTmp2);
			_mm_store_si128(targetBase+0, newTmp);
		}
		
		pTarget += targetLineOffset;
	}
}

struct LineAveragingReducer_Ratio1
{
public:
	const size_t ratioSource;
	const size_t targetWidth;
	
	LineAveragingReducer_Ratio1(const size_t width)
		:
		ratioSource(1),
		targetWidth(width)
	{
	}
	
	void iterate(const __m128i* pSrc, __m128* pTarget)
	{
		const size_t xCount = targetWidth / 16 * 3;
		for (size_t x=0; x<xCount; ++x) {
			const __m128i* srcBase = pSrc + x;
			__m128* targetBase = pTarget + 4*x;
			__m128i s1 = _mm_load_si128(srcBase+0);
			__m128i loSrc = _mm_unpacklo_epi8(s1, _mm_setzero_si128());
			__m128i hiSrc = _mm_unpackhi_epi8(s1, _mm_setzero_si128());
			__m128 f1 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(loSrc, _mm_setzero_si128()));
			__m128 f2 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(loSrc, _mm_setzero_si128()));
			__m128 f3 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(hiSrc, _mm_setzero_si128()));
			__m128 f4 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(hiSrc, _mm_setzero_si128()));
			__m128 tmp1 = _mm_load_ps(targetBase+0);
			__m128 tmp2 = _mm_load_ps(targetBase+1);
			__m128 tmp3 = _mm_load_ps(targetBase+2);
			__m128 tmp4 = _mm_load_ps(targetBase+3);
			__m128 result1 = _mm_add_ps(tmp1, f1);
			__m128 result2 = _mm_add_ps(tmp2, f2);
			__m128 result3 = _mm_add_ps(tmp3, f3);
			__m128 result4 = _mm_add_ps(tmp4, f4);
			_mm_store_ps(targetBase+0, result1);
			_mm_store_ps(targetBase+1, result2);
			_mm_store_ps(targetBase+2, result3);
			_mm_store_ps(targetBase+3, result4);
		}
	}
	
	void iterate(const __m128i* pSrc, __m128* pTarget1, __m128 mulFactor1, __m128* pTarget2, __m128 mulFactor2)
	{
		const size_t xCount = targetWidth / 16 * 3;
		for (size_t x=0; x<xCount; ++x) {
			__m128i s1 = _mm_load_si128(pSrc + x);
			__m128i loSrc = _mm_unpacklo_epi8(s1, _mm_setzero_si128());
			__m128i hiSrc = _mm_unpackhi_epi8(s1, _mm_setzero_si128());
			__m128 f1 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(loSrc, _mm_setzero_si128()));
			__m128 f2 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(loSrc, _mm_setzero_si128()));
			__m128 f3 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(hiSrc, _mm_setzero_si128()));
			__m128 f4 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(hiSrc, _mm_setzero_si128()));
			{
				__m128* targetBase = pTarget1 + 4*x;
				__m128 r1,r2,r3,r4;
				r1 = _mm_mul_ps(f1, mulFactor1);
				r2 = _mm_mul_ps(f2, mulFactor1);
				r3 = _mm_mul_ps(f3, mulFactor1);
				r4 = _mm_mul_ps(f4, mulFactor1);
				_mm_store_ps(targetBase+0, _mm_add_ps(_mm_load_ps(targetBase+0), r1));
				_mm_store_ps(targetBase+1, _mm_add_ps(_mm_load_ps(targetBase+1), r2));
				_mm_store_ps(targetBase+2, _mm_add_ps(_mm_load_ps(targetBase+2), r3));
				_mm_store_ps(targetBase+3, _mm_add_ps(_mm_load_ps(targetBase+3), r4));
			}
			{
				__m128* targetBase = pTarget2 + 4*x;
				__m128 r1,r2,r3,r4;
				r1 = _mm_mul_ps(f1, mulFactor2);
				r2 = _mm_mul_ps(f2, mulFactor2);
				r3 = _mm_mul_ps(f3, mulFactor2);
				r4 = _mm_mul_ps(f4, mulFactor2);
				_mm_store_ps(targetBase+0, _mm_add_ps(_mm_load_ps(targetBase+0), r1));
				_mm_store_ps(targetBase+1, _mm_add_ps(_mm_load_ps(targetBase+1), r2));
				_mm_store_ps(targetBase+2, _mm_add_ps(_mm_load_ps(targetBase+2), r3));
				_mm_store_ps(targetBase+3, _mm_add_ps(_mm_load_ps(targetBase+3), r4));
			}
		}
	}
};

struct LineAveragingReducer_Ratio1NX
{
public:
	const size_t ratioSource;
	const size_t targetWidth;
	
	LineAveragingReducer_Ratio1NX(const size_t srcWidth, const size_t ratioSource)
		:
		targetWidth(srcWidth/ratioSource),
		ratioSource(ratioSource)
	{
		;
	}
	
	template <typename EffectorT>
	void process(const __m128i* srcBuff, EffectorT& effector)
	{
		const __m128i* pSrc = srcBuff;
		__m128i t1, t2, t3;
		size_t i = 0;
		for (; i<targetWidth/8; ++i) {
			Collect8_Add(pSrc, t1,t2,t3, ratioSource);
			OffsetPtr(pSrc, 8*3*ratioSource);
			effector(t1,t2,t3);
		}
		i *= 8;
		for (; i<targetWidth; ++i) {
			;
		}
	}
	
	void iterate(const __m128i* pSrc, __m128* pTarget)
	{
		struct Single
		{
			__m128* p;
			
			Single(__m128* p)
				:
				p(p)
			{
			}
			
			__forceinline void operator () (__m128i c1, __m128i c2, __m128i c3)
			{
				__m128 t1 = _mm_load_ps(p+0);
				__m128 t2 = _mm_load_ps(p+1);
				__m128 t3 = _mm_load_ps(p+2);
				__m128 t4 = _mm_load_ps(p+3);
				__m128 t5 = _mm_load_ps(p+4);
				__m128 t6 = _mm_load_ps(p+5);
				
				t1 = _mm_add_ps(t1, _mm_cvtepi32_ps(_mm_unpacklo_epi16(c1, _mm_setzero_si128())));
				t2 = _mm_add_ps(t2, _mm_cvtepi32_ps(_mm_unpackhi_epi16(c1, _mm_setzero_si128())));
				t3 = _mm_add_ps(t3, _mm_cvtepi32_ps(_mm_unpacklo_epi16(c2, _mm_setzero_si128())));
				t4 = _mm_add_ps(t4, _mm_cvtepi32_ps(_mm_unpackhi_epi16(c2, _mm_setzero_si128())));
				t5 = _mm_add_ps(t5, _mm_cvtepi32_ps(_mm_unpacklo_epi16(c3, _mm_setzero_si128())));
				t6 = _mm_add_ps(t6, _mm_cvtepi32_ps(_mm_unpackhi_epi16(c3, _mm_setzero_si128())));
				
				_mm_store_ps(p+0, t1);
				_mm_store_ps(p+1, t2);
				_mm_store_ps(p+2, t3);
				_mm_store_ps(p+3, t4);
				_mm_store_ps(p+4, t5);
				_mm_store_ps(p+5, t6);
				
				p += 6;
			}
		};
		process(pSrc, Single(pTarget));
	}
	
	void iterate(const __m128i* pSrc, __m128* pTarget1, __m128 mulFactor1, __m128* pTarget2, __m128 mulFactor2)
	{
		struct Double
		{
			__m128* ptr1;
			const __m128 mulFactor1;
			__m128* ptr2;
			const __m128 mulFactor2;
			
			Double(__m128* ptr1, __m128 mulFactor1, __m128* ptr2, __m128 mulFactor2)
				:
				ptr1(ptr1),
				mulFactor1(mulFactor1),
				ptr2(ptr2),
				mulFactor2(mulFactor2)
			{
			}
			
			__forceinline void operator () (__m128i c1, __m128i c2, __m128i c3)
			{
				__m128 t1 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(c1, _mm_setzero_si128()));
				__m128 t2 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(c1, _mm_setzero_si128()));
				__m128 t3 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(c2, _mm_setzero_si128()));
				__m128 t4 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(c2, _mm_setzero_si128()));
				__m128 t5 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(c3, _mm_setzero_si128()));
				__m128 t6 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(c3, _mm_setzero_si128()));
				
				_mm_store_ps(ptr1+0, _mm_add_ps(_mm_load_ps(ptr1+0), _mm_mul_ps(mulFactor1, t1)));
				_mm_store_ps(ptr1+1, _mm_add_ps(_mm_load_ps(ptr1+1), _mm_mul_ps(mulFactor1, t2)));
				_mm_store_ps(ptr1+2, _mm_add_ps(_mm_load_ps(ptr1+2), _mm_mul_ps(mulFactor1, t3)));
				_mm_store_ps(ptr1+3, _mm_add_ps(_mm_load_ps(ptr1+3), _mm_mul_ps(mulFactor1, t4)));
				_mm_store_ps(ptr1+4, _mm_add_ps(_mm_load_ps(ptr1+4), _mm_mul_ps(mulFactor1, t5)));
				_mm_store_ps(ptr1+5, _mm_add_ps(_mm_load_ps(ptr1+5), _mm_mul_ps(mulFactor1, t6)));
				ptr1 += 6;
				
				_mm_store_ps(ptr2+0, _mm_add_ps(_mm_load_ps(ptr2+0), _mm_mul_ps(mulFactor2, t1)));
				_mm_store_ps(ptr2+1, _mm_add_ps(_mm_load_ps(ptr2+1), _mm_mul_ps(mulFactor2, t2)));
				_mm_store_ps(ptr2+2, _mm_add_ps(_mm_load_ps(ptr2+2), _mm_mul_ps(mulFactor2, t3)));
				_mm_store_ps(ptr2+3, _mm_add_ps(_mm_load_ps(ptr2+3), _mm_mul_ps(mulFactor2, t4)));
				_mm_store_ps(ptr2+4, _mm_add_ps(_mm_load_ps(ptr2+4), _mm_mul_ps(mulFactor2, t5)));
				_mm_store_ps(ptr2+5, _mm_add_ps(_mm_load_ps(ptr2+5), _mm_mul_ps(mulFactor2, t6)));
				ptr2 += 6;				
			}
		};
		process(pSrc, Double(pTarget1, mulFactor1, pTarget2, mulFactor2));
	}
};

void AveragingReduce_RatioAny_StoreToTarget(
	const size_t xCount, const __m128 invertRatioSource, const __m128 productOfInverRatioSourceAndRatioTarget,
	__m128i* pTarget, __m128* pTmpPrimary, __m128* pTmpBody
	)
{
	for (size_t x=0; x<xCount; ++x) {
		__m128i* targetBase = pTarget + x;
		__m128* bodyBase = pTmpBody + 4*x;
		__m128* primaryBase = pTmpPrimary + 4*x;
		
		__m128 body1 = _mm_load_ps(bodyBase+0); 
		__m128 body2 = _mm_load_ps(bodyBase+1);
		__m128 body3 = _mm_load_ps(bodyBase+2); 
		__m128 body4 = _mm_load_ps(bodyBase+3);
		_mm_store_ps(bodyBase+0, _mm_setzero_ps());
		_mm_store_ps(bodyBase+1, _mm_setzero_ps());
		_mm_store_ps(bodyBase+2, _mm_setzero_ps());
		_mm_store_ps(bodyBase+3, _mm_setzero_ps());
		body1 = _mm_mul_ps(body1, productOfInverRatioSourceAndRatioTarget);
		body2 = _mm_mul_ps(body2, productOfInverRatioSourceAndRatioTarget);
		body3 = _mm_mul_ps(body3, productOfInverRatioSourceAndRatioTarget);
		body4 = _mm_mul_ps(body4, productOfInverRatioSourceAndRatioTarget);
		
		__m128 primary1 = _mm_load_ps(primaryBase+0);
		__m128 primary2 = _mm_load_ps(primaryBase+1);
		__m128 primary3 = _mm_load_ps(primaryBase+2);
		__m128 primary4 = _mm_load_ps(primaryBase+3);
		_mm_store_ps(primaryBase+0, _mm_setzero_ps());
		_mm_store_ps(primaryBase+1, _mm_setzero_ps());
		_mm_store_ps(primaryBase+2, _mm_setzero_ps());
		_mm_store_ps(primaryBase+3, _mm_setzero_ps());
		primary1 = _mm_mul_ps(primary1, invertRatioSource);
		primary2 = _mm_mul_ps(primary2, invertRatioSource);
		primary3 = _mm_mul_ps(primary3, invertRatioSource);
		primary4 = _mm_mul_ps(primary4, invertRatioSource);
		
		__m128 c11 = _mm_add_ps(body1, primary1);
		__m128 c12 = _mm_add_ps(body2, primary2);
		__m128 c21 = _mm_add_ps(body3, primary3);
		__m128 c22 = _mm_add_ps(body4, primary4);
		
		__m128i pack16a = _mm_packs_epi32(_mm_cvtps_epi32(c11),_mm_cvtps_epi32(c12));
		__m128i pack16b = _mm_packs_epi32(_mm_cvtps_epi32(c21),_mm_cvtps_epi32(c22));
		__m128i result = _mm_packus_epi16(pack16a, pack16b);
		_mm_stream_si128(targetBase, result);
/*
		TmpColorT c;
		c = tmpBuffBody[x] * ratioTarget2;
		c += tmpBuffMain[x] * invertRatioSource;
		tmpBuffBody[x] *= 0;
		tmpBuffMain[x] *= 0;
		targetBuff[x] = converter(c);
*/
	}
}

// 縦方向縮小比率 1 : N、横方向縮小比率 自由
template <typename LineAveragingReducerT>
void AveragingReduce_Ratio1NX(
	LineAveragingReducerT& lineReducer,
	const size_t heightRatioSource,
	const __m128i* srcBuff, const size_t srcHeight, const int srcLineOffsetBytes,
	__m128i* targetBuff, const int targetLineOffsetBytes,
	__m128* tmpBuff
	)
{
	assert(srcLineOffsetBytes % 16 == 0);		// 128ビットで割り切れる数
	assert(targetLineOffsetBytes % 16 == 0);	// 128ビットで割り切れる数
	
	const size_t outerLoopCount = srcHeight / heightRatioSource;
	const int srcLineOffset = srcLineOffsetBytes / 16;
	const int targetLineOffset = targetLineOffsetBytes / 16;
	
	const size_t targetWidth = lineReducer.targetWidth;
	const size_t xCount = (targetWidth / 16 + ((targetWidth % 16) ? 1 : 0)) * 3;
	
	const __m128i* pSrc = srcBuff;
	__m128i* pTarget = targetBuff;
	
	const size_t widthRatioSource = lineReducer.ratioSource;
	__m128 invertRatioSource =	_mm_set_ps1(1.0 / (widthRatioSource * heightRatioSource));
	for (size_t y=0; y<outerLoopCount; ++y) {
		for (size_t i=0; i<heightRatioSource; ++i) {
			lineReducer.iterate(pSrc, tmpBuff);
			OffsetPtr(pSrc, srcLineOffsetBytes);
		}
		for (size_t x=0; x<xCount; ++x) {
			__m128i* targetBase = pTarget + x;
			__m128* tmpBase = tmpBuff + 4 * x;

			__m128 tmp1 = _mm_load_ps(tmpBase+0); 
			__m128 tmp2 = _mm_load_ps(tmpBase+1);
			__m128 tmp3 = _mm_load_ps(tmpBase+2); 
			__m128 tmp4 = _mm_load_ps(tmpBase+3);
			_mm_store_ps(tmpBase+0, _mm_setzero_ps());
			_mm_store_ps(tmpBase+1, _mm_setzero_ps());
			_mm_store_ps(tmpBase+2, _mm_setzero_ps());
			_mm_store_ps(tmpBase+3, _mm_setzero_ps());
			tmp1 = _mm_mul_ps(tmp1, invertRatioSource);
			tmp2 = _mm_mul_ps(tmp2, invertRatioSource);
			tmp3 = _mm_mul_ps(tmp3, invertRatioSource);
			tmp4 = _mm_mul_ps(tmp4, invertRatioSource);
			
			__m128i pack16a = _mm_packs_epi32(_mm_cvtps_epi32(tmp1),_mm_cvtps_epi32(tmp2));
			__m128i pack16b = _mm_packs_epi32(_mm_cvtps_epi32(tmp3),_mm_cvtps_epi32(tmp4));
			__m128i result = _mm_packus_epi16(pack16a, pack16b);
			_mm_store_si128(targetBase, result);
		}
		pTarget += targetLineOffset;
	}
}

// 縦方向縮小比率自由
template <typename LineAveragingReducerT>
void AveragingReduce_RatioAny(
	LineAveragingReducerT& lineReducer,
	const size_t heightRatioTarget, const size_t heightRatioSource,
	const __m128i* srcBuff, const size_t srcHeight, int srcLineOffsetBytes,
	__m128i* targetBuff, const int targetLineOffsetBytes,
	__m128* tmpBuff, const int tmpLineOffsetBytes
	)
{
	assert(heightRatioTarget != 1);
	assert(heightRatioSource > heightRatioTarget);
	
	assert(srcLineOffsetBytes % 16 == 0);
	assert(targetLineOffsetBytes % 16 == 0);
	assert(tmpLineOffsetBytes % 16 == 0);
	
	const size_t ratioSource = heightRatioSource;
	const size_t ratioTarget = heightRatioTarget;
	const size_t ratioRemainder = ratioSource % ratioTarget;
	assert(ratioSource - ratioTarget - ratioRemainder >= 0);
	
	const size_t targetWidth = lineReducer.targetWidth;
	const size_t xCount = (targetWidth / 16 + ((targetWidth % 16) ? 1 : 0)) * 3;
	const size_t widthRatioSource = lineReducer.ratioSource;
	__m128 invertRatioSource =	_mm_set_ps1(1.0 / (ratioSource*widthRatioSource));
	__m128 productOfInverRatioSourceAndRatioTarget = _mm_set_ps1((1.0 * ratioTarget) / (ratioSource*widthRatioSource));
	
	const size_t endPartBodyIteCnt = (ratioSource - ratioTarget - ratioRemainder) / ratioTarget;	// end means head and tail
	const size_t ratioFirstBodyHead = ratioTarget - ratioRemainder;
	const size_t ratioFirstBodyBodyIteCnt = (ratioSource - ratioFirstBodyHead) / ratioTarget;
	const size_t ratioFirstBodyTail = (ratioSource - ratioFirstBodyHead) % ratioTarget;
	
	const __m128i* pSrc = srcBuff;
	__m128i* pTarget = targetBuff;
	
	__m128* tmpBuffBody = tmpBuff;
	__m128* tmpBuffPrimary = tmpBuff; OffsetPtr(tmpBuffPrimary, tmpLineOffsetBytes);
	__m128* tmpBuffSecondary = tmpBuffPrimary; OffsetPtr(tmpBuffSecondary, tmpLineOffsetBytes);
	
	const size_t mainIteCnt = srcHeight / ratioSource;
	for (size_t i=0; i<mainIteCnt; ++i) {
		size_t ratioNextHead = ratioFirstBodyHead;
		
		// HEAD
		{
			// head + body
			for (size_t j=0; j<1+endPartBodyIteCnt; ++j) {
				lineReducer.iterate(pSrc, tmpBuffBody);
				OffsetPtr(pSrc, srcLineOffsetBytes);
			}
			// tail and next head
			lineReducer.iterate(pSrc, tmpBuffPrimary, _mm_set1_ps(ratioRemainder), tmpBuffSecondary, _mm_set1_ps(ratioNextHead));
			OffsetPtr(pSrc, srcLineOffsetBytes);
			
			AveragingReduce_RatioAny_StoreToTarget(
				xCount, invertRatioSource, productOfInverRatioSourceAndRatioTarget,
				pTarget, tmpBuffPrimary, tmpBuffBody
			);
			std::swap(tmpBuffPrimary, tmpBuffSecondary);
			OffsetPtr(pTarget, targetLineOffsetBytes);
		}
		// BODY
		{
			size_t bodyBodyIteCnt = ratioFirstBodyBodyIteCnt;
			size_t ratioTail = ratioFirstBodyTail;
			const size_t bodyIteCnt = ratioTarget - 2;	// total - sides
			for (size_t j=0; j<bodyIteCnt; ++j) {
				// body
				for (size_t k=0; k<bodyBodyIteCnt; ++k) {
					lineReducer.iterate(pSrc, tmpBuffBody);
					OffsetPtr(pSrc, srcLineOffsetBytes);
				}
				// tail and next head
				ratioNextHead = ratioTarget - ratioTail;
				lineReducer.iterate(pSrc, tmpBuffPrimary, _mm_set1_ps(ratioTail), tmpBuffSecondary, _mm_set1_ps(ratioNextHead));
				OffsetPtr(pSrc, srcLineOffsetBytes);
				
				// convert
				AveragingReduce_RatioAny_StoreToTarget(
					xCount, invertRatioSource, productOfInverRatioSourceAndRatioTarget,
					pTarget, tmpBuffPrimary, tmpBuffBody
				);
				std::swap(tmpBuffPrimary, tmpBuffSecondary);
				OffsetPtr(pTarget, targetLineOffsetBytes);
				
				// calc next ratio
				size_t withoutHead = (ratioSource - ratioNextHead);
				bodyBodyIteCnt = withoutHead / ratioTarget;
				ratioTail = withoutHead % ratioTarget;
			}
		}
		// TAIL
		{
			// body + tail
			for (size_t j=0; j<endPartBodyIteCnt+1; ++j) {
				lineReducer.iterate(pSrc, tmpBuffBody);
				OffsetPtr(pSrc, srcLineOffsetBytes);
			}
			// convert
			AveragingReduce_RatioAny_StoreToTarget(
				xCount, invertRatioSource, productOfInverRatioSourceAndRatioTarget,
				pTarget, tmpBuffPrimary, tmpBuffBody
			);
			std::swap(tmpBuffPrimary, tmpBuffSecondary);
			OffsetPtr(pTarget, targetLineOffsetBytes);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
// 24bitの配列の足し算処理関数群、足し算の結果は、128bitレジスタに格納する。
// 結果は各値16bit3つで48bitになり、先頭に入る。
////////////////////////////////////////////////////////////////////////////////////
/*
01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16
rgbrgbrgbrgbrgbr
                gbrgbrgbrgbrgbrg
				                brgbrgbrgbrgbrgb
*/

// 1ドットの足し算処理（実質変換のみ）
__forceinline __m128i Load_1(const __m128i* pSrc)
{
#if 0
	union Vec {
		__m128i i;
		__m128 f;
	};
	Vec v;
	v.f = _mm_load_ss((float*)pSrc);
	return _mm_unpacklo_epi8(v.i, _mm_setzero_si128());
#else
	__m128i bytes = _mm_loadl_epi64(pSrc);
	return _mm_unpacklo_epi8(bytes, _mm_setzero_si128());
#endif
}

// 2ドットの足し算処理
__forceinline __m128i Add_2(const __m128i* pSrc)
{
	__m128i bytes = _mm_loadl_epi64(pSrc);
	__m128i remain16 = _mm_unpacklo_epi8(bytes, _mm_setzero_si128());
	__m128i another = _mm_srli_si128 (remain16, 6);
	__m128i sum = _mm_add_epi16(remain16, another);
	return sum;
}

// 3ドットの足し算処理
__forceinline __m128i Add_3(const __m128i* pSrc)
{
	__m128i bytes = load_unaligned_128(pSrc);
	__m128i c1 = _mm_unpacklo_epi8(bytes, _mm_setzero_si128());
	__m128i c2 = _mm_srli_si128 (c1, 6);
	__m128i sum = _mm_add_epi16(c1, c2);
	__m128i shiftedBytes = _mm_srli_si128 (bytes, 6);
	__m128i c3 = _mm_unpacklo_epi8(shiftedBytes, _mm_setzero_si128());
	sum = _mm_add_epi16(sum, c3);
	return sum;
}

__forceinline __m128i Add_4(const __m128i* pSrc)
{
	__m128i bytes = load_unaligned_128(pSrc);
	__m128i shiftedBytes = _mm_srli_si128 (bytes, 6);
	__m128i shorts = _mm_unpacklo_epi8(bytes, _mm_setzero_si128());
	__m128i shiftedShorts = _mm_unpacklo_epi8(shiftedBytes, _mm_setzero_si128());
	__m128i sum = _mm_add_epi16(shorts, shiftedShorts);
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_5(const __m128i* pSrc)
{
	__m128i bytes = load_unaligned_128(pSrc);
	static const __m128i mask = _mm_set_epi8(0,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255);
	bytes = _mm_and_si128(bytes, mask);
	__m128i shiftedBytes1 = _mm_srli_si128 (bytes, 6);
	__m128i shiftedBytes2 = _mm_srli_si128 (bytes, 12);
	__m128i shorts = _mm_unpacklo_epi8(bytes, _mm_setzero_si128());
	__m128i shiftedShorts1 = _mm_unpacklo_epi8(shiftedBytes1, _mm_setzero_si128());
	__m128i shiftedShorts2 = _mm_unpacklo_epi8(shiftedBytes2, _mm_setzero_si128());
	__m128i sum = _mm_add_epi16(shorts, shiftedShorts1);
	sum = _mm_add_epi16(sum, shiftedShorts2);
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_6(const __m128i* pSrc)
{
	__m128i bits128 = load_unaligned_128(pSrc);
	__m128i bits64 = _mm_loadl_epi64(pSrc+1);
	__m128i shiftedBytes1 = _mm_srli_si128(bits128, 6);
	__m128i shiftedBytes2 = _mm_srli_si128(bits128, 12);
	__m128i shifterBytes3 = _mm_slli_si128(bits64, 4);
	shiftedBytes2 = _mm_or_si128(shiftedBytes2, shifterBytes3);

	__m128i shorts = _mm_unpacklo_epi8(bits128, _mm_setzero_si128());
	__m128i shiftedShorts1 = _mm_unpacklo_epi8(shiftedBytes1, _mm_setzero_si128());
	__m128i shiftedShorts2 = _mm_unpacklo_epi8(shiftedBytes2, _mm_setzero_si128());
	__m128i sum = _mm_add_epi16(shorts, shiftedShorts1);
	sum = _mm_add_epi16(sum, shiftedShorts2);
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_7(const __m128i* pSrc)
{
	__m128i bits128 = load_unaligned_128(pSrc);
	__m128i bits64 = _mm_loadl_epi64(pSrc+1);
	bits64 = _mm_slli_si128(bits64, 11);

	__m128i shiftedBytes1 = _mm_srli_si128(bits128, 6);
	__m128i shiftedBytes2 = _mm_srli_si128(bits128, 12);
	__m128i shifterBytes3 = _mm_srli_si128(bits64, 11-4);
	shiftedBytes2 = _mm_or_si128(shiftedBytes2 , shifterBytes3);
	shifterBytes3 = _mm_srli_si128(shiftedBytes2, 6);
	
	__m128i shorts = _mm_unpacklo_epi8(bits128, _mm_setzero_si128());
	__m128i shiftedShorts1 = _mm_unpacklo_epi8(shiftedBytes1, _mm_setzero_si128());
	__m128i shiftedShorts2 = _mm_unpacklo_epi8(shiftedBytes2, _mm_setzero_si128());
	__m128i shiftedShorts3 = _mm_unpacklo_epi8(shifterBytes3, _mm_setzero_si128());
	__m128i sum = _mm_add_epi16(shorts, shiftedShorts1);
	sum = _mm_add_epi16(sum, shiftedShorts2);
	sum = _mm_add_epi16(sum, shiftedShorts3);
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_8(const __m128i* pSrc)
{
	__m128i bits128 = load_unaligned_128(pSrc);
	__m128i bits64 = _mm_loadl_epi64(pSrc+1);

	__m128i shiftedBytes1 = _mm_srli_si128(bits128, 6);
	__m128i shiftedBytes2 = _mm_srli_si128(bits128, 12);
	__m128i shifterBytes3 = _mm_slli_si128(bits64, 4);
	shiftedBytes2 = _mm_or_si128(shiftedBytes2 , shifterBytes3);
	shifterBytes3 = _mm_srli_si128(shiftedBytes2, 6);
	
	__m128i shorts = _mm_unpacklo_epi8(bits128, _mm_setzero_si128());
	__m128i shiftedShorts1 = _mm_unpacklo_epi8(shiftedBytes1, _mm_setzero_si128());
	__m128i shiftedShorts2 = _mm_unpacklo_epi8(shiftedBytes2, _mm_setzero_si128());
	__m128i shiftedShorts3 = _mm_unpacklo_epi8(shifterBytes3, _mm_setzero_si128());
	__m128i sum = _mm_add_epi16(shorts, shiftedShorts1);
	sum = _mm_add_epi16(sum, shiftedShorts2);
	sum = _mm_add_epi16(sum, shiftedShorts3);
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_9(const __m128i* pSrc)
{
	__m128i bits0 = load_unaligned_128(pSrc);
	__m128i bits1 = load_unaligned_128(pSrc+1);
	bits1 = _mm_slli_si128(bits1, 5);
	bits1 = _mm_srli_si128(bits1, 5);
	
	__m128i shiftedBytes1 = _mm_srli_si128(bits0, 6);
	__m128i shiftedBytes2 = _mm_srli_si128(bits0, 12);
	__m128i tmp = _mm_slli_si128(bits1, 4);
	shiftedBytes2 = _mm_or_si128(shiftedBytes2 , tmp);
	__m128i shiftedBytes3 = _mm_srli_si128(bits1, 2);
	__m128i shiftedBytes4 = _mm_srli_si128(bits1, 8);
		
	__m128i sum = _mm_unpacklo_epi8(bits0, _mm_setzero_si128());
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(shiftedBytes1, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(shiftedBytes2, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(shiftedBytes3, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(shiftedBytes4, _mm_setzero_si128()));
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_10(const __m128i* pSrc)
{
	__m128i bits0 = load_unaligned_128(pSrc);
	__m128i bits1 = load_unaligned_128(pSrc+1);
	
	__m128i shiftedBytes1 = _mm_srli_si128(bits0, 6);
	__m128i shiftedBytes2 = _mm_srli_si128(bits0, 12);
	__m128i tmp = _mm_slli_si128(bits1, 4);
	shiftedBytes2 = _mm_or_si128(shiftedBytes2 , tmp);
	__m128i shiftedBytes3 = _mm_srli_si128(bits1, 2);
	__m128i shiftedBytes4 = _mm_srli_si128(bits1, 8);
	
	__m128i sum = _mm_unpacklo_epi8(bits0, _mm_setzero_si128());
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(shiftedBytes1, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(shiftedBytes2, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(shiftedBytes3, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(shiftedBytes4, _mm_setzero_si128()));
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_11(const __m128i* pSrc)
{
	__m128i bits0 = load_unaligned_128(pSrc);
	__m128i bits1 = load_unaligned_128(pSrc+1);
	__m128i bits2 = _mm_loadl_epi64(pSrc+2);
	bits2 = _mm_slli_si128(bits2, 15);
	bits2 = _mm_srli_si128(bits2, 13);
	bits2 = _mm_or_si128( _mm_srli_si128(bits1, 14), bits2);
	
	bits1 = _mm_slli_si128(bits1, 2);
	bits1 = _mm_srli_si128(bits1, 1);
	bits1 = _mm_or_si128( _mm_srli_si128(bits0, 15), bits1);
	
	bits0 = _mm_slli_si128(bits0, 1);
	bits0 = _mm_srli_si128(bits0, 1);

	__m128i sum = _mm_unpacklo_epi8(bits0, _mm_setzero_si128());
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits1, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits2, _mm_setzero_si128()));
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_12(const __m128i* pSrc)
{
	__m128i bits0 = load_unaligned_128(pSrc);
	__m128i bits1 = load_unaligned_128(pSrc+1);
	__m128i bits2 = _mm_loadl_epi64(pSrc+2);
	bits2 = _mm_slli_si128(bits2, 12);
	bits2 = _mm_srli_si128(bits2, 10);
	bits2 = _mm_or_si128( _mm_srli_si128(bits1, 14), bits2);
	
	bits1 = _mm_slli_si128(bits1, 2);
	bits1 = _mm_srli_si128(bits1, 1);
	bits1 = _mm_or_si128( _mm_srli_si128(bits0, 15), bits1);
	
	bits0 = _mm_slli_si128(bits0, 1);
	bits0 = _mm_srli_si128(bits0, 1);

	__m128i sum = _mm_unpacklo_epi8(bits0, _mm_setzero_si128());
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits1, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits2, _mm_setzero_si128()));
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_13(const __m128i* pSrc)
{
	__m128i bits0 = load_unaligned_128(pSrc);
	__m128i bits1 = load_unaligned_128(pSrc+1);
	__m128i bits2 = _mm_loadl_epi64(pSrc+2);
	bits2 = _mm_slli_si128(bits2, 9);
	bits2 = _mm_srli_si128(bits2, 7);
	bits2 = _mm_or_si128( _mm_srli_si128(bits1, 14), bits2);
	
	bits1 = _mm_slli_si128(bits1, 2);
	bits1 = _mm_srli_si128(bits1, 1);
	bits1 = _mm_or_si128( _mm_srli_si128(bits0, 15), bits1);
	
	bits0 = _mm_slli_si128(bits0, 1);
	bits0 = _mm_srli_si128(bits0, 1);

	__m128i sum = _mm_unpacklo_epi8(bits0, _mm_setzero_si128());
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits1, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits2, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits2, 6), _mm_setzero_si128()));
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_14(const __m128i* pSrc)
{
	__m128i bits0 = load_unaligned_128(pSrc);
	__m128i bits1 = load_unaligned_128(pSrc+1);
	__m128i bits2	 = load_unaligned_128(pSrc+2);
	bits2 = _mm_slli_si128(bits2, 6);
	bits2 = _mm_srli_si128(bits2, 4);
	bits2 = _mm_or_si128( _mm_srli_si128(bits1, 14), bits2);
	
	bits1 = _mm_slli_si128(bits1, 2);
	bits1 = _mm_srli_si128(bits1, 1);
	bits1 = _mm_or_si128( _mm_srli_si128(bits0, 15), bits1);
	
	bits0 = _mm_slli_si128(bits0, 1);
	bits0 = _mm_srli_si128(bits0, 1);

	__m128i sum = _mm_unpacklo_epi8(bits0, _mm_setzero_si128());
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits1, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits2, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits2, 6), _mm_setzero_si128()));
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_15(const __m128i* pSrc)
{
	__m128i bits0 = load_unaligned_128(pSrc);
	__m128i bits1 = load_unaligned_128(pSrc+1);
	__m128i bits2 = load_unaligned_128(pSrc+2);
	bits2 = _mm_slli_si128(bits2, 3);
	bits2 = _mm_srli_si128(bits2, 1);
	bits2 = _mm_or_si128( _mm_srli_si128(bits1, 14), bits2);
	
	bits1 = _mm_slli_si128(bits1, 2);
	bits1 = _mm_srli_si128(bits1, 1);
	bits1 = _mm_or_si128( _mm_srli_si128(bits0, 15), bits1);
	
	bits0 = _mm_slli_si128(bits0, 1);
	bits0 = _mm_srli_si128(bits0, 1);

	__m128i sum = _mm_unpacklo_epi8(bits0, _mm_setzero_si128());
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits1, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits2, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits2, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits2, 12), _mm_setzero_si128()));
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add_16(const __m128i* pSrc)
{
	__m128i bits0 = load_unaligned_128(pSrc);
	__m128i bits1 = load_unaligned_128(pSrc+1);
	__m128i bits2 = load_unaligned_128(pSrc+2);
	__m128i bits2a = bits2;
	bits2 = _mm_slli_si128(bits2, 3);
	bits2 = _mm_srli_si128(bits2, 1);
	bits2 = _mm_or_si128( _mm_srli_si128(bits1, 14), bits2);
	
	bits1 = _mm_slli_si128(bits1, 2);
	bits1 = _mm_srli_si128(bits1, 1);
	bits1 = _mm_or_si128( _mm_srli_si128(bits0, 15), bits1);
	
	bits0 = _mm_slli_si128(bits0, 1);
	bits0 = _mm_srli_si128(bits0, 1);

	__m128i sum = _mm_unpacklo_epi8(bits0, _mm_setzero_si128());
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits0, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits1, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits1, 12), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(bits2, _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits2, 6), _mm_setzero_si128()));
	sum = _mm_add_epi16(sum, _mm_unpacklo_epi8(_mm_srli_si128(bits2a, 10), _mm_setzero_si128()));
	
	__m128i shiftedSum = _mm_srli_si128 (sum, 6);
	sum = _mm_add_epi16(sum, shiftedSum);
	return sum;
}

__forceinline __m128i Add(const __m128i* pSrc, const size_t count)
{
	switch (count) {
	case 0: return _mm_setzero_si128();
	case 1:	return Load_1(pSrc);
	case 2: return Add_2(pSrc);
	case 3: return Add_3(pSrc);
	case 4: return Add_4(pSrc);
	case 5: return Add_5(pSrc);
	case 6: return Add_6(pSrc);
	case 7: return Add_7(pSrc);
	case 8: return Add_8(pSrc);
	case 9: return Add_9(pSrc);
	case 10: return Add_10(pSrc);
	case 11: return Add_11(pSrc);
	case 12: return Add_12(pSrc);
	case 13: return Add_13(pSrc);
	case 14: return Add_14(pSrc);
	case 15: return Add_15(pSrc);
	case 16: return Add_16(pSrc);
	default:
#if 1
		{
			__m128i ret = _mm_setzero_si128();
			for (size_t i=0; i<count/16; ++i) {
				ret = _mm_add_epi16(ret, Add_16(pSrc));
				OffsetPtr(pSrc, 16*3);
			}
			return _mm_add_epi16(ret, Add(pSrc, count%16));
		}
#else
		__assume(0);
#endif
	}
}



// AMD sempron Manilaだと、遅い。演算回数が多すぎるのか？
inline void Collect8_Add_2(
	const __m128i* srcBuff,
	__m128i& t1, __m128i& t2, __m128i& t3
	)
{
	__m128i bits0 = _mm_load_si128(srcBuff+0);							// RGBRGBRGBRGBRGBR                                 bits0
	__m128i bits1 = _mm_load_si128(srcBuff+1);							//                 GBRGBRGBRGBRGBRG                 bits1
	__m128i bits2 = _mm_load_si128(srcBuff+2);							//                                 R9G9B9RaGaBaRbGb bits2
	__m128i shorts0 = _mm_unpacklo_epi8(bits0, _mm_setzero_si128());	// R1G1B1R2G2B2R3G3                                 shorts0
	__m128i shorts1 = _mm_unpackhi_epi8(bits0, _mm_setzero_si128());	// B3R4G4B4R5G5B5R6                                 shorts1
	__m128i shorts2 = _mm_unpacklo_epi8(bits1, _mm_setzero_si128());	// G6B6R7G7B7R8G8B8                                 shorts2
	__m128i shorts3 = _mm_unpackhi_epi8(bits1, _mm_setzero_si128());	// R9G9B9RaGaBaRbGb                                 shorts3
	__m128i shorts4 = _mm_unpacklo_epi8(bits2, _mm_setzero_si128());	// BbRcGcBcRdGdBdRe                                 shorts4
	__m128i shorts5 = _mm_unpackhi_epi8(bits2, _mm_setzero_si128());	// GeBeRfGfBfRgGgBg                                 shorts5
	
	__m128i w0,w1,w2, w00,w01,w02,w03;
	
	/////////////////////////////////////////////

	w0 = _mm_slli_si128(shorts0, 10);									// 0000000000R1G1B1 shorts0をshift
	w1 = _mm_slli_si128(shorts0, 4);									// 0000R1G1B1R2G2B2 shorts0をshift
	w0 = _mm_add_epi16(w0, w1);											// 0000R1G1B1RxGxBx add
	w00 = _mm_srli_si128(w0, 10);										// RxGxBx0000000000 shift
	
	w0 = _mm_srli_si128(shorts0, 12);									// R3G3000000000000 shorts0をshift
	w1 = _mm_slli_si128(shorts1, 14);									// 00000000000000B3 shorts1をshift
	w1 = _mm_srli_si128(w1, 10);										// 0000B30000000000 shift
	w1 = _mm_or_si128(w0, w1);											// R3G3B30000000000 or
	w0 = _mm_slli_si128(shorts1, 8);									// 00000000B3R4G4B4 shorts1をshift
	w0 = _mm_srli_si128(w0, 10);										// R4G4B40000000000 shift
	w0 = _mm_add_epi16(w0, w1);											// RxGxBx0000000000 add
	w01 = _mm_slli_si128(w0, 6);										// 000000RxGxBx0000 shift
	
	w0 = _mm_srli_si128(shorts1, 8);									// R5G5B5R600000000 shorts1をshift
	w2 = _mm_slli_si128(w0, 12);										// 000000000000R5G5 shift
	w0 = _mm_srli_si128(shorts1, 14);									// R600000000000000 shorts1をshift
	w1 = _mm_slli_si128(shorts2, 2);									// 00G6B6R7G7B7R8G8 shorts2をshift
	w0 = _mm_or_si128(w0, w1);											// R6G6B6R7G7B7R8G8 or
	w0 = _mm_slli_si128(w0, 12);										// 000000000000R6G6 shift
	w02 = _mm_add_epi16(w2, w0);										// 000000000000RxGx add
	
	t1 = _mm_or_si128(w00, w01);										// R1G1B1R3G3B30000 or
	t1 = _mm_or_si128(t1, w02);											// R1G1B1R3G3B3R5G5 or
	
//	t1 = _mm_setzero_si128();

	/////////////////////////////////////////////
	
	w0 = _mm_slli_si128(shorts1, 2);									// 00B3R4G4B4R5G5B5 shorts1をshift
	w1 = _mm_slli_si128(shorts2, 12);									// 000000000000G6B6 shorts2をshift
	w0 = _mm_add_epi16(w0, w1);											// --------------B5 add
	w00 = _mm_srli_si128(w0, 14);										// B500000000000000 shift
	
	w0 = _mm_slli_si128(shorts2, 6);									// 000000G6B6R7G7B7 shorts2をshift
	;																	// G6B6R7G7B7R8G8B8 shorts2そのまま
	w1 = _mm_add_epi16(w0, shorts2);									// ----------R7G7B7 add
	w1 = _mm_srli_si128(w1, 10);										// R7G7B70000000000 shift
	w01 = _mm_slli_si128(w1, 2);										// 00R7G7B700000000 shift
	
	;																	// R9G9B9RaGaBaRbGb shorts3そのまま
	w0 = _mm_srli_si128(shorts3, 6);									// RaGaBaRbGb000000 shorts3をshift
	w0 = _mm_add_epi16(shorts3, w0);									// R9G9B9---------- add
	w0 = _mm_slli_si128(w0, 10);										// 0000000000R9G9B9 shift
	w02 = _mm_srli_si128(w0, 2);										// 00000000R9G9B900 shift
	
	w0 = _mm_srli_si128(shorts3, 12);									// RbGb000000000000 shorts3をshift
	w0 = _mm_slli_si128(w0, 14);										// 00000000000000Rb shift
	w1 = _mm_srli_si128(shorts4, 2);									// RcGcBcRdGdBdRe00 shorts4をshift
	w1 = _mm_slli_si128(w1, 14);										// 00000000000000Rc shift
	w03 = _mm_add_epi16(w0, w1);										// 00000000000000Rb add
	
	t2 = _mm_or_si128(w00, w01);										// B5R7G7B700000000 or
	t2 = _mm_or_si128(t2, w02);											// B5R7G7B7R9G9B900 or
	t2 = _mm_or_si128(t2, w03);											// B5R7G7B7R9G9B9Rb or
	
//	t2 = _mm_setzero_si128();

	/////////////////////////////////////////////
	w0 = _mm_srli_si128(shorts3, 2);									// G9B9RaGaBaRbGb00 shorts3をshift
	w1 = _mm_slli_si128(shorts4, 14);									// 00000000000000Bb shorts4をshift
	w0 = _mm_or_si128(w0, w1);											// G9B9RaGaBaRbGbBb or
	w0 = _mm_srli_si128(w0, 12);										// GbBb000000000000 shift
	w1 = _mm_slli_si128(shorts4, 8);									// 00000000BbRcGcBc shorts4をshift
	w1 = _mm_srli_si128(w1, 12);										// GcBc000000000000 shift
	w00 = _mm_add_epi16(w0, w1);										// GbBb000000000000 add
	
	w0 = _mm_slli_si128(shorts4, 2);									// 00BbRcGcBcRdGdBd shorts4をshift
	w1 = _mm_srli_si128(shorts4, 4);									// GcBcRdGdBdRe0000 shorts4をshift
	w2 = _mm_slli_si128(shorts5, 12);									// 000000000000GeBe shorts5をshift
	w1 = _mm_or_si128(w1, w2);											// GcBcRdGdBdReGeBe or
	w0 = _mm_add_epi16(w0, w1);											// ----------RdGdBd add
	w0 = _mm_srli_si128(w0, 10);										// RdGdBd0000000000 shift
	w01 = _mm_slli_si128(w0, 4);										// 0000RdGdBd000000 shift
	
	;																	// shorts5そのまま
	w0 = _mm_slli_si128(shorts5, 6);									// 000000GeBeRfGfBf shift
	w1 = _mm_add_epi16(shorts5, w0);									// ----------RfGfBf add
	w1 = _mm_srli_si128(w1, 10);										// RfGfBf0000000000 shift
	w02 = _mm_slli_si128(w1, 10);										// 0000000000RfGfBf shift
	
	t3 = _mm_or_si128(w00, w01);										// GbBbRdGdBd000000 or
	t3 = _mm_or_si128(t3, w02);											// GbBbRdGdBdRfGfBf or

//	t3 = _mm_setzero_si128();
}

inline void Collect8_Add(
	const __m128i* srcBuff,
	__m128i& t1, __m128i& t2, __m128i& t3,
	const size_t widthRatioSource
	)
{
/*
R1G1B1R2G2B2R3G3
                B3R4G4B4R5G5B5R6
                                G6B6R7G7B7R8G8B8
*/

	if (widthRatioSource == 2) {
#if 0
		Collect8_Add_2(srcBuff, t1,t2,t3);
#else
		const __m128i* pSrc = srcBuff;
		static const __m128i mask = _mm_set_epi16(0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF);
		
		__m128i s1 = _mm_and_si128(Add_2(pSrc), mask); OffsetPtr(pSrc, 6);
		__m128i s2 = _mm_and_si128(Add_2(pSrc), mask); OffsetPtr(pSrc, 6);
		__m128i s3 = _mm_and_si128(Add_2(pSrc), mask); OffsetPtr(pSrc, 6);
		__m128i s4 = _mm_and_si128(Add_2(pSrc), mask); OffsetPtr(pSrc, 6);
		__m128i s5 = _mm_and_si128(Add_2(pSrc), mask); OffsetPtr(pSrc, 6);
		__m128i s6 = _mm_and_si128(Add_2(pSrc), mask); OffsetPtr(pSrc, 6);
		__m128i s7 = _mm_and_si128(Add_2(pSrc), mask); OffsetPtr(pSrc, 6);
		__m128i s8 = _mm_and_si128(Add_2(pSrc), mask); OffsetPtr(pSrc, 6);

		t1 = _mm_or_si128(s1, _mm_slli_si128(s2,6));
		t1 = _mm_or_si128(t1, _mm_slli_si128(s3,12));
		t2 = _mm_or_si128(_mm_srli_si128(s3,4), _mm_slli_si128(s4,2));
		t2 = _mm_or_si128(t2, _mm_slli_si128(s5,8));
		t2 = _mm_or_si128(t2, _mm_slli_si128(s6,14));
		t3 = _mm_or_si128(_mm_srli_si128(s6,2), _mm_slli_si128(s7,4));
		t3 = _mm_or_si128(t3, _mm_slli_si128(s8,10));

#endif
	}else {
		const __m128i* pSrc = srcBuff;
		static const __m128i mask = _mm_set_epi16(0,0,0,0,0,0xFFFF,0xFFFF,0xFFFF);
		
		__m128i s1 = _mm_and_si128(Add(pSrc, widthRatioSource), mask); OffsetPtr(pSrc, 3*widthRatioSource);
		__m128i s2 = _mm_and_si128(Add(pSrc, widthRatioSource), mask); OffsetPtr(pSrc, 3*widthRatioSource);
		__m128i s3 = _mm_and_si128(Add(pSrc, widthRatioSource), mask); OffsetPtr(pSrc, 3*widthRatioSource);
		__m128i s4 = _mm_and_si128(Add(pSrc, widthRatioSource), mask); OffsetPtr(pSrc, 3*widthRatioSource);
		__m128i s5 = _mm_and_si128(Add(pSrc, widthRatioSource), mask); OffsetPtr(pSrc, 3*widthRatioSource);
		__m128i s6 = _mm_and_si128(Add(pSrc, widthRatioSource), mask); OffsetPtr(pSrc, 3*widthRatioSource);
		__m128i s7 = _mm_and_si128(Add(pSrc, widthRatioSource), mask); OffsetPtr(pSrc, 3*widthRatioSource);
		__m128i s8 = _mm_and_si128(Add(pSrc, widthRatioSource), mask); OffsetPtr(pSrc, 3*widthRatioSource);

		t1 = _mm_or_si128(s1, _mm_slli_si128(s2,6));
		t1 = _mm_or_si128(t1, _mm_slli_si128(s3,12));
		t2 = _mm_or_si128(_mm_srli_si128(s3,4), _mm_slli_si128(s4,2));
		t2 = _mm_or_si128(t2, _mm_slli_si128(s5,8));
		t2 = _mm_or_si128(t2, _mm_slli_si128(s6,14));
		t3 = _mm_or_si128(_mm_srli_si128(s6,2), _mm_slli_si128(s7,4));
		t3 = _mm_or_si128(t3, _mm_slli_si128(s8,10));
	}
}

inline void AddAverage16(
	const __m128i* pSrc,
	__m128i* pTarget,
	const size_t widthRatioSource,
	const __m128i invertRatioSource
	)
{
	__m128i t1, t2, t3;
	Collect8_Add(pSrc, t1,t2,t3, widthRatioSource);
	__m128i packed1 = _mm_packus_epi16(
		_mm_mulhi_epu16(t1, invertRatioSource),
		_mm_mulhi_epu16(t2, invertRatioSource)
	);
	// _mm_storeu_si128
	_mm_storeu_si128(pTarget, packed1);

	OffsetPtr(pSrc, 8*3*widthRatioSource);
	
	__m128i t4, t5, t6;
	Collect8_Add(pSrc, t4,t5,t6, widthRatioSource);
	__m128i packed2 = _mm_packus_epi16(
		_mm_mulhi_epu16(t3, invertRatioSource),
		_mm_mulhi_epu16(t4, invertRatioSource)
	);
	_mm_store_si128(pTarget+1, packed2);
	
	__m128i packed3 = _mm_packus_epi16(
		_mm_mulhi_epu16(t5, invertRatioSource),
		_mm_mulhi_epu16(t6, invertRatioSource)
	);
	_mm_store_si128(pTarget+2, packed3);
}

// 縦方向縮小比率 1: N、横方向縮小比率 1 : M
void AveragingReduce_RatioNXNX(
	const size_t widthRatioSource, const size_t heightRatioSource,
	const __m128i* srcBuff, const size_t srcWidth, const size_t srcHeight, const int srcLineOffsetBytes,
	__m128i* targetBuff, const int targetLineOffsetBytes,
	__m128i* tmpBuff
	)
{
	assert(widthRatioSource != 1);
	const size_t targetWidth = srcWidth / widthRatioSource;
	const __m128i* pSrc = srcBuff;
	__m128i* pTarget = targetBuff;
	
	__m128i invertRatioSource = _mm_set1_epi16(RoundValue(65536.0 / widthRatioSource));
	
	if (heightRatioSource == 1) {
		for (size_t y=0; y<srcHeight; ++y) {
			const __m128i* pSrcTmp = pSrc;
			__m128i* pTargetTmp = pTarget;
			size_t i = 0;
			for (; i<targetWidth/16; ++i) {
				AddAverage16(pSrcTmp, pTargetTmp, widthRatioSource, invertRatioSource);
				pTargetTmp += 3;
				pSrcTmp += 3*widthRatioSource;
			}
			i *= 16;
			for (; i<targetWidth; ++i) {
				__m128i ret = Add(pSrcTmp, widthRatioSource);
				ret = _mm_mulhi_epu16(ret, invertRatioSource);
				__m128i ret8 = _mm_packus_epi16(ret, _mm_setzero_si128());
				_mm_storeu_si128(pTargetTmp, ret8);
				OffsetPtr(pTargetTmp, 3);
				OffsetPtr(pSrcTmp, 3*widthRatioSource);
			}
			OffsetPtr(pSrc, srcLineOffsetBytes);
			OffsetPtr(pTarget, targetLineOffsetBytes);
		}
	}else {
		
	}
}

__forceinline __m128i LineAveragingReduce_RatioAny_CollectSample(
	__m128i& prevLastSample, const __m128i*& pSrc,
	const __m128i headSampleCountShorts, const size_t bodyIterationCount, const __m128i bodySampleCountShorts, const __m128i tailSampleCountShorts
)
{
	// head
	__m128i tmp = _mm_madd_epi16(prevLastSample, headSampleCountShorts);
	// body
	__m128i bodySampleSum = _mm_madd_epi16(shorts_to_ints(Add(pSrc, bodyIterationCount)), bodySampleCountShorts);
	OffsetPtr(pSrc, 3*bodyIterationCount);
	tmp = _mm_add_epi32(tmp, bodySampleSum);
	// tail
	prevLastSample = shorts_to_ints(Load_1(pSrc));
	OffsetPtr(pSrc, 3);
	tmp = _mm_add_epi32(tmp, _mm_madd_epi16(prevLastSample, tailSampleCountShorts));
	return tmp;
}

__forceinline __m128i LineAveragingReduce_RatioAny_CollectSample(
	__m128i& prevLastSample, const __m128i*& pSrc,
	const __m128i headSampleCountShorts, const __m128i bodySampleCountShorts, const __m128i tailSampleCountShorts
)
{
	// head
	__m128i tmp = _mm_madd_epi16(prevLastSample, headSampleCountShorts);

	__m128i bytes = _mm_loadl_epi64(pSrc); OffsetPtr(pSrc, 6);
	__m128i shorts = _mm_unpacklo_epi8(bytes, _mm_setzero_si128());
	// body
	__m128i bodySampleSum = _mm_madd_epi16(shorts_to_ints(shorts), bodySampleCountShorts);
	tmp = _mm_add_epi32(tmp, bodySampleSum);
	// tail
	prevLastSample = shorts_to_ints(_mm_srli_si128(shorts, 6));
	tmp = _mm_add_epi32(tmp, _mm_madd_epi16(prevLastSample, tailSampleCountShorts));
	return tmp;
}

__inline __m128i LineAveragingReduce_RatioAny_CollectSample(
	__m128i& prevLastSample, const __m128i*& pSrc, const __m128i headSampleCountShorts, const __m128i tailSampleCountShorts
)
{
	// head
	__m128i tmp = _mm_madd_epi16(prevLastSample, headSampleCountShorts);
	// tail
	prevLastSample = shorts_to_ints(Load_1(pSrc)); OffsetPtr(pSrc, 3);
	tmp = _mm_add_epi32(tmp, _mm_madd_epi16(prevLastSample, tailSampleCountShorts));
	return tmp;
}

struct LineAveragingReducer_RatioAny
{
public:
	const size_t ratioSource;
	const size_t ratioTarget;
	const size_t srcWidth;
	const size_t targetWidth;
	
	// precalculation
	const size_t ratioRemainder;
	// end means head and tail
	const size_t endPartBodyIteCnt;
	
	const size_t numOfFullDots;
	const size_t imcompleteDotSamples;
	
	size_t preCalculatedParameters[8192];
	
	LineAveragingReducer_RatioAny(const size_t srcWidth, const size_t ratioSource, const size_t ratioTarget)
		:
		srcWidth(srcWidth),
		ratioSource(ratioSource),
		ratioTarget(ratioTarget),
		targetWidth((srcWidth * ratioTarget) / ratioSource + ((srcWidth * ratioTarget) % ratioSource != 0)),
		ratioRemainder(ratioSource % ratioTarget),
		endPartBodyIteCnt((ratioSource - ratioTarget - ratioRemainder) / ratioTarget),
		numOfFullDots((ratioTarget * (srcWidth % ratioSource )) / ratioSource ),
		imcompleteDotSamples((ratioTarget * (srcWidth % ratioSource )) % ratioSource )
	{
		assert(ratioSource % ratioTarget != 0);

		size_t ratioTail = ratioRemainder;
		const size_t bodyIteCnt = (ratioTarget - 2);	// total - sides
		assert(bodyIteCnt < 8192);
		for (size_t i=0; i<bodyIteCnt; ++i) {
			const size_t ratioHead = ratioTarget - ratioTail;
			const size_t withoutHead = ratioSource - ratioHead;
			const size_t bodyBodyIteCnt = withoutHead / ratioTarget;
			ratioTail = withoutHead % ratioTarget;
			preCalculatedParameters[i] = bodyBodyIteCnt | (ratioTail << 16);
		}
	}

	template <typename EffectorT>
	void process(const __m128i* srcBuff, EffectorT& effector)
	{
		const __m128i vRatioSource = _mm_set1_epi16(ratioSource);
		const __m128i vRatioTarget = _mm_set1_epi16(ratioTarget);
		const __m128i vRatioTargetShorts = _mm_set1_epi16(ratioTarget);
		const __m128i vRatioRemainder = _mm_set1_epi16(ratioRemainder);
		
		const __m128i* pSrc = srcBuff;
		const size_t iteCnt = srcWidth / ratioSource;

		for (size_t i=0; i<iteCnt; ++i) {
			// HEAD
			__m128i firstSample = shorts_to_ints(Load_1(pSrc)); OffsetPtr(pSrc, 3);
			effector( LineAveragingReduce_RatioAny_CollectSample(firstSample, pSrc, vRatioTarget, endPartBodyIteCnt, vRatioTargetShorts, vRatioRemainder) );
			
			// BODY
			__m128i vRatioTail = vRatioRemainder;
			const size_t bodyIteCnt = (ratioTarget - 2);	// total - sides
			if (ratioTarget * 2 > ratioSource) {
				for (size_t j=0; j<bodyIteCnt; ++j) {
					// next ratio
					const __m128i vRatioHead = _mm_sub_epi16(vRatioTarget, vRatioTail);
					const size_t param = preCalculatedParameters[j];
					const size_t bodyBodyIteCnt = param & 0xFFFF;
					if (bodyBodyIteCnt == 0) {
						vRatioTail = _mm_sub_epi16(vRatioSource, vRatioHead);
						effector( LineAveragingReduce_RatioAny_CollectSample(firstSample, pSrc, vRatioHead, vRatioTail) );
					}else {
						assert(bodyBodyIteCnt == 1);
						vRatioTail = _mm_set1_epi16(param >> 16);
						effector( LineAveragingReduce_RatioAny_CollectSample(firstSample, pSrc, vRatioHead, vRatioTargetShorts, vRatioTail) );
					}
				}
			}else {
				for (size_t j=0; j<bodyIteCnt; ++j) {
					// next ratio
					const __m128i vRatioHead = _mm_sub_epi16(vRatioTarget, vRatioTail);
					const size_t param = preCalculatedParameters[j];
					const size_t bodyBodyIteCnt = param & 0xFFFF;
					vRatioTail = _mm_set1_epi16(param >> 16);
					effector( LineAveragingReduce_RatioAny_CollectSample(firstSample, pSrc, vRatioHead, bodyBodyIteCnt, vRatioTargetShorts, vRatioTail) );
				}
			}
			// TAIL
			effector( LineAveragingReduce_RatioAny_CollectSample(firstSample, pSrc, vRatioRemainder, endPartBodyIteCnt, vRatioTargetShorts, vRatioTarget) );
		}

	}
	
#if 1
	void iterate(const __m128i* pSrc, __m128* pTarget)
	{
		struct Single
		{
			__m128* p;
			
			Single(__m128* p)
				:
				p(p)
			{
			}
			
			__forceinline void operator () (__m128i col)
			{
				__m128 c = _mm_loadu_ps(p);
				static const __m128 set0 = {1.0,1.0,1.0,0.0};
				c = _mm_add_ps(c, _mm_mul_ps(_mm_cvtepi32_ps(col), set0));
				_mm_storeu_ps(p, c);
				OffsetPtr(p, 12);
			}
		};
		process(pSrc, Single(pTarget));
	}
	
	void iterate(const __m128i* pSrc, __m128* pTarget1, __m128 mulFactor1, __m128* pTarget2, __m128 mulFactor2)
	{
		struct Double
		{
			__m128* ptr1;
			const __m128 mulFactor1;
			__m128* ptr2;
			const __m128 mulFactor2;
			
			Double(__m128* ptr1, __m128 mulFactor1, __m128* ptr2, __m128 mulFactor2)
				:
				ptr1(ptr1),
				mulFactor1(mulFactor1),
				ptr2(ptr2),
				mulFactor2(mulFactor2)
			{
			}

			__forceinline void operator () (__m128i col)
			{
				__m128 c = _mm_cvtepi32_ps(col);
				__m128 mc1 = _mm_mul_ps(c, mulFactor1);
				__m128 mc2 = _mm_mul_ps(c, mulFactor2);
				
				__m128 c1 = _mm_loadu_ps(ptr1);
				c1 = _mm_add_ps(c1, mc1);
				_mm_storeu_ps(ptr1, c1);
				OffsetPtr(ptr1, 12);
				
				__m128 c2 = _mm_loadu_ps(ptr2);
				c2 = _mm_add_ps(c2, mc2);
				_mm_storeu_ps(ptr2, c2);
				OffsetPtr(ptr2, 12);
			}
		};
		static const __m128 set0 = {1.0,1.0,1.0,0.0};
		mulFactor1 = _mm_mul_ps(mulFactor1, set0);
		mulFactor2 = _mm_mul_ps(mulFactor2, set0);
		process(pSrc, Double(pTarget1, mulFactor1, pTarget2, mulFactor2));
	}
#else
	// 下記の方法は、Core2 Meromだと少し速いが(7/8)、Athlon64 Sempronだととても遅い。(3/2)
	void iterate(const __m128i* pSrc, __m128* pTarget)
	{
		struct Single
		{
			__m128* p;
			size_t cnt;
			__m128i tmp;
			
			Single(__m128* p)
				:
				p(p),
				cnt(0)
			{
			}
			
			__forceinline void operator () (__m128i col)
			{
				switch (cnt) {
				case 0:
					{
						static const __m128i mask = _mm_set_epi32(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
						tmp = _mm_and_si128(col, mask);
						++cnt;
					}
					break;
				case 1:
					{
						__m128 f = _mm_cvtepi32_ps(_mm_or_si128(_mm_slli_si128(col, 12), tmp));
						__m128 l = _mm_load_ps(p);
						static const __m128i mask = _mm_set_epi32(0, 0, 0xFFFFFFFF, 0xFFFFFFFF);
						tmp = _mm_and_si128(_mm_srli_si128(col, 4), mask);
						_mm_store_ps(p, _mm_add_ps(l, f));
						++p;
						++cnt;
					}
					break;
				case 2:
					{
						__m128 f = _mm_cvtepi32_ps(_mm_or_si128(_mm_slli_si128(col, 8), tmp));
						__m128 l = _mm_load_ps(p);
						static const __m128i mask = _mm_set_epi32(0, 0, 0, 0xFFFFFFFF);
						tmp = _mm_and_si128(_mm_srli_si128(col, 8), mask);
						_mm_store_ps(p, _mm_add_ps(l, f));
						++p;
						++cnt;
					}
					break;
				case 3:
					{
						__m128 f = _mm_cvtepi32_ps(_mm_or_si128(_mm_slli_si128(col, 4), tmp));
						_mm_store_ps(p, _mm_add_ps(_mm_load_ps(p), f));
						++p;
						cnt = 0;
					}
					break;
				default:
					__assume(0);
				}
			}
		};
		process(pSrc, Single(pTarget));
	}
	
	void iterate(const __m128i* pSrc, __m128* pTarget1, __m128 mulFactor1, __m128* pTarget2, __m128 mulFactor2)
	{
		struct Double
		{
			__m128* ptr1;
			const __m128 mulFactor1;
			__m128* ptr2;
			const __m128 mulFactor2;
			
			size_t cnt;
			__m128i tmp;
			
			Double(__m128* ptr1, __m128 mulFactor1, __m128* ptr2, __m128 mulFactor2)
				:
				ptr1(ptr1),
				mulFactor1(mulFactor1),
				ptr2(ptr2),
				mulFactor2(mulFactor2),
				cnt(0)
			{
			}
			
			__forceinline void operator () (__m128i col)
			{
				switch (cnt) {
				case 0:
					{
						static const __m128i mask = _mm_set_epi32(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
						tmp = _mm_and_si128(col, mask);
						++cnt;
					}
					break;
				case 1:
					{
						__m128 f = _mm_cvtepi32_ps(_mm_or_si128(_mm_slli_si128(col, 12), tmp));
						__m128 mc1 = _mm_mul_ps(f, mulFactor1);
						__m128 mc2 = _mm_mul_ps(f, mulFactor2);
						__m128 l1 = _mm_load_ps(ptr1);
						__m128 l2 = _mm_load_ps(ptr2);
						static const __m128i mask = _mm_set_epi32(0, 0, 0xFFFFFFFF, 0xFFFFFFFF);
						tmp = _mm_and_si128(_mm_srli_si128(col, 4), mask);
						_mm_store_ps(ptr1, _mm_add_ps(l1, mc1));
						_mm_store_ps(ptr2, _mm_add_ps(l2, mc2));
						++ptr1;
						++ptr2;
						++cnt;
					}
					break;
				case 2:
					{
						__m128 f = _mm_cvtepi32_ps(_mm_or_si128(_mm_slli_si128(col, 8), tmp));
						__m128 mc1 = _mm_mul_ps(f, mulFactor1);
						__m128 mc2 = _mm_mul_ps(f, mulFactor2);
						__m128 l1 = _mm_load_ps(ptr1);
						__m128 l2 = _mm_load_ps(ptr2);
						static const __m128i mask = _mm_set_epi32(0, 0, 0, 0xFFFFFFFF);
						tmp = _mm_and_si128(_mm_srli_si128(col, 8), mask);
						_mm_store_ps(ptr1, _mm_add_ps(l1, mc1));
						_mm_store_ps(ptr2, _mm_add_ps(l2, mc2));
						++ptr1;
						++ptr2;
						++cnt;
					}
					break;
				case 3:
					{
						__m128 f = _mm_cvtepi32_ps(_mm_or_si128(_mm_slli_si128(col, 4), tmp));
						__m128 mc1 = _mm_mul_ps(f, mulFactor1);
						__m128 mc2 = _mm_mul_ps(f, mulFactor2);
						__m128 l1 = _mm_load_ps(ptr1);
						__m128 l2 = _mm_load_ps(ptr2);
						_mm_store_ps(ptr1, _mm_add_ps(l1, mc1));
						_mm_store_ps(ptr2, _mm_add_ps(l2, mc2));
						++ptr1;
						++ptr2;
						cnt = 0;
					}
					break;
				default:
					__assume(0);
				}
			}
		};
		process(pSrc, Double(pTarget1, mulFactor1, pTarget2, mulFactor2));
	}
#endif
};

void AveragingReduce(
	const size_t heightRatioTarget, const size_t heightRatioSource, const size_t srcHeight, 
	const size_t widthRatioTarget, const size_t widthRatioSource, const size_t srcWidth,
	const __m128i* srcBuff, const int srcLineOffsetBytes,
	__m128i* targetBuff, const int targetLineOffsetBytes,
	char* tmpBuff
	)
{
#if 0
	__m128i* pTarget = targetBuff;
	for (size_t i=0; i<srcHeight; ++i) {
		memset(pTarget, 0, abs(targetLineOffsetBytes));
		OffsetPtr(pTarget, targetLineOffsetBytes);
	}
#endif

	const size_t srcWidthFixed = srcWidth + (16 - srcWidth % 16) + 16;
	
	if (widthRatioTarget == widthRatioSource) {
		if (heightRatioTarget == heightRatioSource) {
			const __m128i* pSrc = srcBuff;
			__m128i* pTarget = targetBuff;
			for (size_t y=0; y<srcHeight; ++y) {
				memcpy(pTarget, pSrc, srcWidth*3);
				OffsetPtr(pSrc, srcLineOffsetBytes);
				OffsetPtr(pTarget, targetLineOffsetBytes);
			}
		}else if (heightRatioSource % heightRatioTarget == 0) {
			const int tmpBuffLineOffsetBytes = (srcWidthFixed * 3) * 2;
			std::fill(tmpBuff, tmpBuff+tmpBuffLineOffsetBytes, 0);
			AveragingReduce_Ratio1NX(
				heightRatioSource/heightRatioTarget, srcWidth,
				srcBuff, srcHeight, srcLineOffsetBytes,
				(__m128i*)targetBuff, targetLineOffsetBytes,
				(__m128i*)tmpBuff
			);
		}else {
			const int tmpBuffLineOffsetBytes = (srcWidthFixed * 3) * 4;
			std::fill(tmpBuff, tmpBuff+tmpBuffLineOffsetBytes*3, 0);
			AveragingReduce_RatioAny(
				LineAveragingReducer_Ratio1(srcWidth),
				heightRatioTarget, heightRatioSource,
				srcBuff, srcHeight, srcLineOffsetBytes,
				targetBuff, targetLineOffsetBytes,
				(__m128*)tmpBuff, tmpBuffLineOffsetBytes
			);
		}
	}else if ((widthRatioSource % widthRatioTarget) == 0) {
		const size_t widthRatioSourceFixed = widthRatioSource / widthRatioTarget;
		if ((heightRatioSource % heightRatioTarget) == 0) {
			AveragingReduce_RatioNXNX(
				widthRatioSourceFixed, heightRatioSource/heightRatioTarget,
				srcBuff, srcWidth, srcHeight, srcLineOffsetBytes,
				targetBuff, targetLineOffsetBytes,
				(__m128i*)tmpBuff
			);
		}else {
			const size_t targetWidth = (srcWidth / widthRatioSourceFixed) + (srcWidth % widthRatioSourceFixed ? widthRatioSourceFixed : 0);
			const int tmpBuffLineOffsetBytes = align((targetWidth * 3) * 4, 16);
			std::fill(tmpBuff, tmpBuff+tmpBuffLineOffsetBytes, 0);
			AveragingReduce_RatioAny(
				LineAveragingReducer_Ratio1NX(srcWidth, widthRatioSourceFixed),
				heightRatioTarget, heightRatioSource,
				srcBuff, srcHeight, srcLineOffsetBytes,
				targetBuff, targetLineOffsetBytes,
				(__m128*)tmpBuff, tmpBuffLineOffsetBytes
			);
		}
	}else {
		if ((heightRatioSource % heightRatioTarget) == 0) {
			const int tmpBuffLineOffsetBytes = srcWidthFixed * 4;
			std::fill(tmpBuff, tmpBuff+tmpBuffLineOffsetBytes*3, 0);
			AveragingReduce_Ratio1NX(
				LineAveragingReducer_RatioAny(srcWidth, widthRatioSource, widthRatioTarget),
				heightRatioSource/heightRatioTarget,
				srcBuff, srcHeight, srcLineOffsetBytes,
				targetBuff, targetLineOffsetBytes,
				(__m128*)tmpBuff
			);

		}else {
			const int tmpBuffLineOffsetBytes = (srcWidthFixed * 3) * 4;
			std::fill(tmpBuff, tmpBuff+tmpBuffLineOffsetBytes*3, 0);
			AveragingReduce_RatioAny(
				LineAveragingReducer_RatioAny(srcWidth, widthRatioSource, widthRatioTarget),
				heightRatioTarget, heightRatioSource,
				srcBuff, srcHeight, srcLineOffsetBytes,
				targetBuff, targetLineOffsetBytes,
				(__m128*)tmpBuff, tmpBuffLineOffsetBytes
			);
		}
	}
}

}	// namespace intrinsics_sse2_inout3b

}	// namespace gl

