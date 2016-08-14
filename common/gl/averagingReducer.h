#pragma once

#include "common.h"

#include <algorithm>

/*
	面積平均法（平均画素法）による画像縮小処理
*/

namespace gl
{

template <typename TargetColorT>
struct Single
{
	typedef typename TargetColorT TargetColorT;
	
	TargetColorT* p;
	Single(TargetColorT* p) : p(p) {}
	__forceinline void operator () (const TargetColorT& col)
	{
		*p += col;
		++p;
	}
};

template <typename TargetColorT>
struct Double
{
	typedef typename TargetColorT TargetColorT;
	TargetColorT* ptr1;
	const size_t mulFactor1;
	TargetColorT* ptr2;
	const size_t mulFactor2;
	Double(TargetColorT* ptr1, size_t mulFactor1, TargetColorT* ptr2, size_t mulFactor2)
		:
		ptr1(ptr1),
		mulFactor1(mulFactor1),
		ptr2(ptr2),
		mulFactor2(mulFactor2)
	{
	}
	__forceinline void operator () (const TargetColorT& col)
	{
		*ptr1++ += col * mulFactor1;
		*ptr2++ += col * mulFactor2;
	}
};

struct LineAveragingReducer_RatioNX
{
public:
	const size_t ratioSource;
	const size_t srcWidth;
	const size_t targetWidth;
	
	LineAveragingReducer_RatioNX(const size_t ratioSource, const size_t srcWidth)
		:
		ratioSource(ratioSource),
		srcWidth(srcWidth),
		targetWidth( srcWidth / ratioSource )
	{
	}
	
	template <typename SrcColorT, typename EffectorT, typename TargetColorT>
	void iterate(const SrcColorT* srcBuff, EffectorT& effector, TargetColorT bgColor)
	{
		typedef EffectorT::TargetColorT TargetColorT;
		for (size_t i=0; i<targetWidth; ++i) {
			TargetColorT tmp(0);
			for (size_t j=0; j<ratioSource/2; ++j) {
				tmp += *srcBuff++;
				tmp += *srcBuff++;
			}
			for (size_t j=0; j<ratioSource%2; ++j) {
				tmp += *srcBuff++;
			}
			effector(tmp);
		}
	}
};

// rare case
struct LineAveragingReducer_RatioPlus1
{
public:
	const size_t ratioTarget;
	const size_t targetWidth;
	
	LineAveragingReducer_RatioPlus1(const size_t ratioTarget, const size_t targetWidth)
		:
		ratioTarget(ratioTarget),
		targetWidth(targetWidth)
	{
	}

	template <typename SrcColorT, typename EffectorT, typename TargetColorT>
	void iterate(const SrcColorT* srcBuff, EffectorT& effector, TargetColorT bgColor)
	{
		typedef EffectorT::TargetColorT TargetColorT;
		for (size_t x=0; x<targetWidth; x+=ratioTarget) {
			size_t f1 = ratioTarget;
			size_t f2 = 1;
			TargetColorT c1 = *srcBuff; 
			for (size_t x2=0; x2<ratioTarget; ++x2) {
				TargetColorT c2 = *srcBuff; ++srcBuff;
				effector(c1 * f1 + c2 * f2);
				c1 = c2;
				--f1;
				++f2;
			}
		}
	}
};

struct LineAveragingReducer_RatioNXMinus1
{
public:
	const size_t ratioTarget;
	const size_t ratioSource;
	const size_t srcWidth;
	const size_t targetWidth;

	const size_t edgeBodyCnt;
	const size_t firstRatio;
	const size_t bodyMetamereFullPixelCnt;

	LineAveragingReducer_RatioNXMinus1(const size_t ratioTarget, const size_t ratioSource, const size_t srcWidth)
		:
		ratioTarget(ratioTarget),
		ratioSource(ratioSource),
		srcWidth(srcWidth),
		targetWidth( (srcWidth * ratioTarget) / ratioSource ),

		edgeBodyCnt( ratioSource / ratioTarget - 1 ),
		firstRatio( ratioSource % ratioTarget ),
		bodyMetamereFullPixelCnt( ratioSource / ratioTarget )
		
	{
		assert((ratioSource + 1) % ratioTarget == 0);
	}
	
	template <typename SrcColorT, typename EffectorT, typename TargetColorT>
	void iterate(const SrcColorT* srcBuff, EffectorT& effector, TargetColorT bgColor)
	{
		typedef EffectorT::TargetColorT TargetColorT; 
		size_t arrIdx = 0;
		
		const size_t mainIteCnt = ( targetWidth / ratioTarget );
		for (size_t x=0; x<mainIteCnt; ++x) {
			TargetColorT c1(0);
			{// head
				c1 = srcBuff[arrIdx++];
				TargetColorT c2(0);
				for (size_t i=0; i<edgeBodyCnt; ++i) {
					c2 += srcBuff[arrIdx++];
				}
				c2 += c1;
				c2 *= ratioTarget;
				TargetColorT c3 = srcBuff[arrIdx++];
				c1 = c3;
				c3 *= firstRatio;
				c2 += c3;
				effector(c2);
			}
			{// body
				size_t f1 = 1;
				size_t f2 = firstRatio - 1;
				const size_t bodyMetamereCnt = ( ratioTarget - 2 );
				for (size_t x2=0; x2<bodyMetamereCnt; ++x2) {
					TargetColorT c2(0);
					for (size_t i=0; i<bodyMetamereFullPixelCnt; ++i) {
						c2 += srcBuff[arrIdx++];
					}
					c2 *= ratioTarget;
					c1 *= f1;
					c2 += c1;
					TargetColorT c3 = srcBuff[arrIdx++];
					c1 = c3;
					c3 *= f2;
					c2 += c3;
					effector(c2);
					++f1;
					--f2;
				}
			}
			{// tail
				TargetColorT c2(0);
				for (int i=0; i<edgeBodyCnt; ++i) {
					c2 += srcBuff[arrIdx++];
				}
				c2 += srcBuff[arrIdx++];
				c2 *= ratioTarget;
				c1 *= (ratioTarget-1);
				c2 += c1;
				effector(c2);
			}
		}
		
		const size_t remainCnt = ( targetWidth % ratioTarget );
		if (remainCnt) {
			TargetColorT c1(0);
			{// head
				c1 = srcBuff[arrIdx++];
				TargetColorT c2(0);
				for (size_t i=0; i<edgeBodyCnt; ++i) {
					c2 += srcBuff[arrIdx++];
				}
				c2 += c1;
				c2 *= ratioTarget;
				TargetColorT c3 = srcBuff[arrIdx++];
				c1 = c3;
				c3 *= firstRatio;
				c2 += c3;
				effector(c2);
			}
			{// body
				size_t f1 = 1;
				size_t f2 = firstRatio - 1;
				for (size_t x2=0; x2<remainCnt-1; ++x2) {
					TargetColorT c2(0);
					for (size_t i=0; i<bodyMetamereFullPixelCnt; ++i) {
						c2 += srcBuff[arrIdx++];
					}
					c2 *= ratioTarget;
					c1 *= f1;
					c2 += c1;
					TargetColorT c3 = srcBuff[arrIdx++];
					c1 = c3;
					c3 *= f2;
					c2 += c3;
					effector(c2);
					++f1;
					--f2;
				}
			}
		}
	}
	
};

struct LineAveragingReducer_RatioNXPlus1
{
public:
	const size_t ratioTarget;
	const size_t ratioSource;
	const size_t srcWidth;
	const size_t targetWidth;

	const size_t centerLoopCnt;
	const size_t outerLoopCnt;
	const size_t remainCnt;

	LineAveragingReducer_RatioNXPlus1(const size_t ratioTarget, const size_t ratioSource, const size_t srcWidth)
		:
		ratioTarget(ratioTarget),
		ratioSource(ratioSource),
		srcWidth(srcWidth),
		targetWidth( (srcWidth * ratioTarget) / ratioSource ),

		centerLoopCnt( (ratioSource / ratioTarget) - 1 ),
		outerLoopCnt( targetWidth / ratioTarget ),
		remainCnt( targetWidth % ratioTarget)
	{
		assert((ratioSource - 1) % ratioTarget == 0);
	}

	template <typename SrcColorT, typename EffectorT, typename TargetColorT>
	void iterate(const SrcColorT* srcBuff, EffectorT& effector, TargetColorT bgColor)
	{
		typedef EffectorT::TargetColorT TargetColorT;
		size_t readIdx = 0;
		for (size_t i=0; i<outerLoopCnt; ++i) {
			TargetColorT c1 = srcBuff[readIdx++];
			for (size_t x2=0; x2<ratioTarget; ++x2) {
				TargetColorT c2(0);
				for (size_t j=0; j<centerLoopCnt; ++j) {
					c2 += srcBuff[readIdx++];
				}
				c2 *= ratioTarget;
				c1 *= (ratioTarget-x2);
				c2 += c1;
				TargetColorT c3 = srcBuff[readIdx++];
				c1 = c3;
				c3 *= (x2+1);
				c2 += c3;
				effector(c2);
			}
		}
		const size_t newIdx = readIdx*std::min(remainCnt,(size_t)1u);
		TargetColorT c1 = srcBuff[newIdx];
		++readIdx;
		for (size_t x=0; x<remainCnt; ++x) {
			TargetColorT c2(0);
			for (size_t i=0; i<centerLoopCnt; ++i) {
				c2 += srcBuff[readIdx++];
			}
			c2 *= ratioTarget;
			c1 *= (ratioTarget-x);
			c2 += c1;
			TargetColorT c3 = srcBuff[readIdx++];
			c1 = c3;
			c3 *= (x+1);
			c2 += c3;
			effector(c2);
		}
	}
	
};

struct LineAveragingReducer_RatioAny
{
	
public:
	const size_t ratioTarget;
	const size_t ratioSource;
	const size_t srcWidth;
	const size_t targetWidth;

	// precalculation
	const size_t ratioRemainder;
	const size_t endPartBodyIteCnt;
	
	const size_t ratioFirstBodyHead;
	const size_t ratioFirstBodyBodyIteCnt;
	const size_t ratioFirstBodyTail;
	
	const size_t numOfFullDots;
	const size_t imcompleteDotSamples;

	// 縮小サイズは4096ピクセル以下でなければいけない。場合により要調整。
	size_t collectParameters[4096];

	LineAveragingReducer_RatioAny(const size_t ratioTarget, const size_t ratioSource, const size_t srcWidth)
		:
		ratioTarget(ratioTarget),
		ratioSource(ratioSource),
		srcWidth(srcWidth),
		targetWidth( (srcWidth * ratioTarget) / ratioSource + ((srcWidth * ratioTarget) % ratioSource != 0) ),

//		assert(ratioSource % ratioTarget != 0);
		ratioRemainder( ratioSource % ratioTarget ),
		endPartBodyIteCnt( (ratioSource - ratioTarget - ratioRemainder) / ratioTarget ),	// end means head and tail
		
		ratioFirstBodyHead( ratioTarget - ratioRemainder ),
		ratioFirstBodyBodyIteCnt( (ratioSource - ratioFirstBodyHead) / ratioTarget ),
		ratioFirstBodyTail( (ratioSource - ratioFirstBodyHead) % ratioTarget ),
		
		numOfFullDots( (ratioTarget * (srcWidth % ratioSource /* remainSrcWidth */)) / ratioSource ),
		imcompleteDotSamples( (ratioTarget * (srcWidth % ratioSource /* remainSrcWidth */)) % ratioSource )
	{
		const size_t collectParametersCnt = std::max( (int(ratioTarget) - 2), (int(numOfFullDots)-1) ) + 1;
		assert(collectParametersCnt < 4096);
		size_t bodyBodyIteCnt = ratioFirstBodyBodyIteCnt;
		size_t ratioTail = ratioFirstBodyTail;
		for (size_t i=0; i<collectParametersCnt; ++i) {
			const size_t ratioHead = ratioTarget - ratioTail;
			const size_t withoutHead = (ratioSource - ratioHead);
			bodyBodyIteCnt = withoutHead / ratioTarget;
			ratioTail = withoutHead % ratioTarget;
			collectParameters[i] = bodyBodyIteCnt | (ratioTail << 16);
		}
	}

	template <typename TargetColorT, typename SrcColorT>
	__forceinline TargetColorT Any_CollectSample(
		TargetColorT& prevLastSample, const SrcColorT*& pSrc,
		const size_t headSampleCount, const size_t bodyIterationCount, const size_t bodySampleCount, const size_t tailSampleCount
	)
	{
		// head
		prevLastSample *= headSampleCount;
		register TargetColorT ret(0);
		ret += prevLastSample;

		if (bodyIterationCount == 0) {
			// tail
			prevLastSample = *pSrc++;
			ret += prevLastSample * tailSampleCount;
			return ret;
		}else {
			// body
			TargetColorT bodySampleSum(0);
			for (size_t i=0; i<bodyIterationCount; ++i) {
				bodySampleSum += *pSrc++;
			}
			bodySampleSum *= bodySampleCount;
			ret += bodySampleSum;
			// tail
			prevLastSample = *pSrc++;
			ret += prevLastSample * tailSampleCount;
			return ret;
		}
	}
	
	template <typename SrcColorT, typename EffectorT, typename TargetColorT>
	void iterate(const SrcColorT* srcBuff, EffectorT& effector, TargetColorT bgColor)
	{
		typedef EffectorT::TargetColorT TargetColorT; 
		const SrcColorT* pSrc = srcBuff;
		
		const size_t iteCnt = srcWidth / ratioSource;
		for (size_t i=0; i<iteCnt; ++i) {
			TargetColorT firstSample = *pSrc++;

			// HEAD 
			{
				effector(Any_CollectSample(firstSample, pSrc, ratioTarget, endPartBodyIteCnt, ratioTarget, ratioRemainder));
			}

			// BODY
			{
				size_t ratioHead = ratioFirstBodyHead;
				size_t bodyBodyIteCnt = ratioFirstBodyBodyIteCnt;
				size_t ratioTail = ratioFirstBodyTail;
				const size_t bodyIteCnt = (ratioTarget - 2);	// total - sides
				for (size_t j=0; j<bodyIteCnt; ++j) {
					effector(Any_CollectSample(firstSample, pSrc, ratioHead, bodyBodyIteCnt, ratioTarget, ratioTail));
					// next ratio
					ratioHead = ratioTarget - ratioTail;
					const size_t param = collectParameters[j];
					bodyBodyIteCnt = param & 0xFFFF;
					ratioTail = param >> 16;
				}
			}
			
			// TAIL
			{
				effector(Any_CollectSample(firstSample, pSrc, ratioRemainder, endPartBodyIteCnt, ratioTarget, ratioTarget));
			}
		}
		
		size_t ratioHead = ratioFirstBodyHead;
		TargetColorT firstSample(0);
		if (numOfFullDots || imcompleteDotSamples) {
			firstSample = *pSrc++;
		}
		if (numOfFullDots) {
			// HEAD 
			{
				effector(Any_CollectSample(firstSample, pSrc, ratioTarget, endPartBodyIteCnt, ratioTarget, ratioRemainder));
			}
			// BODY
			{
				size_t bodyBodyIteCnt = ratioFirstBodyBodyIteCnt;
				size_t ratioTail = ratioFirstBodyTail;
				for (size_t j=0; j<numOfFullDots-1; ++j) {
					effector(Any_CollectSample(firstSample, pSrc, ratioHead, bodyBodyIteCnt, ratioTarget, ratioTail));
					// calc next ratio
					ratioHead = ratioTarget - ratioTail;
					const size_t param = collectParameters[j];
					bodyBodyIteCnt = param & 0xFFFF;
					ratioTail = param >> 16;
				}
			}
		}
		if (imcompleteDotSamples) {
			// last tail
			const size_t headSampleCount = (imcompleteDotSamples < ratioHead) ? std::min(ratioRemainder, imcompleteDotSamples) : ratioHead;
			const size_t samplesLeft = imcompleteDotSamples - ratioHead;
			const size_t bodyIteCnt = samplesLeft / ratioTarget;
			effector(
				Any_CollectSample(firstSample, pSrc, headSampleCount, bodyIteCnt, ratioTarget, 0)
				+
				TmpColorT(bgColor) * (ratioSource - imcompleteDotSamples)
			);
		}
	}

};

template <typename SrcColorT, typename TargetColorT>
void AveragingReduce_RatioNXNX_IterateLine(const SrcColorT* srcBuff, TargetColorT* targetBuff, const size_t targetWidth, const size_t widthRatioSource)
{
	for (size_t x=0; x<targetWidth; ++x) {
		TmpColorT tmp(0);
		for (size_t j=0; j<widthRatioSource/2; ++j) {
			tmp += *srcBuff++;
			tmp += *srcBuff++;
		}
		for (size_t j=0; j<widthRatioSource%2; ++j) {
			tmp += *srcBuff++;
		}
		*targetBuff += tmp;
		++targetBuff;
	}
}

template <typename NumericT, typename SrcColorT, typename TargetColorT, typename TmpColorT, typename ToTargetColorConverterT, typename ProgressReporterT>
bool AveragingReduce_RatioNXNX(
	const size_t widthRatioSource, const size_t heightRatioSource,
	const SrcColorT* srcBuff, const size_t srcWidth, const size_t srcHeight, const int srcLineOffset,
	TargetColorT* targetBuff, const int targetLineOffset,
	TmpColorT* tmpBuff, TargetColorT bgColor,
	ToTargetColorConverterT& converter,
	ProgressReporterT& progressReporter
	)
{
	const size_t targetHeight = srcHeight / heightRatioSource;
	
	if (widthRatioSource == 1) {
		if (heightRatioSource == 1) {
			for (size_t y=0; y<srcHeight; ++y) {
				// convert
				for (size_t x=0; x<srcWidth; ++x) {
					targetBuff[x] = converter(srcBuff[x]);
				}
				OffsetPtr(srcBuff, srcLineOffset);
				OffsetPtr(targetBuff, targetLineOffset);
				if (!progressReporter(y))
					return false;
			}
		}else {
			const NumericT invertRatioSource = NumericT(1.0 / heightRatioSource);
			for (size_t y=0; y<targetHeight; ++y) {
				for (size_t i=0; i<heightRatioSource; ++i) {
					const SrcColorT* tmpSrc = srcBuff;
					TmpColorT* tmpTmp = tmpBuff;
					for (size_t x=0; x<srcWidth; ++x) {
						*tmpTmp++ += *tmpSrc++;
					}
					OffsetPtr(srcBuff, srcLineOffset);
				}
				// convert
				for (size_t x=0; x<srcWidth; ++x) {
					tmpBuff[x] *= invertRatioSource;
					targetBuff[x] = converter(tmpBuff[x]);
					tmpBuff[x] *= 0;
				}
				OffsetPtr(targetBuff, targetLineOffset);
				if (!progressReporter(y))
					return false;
			}
		}
	}else {
		const size_t targetWidth = srcWidth / widthRatioSource;
		const NumericT invertRatioSource = NumericT(1.0 / (heightRatioSource*widthRatioSource));
		for (size_t y=0; y<targetHeight; ++y) {
			for (size_t i=0; i<heightRatioSource; ++i) {
				AveragingReduce_RatioNXNX_IterateLine(srcBuff, tmpBuff, targetWidth, widthRatioSource);
				OffsetPtr(srcBuff, srcLineOffset);
			}
			// convert
			for (size_t x=0; x<targetWidth; ++x) {
				tmpBuff[x] *= invertRatioSource;
				targetBuff[x] = converter(tmpBuff[x]);
				tmpBuff[x] *= 0;
			}
			OffsetPtr(targetBuff, targetLineOffset);
			if (!progressReporter(y))
				return false;
		}
	}
	return true;
}

// target * n = source
template <typename NumericT, typename LineAveragingReducerT, typename SrcColorT, typename TargetColorT, typename TmpColorT, typename ToTargetColorConverterT, typename ProgressReporterT>
bool AveragingReduce_RatioNX(
	LineAveragingReducerT& lineReducer,
	const size_t heightRatioSource,
	const SrcColorT* srcBuff, const size_t srcHeight, const int srcLineOffset,
	TargetColorT* targetBuff, const int targetLineOffset,
	TmpColorT* tmpBuff, TargetColorT bgColor,
	ToTargetColorConverterT& converter,
	ProgressReporterT& progressReporter
	)
{
	const NumericT invertRatioSource = NumericT(1.0 / (heightRatioSource*lineReducer.ratioSource));
	const size_t targetHeight = srcHeight / heightRatioSource;

	for (size_t y=0; y<targetHeight; ++y) {
		for (size_t i=0; i<heightRatioSource; ++i) {
			lineReducer.iterate(srcBuff, Single<TmpColorT>(tmpBuff), bgColor);
			OffsetPtr(srcBuff, srcLineOffset);
		}
		// convert
		for (size_t x=0; x<lineReducer.targetWidth; ++x) {
			tmpBuff[x] *= invertRatioSource;
			targetBuff[x] = converter(tmpBuff[x]);
			tmpBuff[x] *= 0;
		}
		OffsetPtr(targetBuff, targetLineOffset);
		if (!progressReporter(y))
			return false;
	}
	return true;
}

template <typename NumericT, typename TargetColorT, typename TmpColorT, typename ToTargetColorConverterT>
void StoreToTarget(
	const size_t targetWidth,
	const NumericT invertRatioSource,
	const NumericT ratioTarget2,
	TargetColorT* targetBuff,
	TmpColorT* __restrict tmpBuffMain,
	TmpColorT* __restrict tmpBuffBody,
	ToTargetColorConverterT& converter)
{
	for (size_t x=0; x<targetWidth; ++x) {
		// 1. (body * ratioTarget + main) * invertRatioSource
		// 2. body * (ratioTarget * invertRatioSource) + main * invertRatioSource
		// 1の方法だと桁溢れする事があるので、2にする。
		/* 1
		TmpColorT c(0);
		c += tmpBuffBody[x];
		tmpBuffBody[x] *= 0;
		c *= ratioTarget;
		c += tmpBuffMain[x];
		tmpBuffMain[x] *= 0;
		c *= invertRatioSource;
		*/
		TmpColorT c;
		c = tmpBuffBody[x];
		c *= ratioTarget2;
		c += tmpBuffMain[x] * invertRatioSource;
		tmpBuffBody[x] *= 0;
		tmpBuffMain[x] *= 0;
		targetBuff[x] = converter(c);
	}
}

template <typename NumericT, typename LineAveragingReducerT, typename SrcColorT, typename TargetColorT, typename TmpColorT, typename ToTargetColorConverterT, typename ProgressReporterT>
bool AveragingReduce_RatioAny(
	LineAveragingReducerT& lineReducer,
	const size_t heightRatioTarget, const size_t heightRatioSource,
	const SrcColorT* srcBuff, const size_t srcHeight, int srcLineOffset,
	TargetColorT* targetBuff, const int targetLineOffset,
	TmpColorT* tmpBuff, TargetColorT bgColor,
	ToTargetColorConverterT& converter,
	ProgressReporterT& progressReporter
)
{
	assert(heightRatioTarget != 1);
	
	const size_t widthRatioSource = lineReducer.ratioSource;
	const NumericT invertRatioSource = 1.0 / (widthRatioSource * heightRatioSource);
//	const size_t targetHeight = (srcHeight * heightRatioTarget) / heightRatioSource;
	
	const size_t ratioSource = heightRatioSource;
	const size_t ratioTarget = heightRatioTarget;
	const NumericT ratioTarget2 = heightRatioTarget * double(invertRatioSource);
	const size_t ratioRemainder = ratioSource % ratioTarget;
	
	assert(ratioSource > ratioTarget);
	assert(ratioSource - ratioTarget - ratioRemainder >= 0);
	const size_t endPartBodyIteCnt = (ratioSource - ratioTarget - ratioRemainder) / ratioTarget;	// end means head and tail
	const size_t ratioFirstBodyHead = ratioTarget - ratioRemainder;
	const size_t ratioFirstBodyBodyIteCnt = (ratioSource - ratioFirstBodyHead) / ratioTarget;
	const size_t ratioFirstBodyTail = (ratioSource - ratioFirstBodyHead) % ratioTarget;
	
	const SrcColorT* pReadLine = srcBuff;
	const size_t targetWidth = lineReducer.targetWidth;
	TmpColorT* tmpBuffMain = tmpBuff;
	TmpColorT* tmpBuffBody = tmpBuff + targetWidth;
	TmpColorT* tmpBuffSub = tmpBuff + targetWidth + targetWidth;
	
	const size_t mainIteCnt = srcHeight / ratioSource;
	const size_t remainSrcHeight = srcHeight % ratioSource;
	const size_t remainNumOfFullLines = (ratioTarget * remainSrcHeight) / ratioSource;
	const size_t imcompleteLineSamples = (ratioTarget * remainSrcHeight) % ratioSource;
	size_t lineCnt = 0;
	for (size_t i=0; i<mainIteCnt; ++i) {
		size_t ratioNextHead = ratioFirstBodyHead;
		
		// HEAD
		{
			// head + body
			for (size_t j=0; j<1+endPartBodyIteCnt; ++j) {
				lineReducer.iterate(pReadLine, Single<TmpColorT>(tmpBuffBody), bgColor);
				OffsetPtr(pReadLine, srcLineOffset);
			}
			// tail and next head
			lineReducer.iterate(pReadLine, Double<TmpColorT>(tmpBuffMain, ratioRemainder, tmpBuffSub, ratioNextHead), bgColor);
			OffsetPtr(pReadLine, srcLineOffset);
			
			StoreToTarget(targetWidth, invertRatioSource, ratioTarget2, targetBuff, tmpBuffMain, tmpBuffBody, converter);
			std::swap(tmpBuffMain, tmpBuffSub);
			OffsetPtr(targetBuff, targetLineOffset);
		}
		if (!progressReporter(lineCnt++)) {
			return false;
		}
		
		// BODY
		{
			size_t bodyBodyIteCnt = ratioFirstBodyBodyIteCnt;
			size_t ratioTail = ratioFirstBodyTail;
			const size_t bodyIteCnt = ratioTarget - 2;	// total - sides
			for (size_t j=0; j<bodyIteCnt; ++j) {
				// body
				for (size_t k=0; k<bodyBodyIteCnt; ++k) {
					lineReducer.iterate(pReadLine, Single<TmpColorT>(tmpBuffBody), bgColor);
					OffsetPtr(pReadLine, srcLineOffset);
				}
				// tail and next head
				ratioNextHead = ratioTarget - ratioTail;
				lineReducer.iterate(pReadLine, Double<TmpColorT>(tmpBuffMain, ratioTail, tmpBuffSub, ratioNextHead), bgColor);
				OffsetPtr(pReadLine, srcLineOffset);
				
				// convert
				StoreToTarget(targetWidth, invertRatioSource, ratioTarget2, targetBuff, tmpBuffMain, tmpBuffBody, converter);
				std::swap(tmpBuffMain, tmpBuffSub);
				OffsetPtr(targetBuff, targetLineOffset);
				
				// calc next ratio
				size_t withoutHead = (ratioSource - ratioNextHead);
				bodyBodyIteCnt = withoutHead / ratioTarget;
				ratioTail = withoutHead % ratioTarget;
				
				if (!progressReporter(lineCnt++)) {
					return false;
				}
			}
		}
		
		// TAIL
		{
			// body + tail
			for (size_t j=0; j<endPartBodyIteCnt+1; ++j) {
				lineReducer.iterate(pReadLine, Single<TmpColorT>(tmpBuffBody), bgColor);
				OffsetPtr(pReadLine, srcLineOffset);
			}
			// convert
			StoreToTarget(targetWidth, invertRatioSource, ratioTarget2, targetBuff, tmpBuffMain, tmpBuffBody, converter);
			std::swap(tmpBuffMain, tmpBuffSub);
			OffsetPtr(targetBuff, targetLineOffset);
		}
		if (!progressReporter(lineCnt++)) {
			return false;
		}

	}

	size_t ratioNextHead = ratioFirstBodyHead;
	if (remainNumOfFullLines) {

		// HEAD
		{
			// head + body
			for (size_t j=0; j<1+endPartBodyIteCnt; ++j) {
				lineReducer.iterate(pReadLine, Single<TmpColorT>(tmpBuffBody), bgColor);
				OffsetPtr(pReadLine, srcLineOffset);
			}
			// tail and next head
			lineReducer.iterate(pReadLine, Double<TmpColorT>(tmpBuffMain, ratioRemainder, tmpBuffSub, ratioNextHead), bgColor);
			OffsetPtr(pReadLine, srcLineOffset);
			
			// convert
			StoreToTarget(targetWidth, invertRatioSource, ratioTarget2, targetBuff, tmpBuffMain, tmpBuffBody, converter);
			std::swap(tmpBuffMain, tmpBuffSub);
			OffsetPtr(targetBuff, targetLineOffset);
		}
		if (!progressReporter(lineCnt++)) {
			return false;
		}
		
		{
			// BODY
			size_t bodyBodyIteCnt = ratioFirstBodyBodyIteCnt;
			size_t ratioTail = ratioFirstBodyTail;
			for (size_t j=0; j<remainNumOfFullLines-1; ++j) {
				// body
				for (size_t k=0; k<bodyBodyIteCnt; ++k) {
					lineReducer.iterate(pReadLine, Single<TmpColorT>(tmpBuffBody), bgColor);
					OffsetPtr(pReadLine, srcLineOffset);
				}
				// tail and next head
				ratioNextHead = ratioTarget - ratioTail;
				lineReducer.iterate(pReadLine, Double<TmpColorT>(tmpBuffMain, ratioTail, tmpBuffSub, ratioNextHead), bgColor);
				OffsetPtr(pReadLine, srcLineOffset);
				
				// convert
				StoreToTarget(targetWidth, invertRatioSource, ratioTarget2, targetBuff, tmpBuffMain, tmpBuffBody, converter);
				std::swap(tmpBuffMain, tmpBuffSub);
				OffsetPtr(targetBuff, targetLineOffset);
				
				// calc next ratio
				const size_t withoutHead = (ratioSource - ratioNextHead);
				bodyBodyIteCnt = withoutHead / ratioTarget;
				ratioTail = withoutHead % ratioTarget;

				if (!progressReporter(lineCnt++)) {
					return false;
				}
			}
		}
	}

	// 全てのsampleが残っていないlineがある場合はそれを処理する。
	if (imcompleteLineSamples) {
		// TAIL
		{
			// body
			const size_t samplesLeft = imcompleteLineSamples - ratioNextHead;
			const size_t iteCnt = samplesLeft / ratioTarget;
			const size_t lastRemain = samplesLeft % ratioTarget;
			for (size_t j=0; j<iteCnt; ++j) {
				lineReducer.iterate(pReadLine, Single<TmpColorT>(tmpBuffBody), bgColor);
				OffsetPtr(pReadLine, srcLineOffset);
			}
			const size_t ratioBG = ratioSource - imcompleteLineSamples;
			const TmpColorT tmpBGColor = TmpColorT(bgColor) * (ratioTarget * ratioBG * ratioSource) / ratioTarget;
			// 残りの部分は背景色で補充
			for (size_t i=0; i<targetWidth; ++i) {
				tmpBuffMain[i] += tmpBGColor;
			}
			// tail
			// 本当は残ってる割合の分だけ処理しないと駄目
//			AveragingReduceLine_Any(lineCalcParam, pReadLine, tmpBuffMain, ratioTarget);
//			OffsetPtr(pReadLine, srcLineOffset);
			
			// convert
			StoreToTarget(targetWidth, invertRatioSource, ratioTarget2, targetBuff, tmpBuffMain, tmpBuffBody, converter);
//			std::swap(tmpBuffMain, tmpBuffSub);
//			OffsetPtr(targetBuff, targetLineOffset);
			if (!progressReporter(lineCnt++)) {
				return false;
			}
		}
	}
	return true;
}

template <typename NumericT, typename SrcColorT, typename TargetColorT, typename TmpColorT, typename ToTargetColorConverterT, typename ProgressReporterT>
bool AveragingReduce(
	size_t widthRatioTarget, size_t widthRatioSource,
	size_t heightRatioTarget, size_t heightRatioSource,
	const SrcColorT* srcBuff, const size_t srcWidth, const size_t srcHeight, const int srcLineOffset,
	TargetColorT* targetBuff, const int targetLineOffset,
	TmpColorT* tmpBuff, TargetColorT bgColor,
	ToTargetColorConverterT& converter,
	ProgressReporterT& progressReporter
)
{
	assert(widthRatioTarget <= widthRatioSource);
	assert(heightRatioTarget <= heightRatioSource);
	
	if (widthRatioTarget == 1) {
		if (heightRatioTarget == 1) {
			return AveragingReduce_RatioNXNX<NumericT>(
				widthRatioSource, heightRatioSource,
				srcBuff, srcWidth, srcHeight, srcLineOffset,
				targetBuff, targetLineOffset,
				tmpBuff, bgColor,
				converter,
				progressReporter
			);
		}
		else {
			LineAveragingReducer_RatioNX lineReducer(widthRatioSource, srcWidth);
			return AveragingReduce_RatioAny<NumericT>(
				lineReducer,
				heightRatioTarget, heightRatioSource,
				srcBuff, srcHeight, srcLineOffset,
				targetBuff, targetLineOffset,
				tmpBuff, bgColor,
				converter,
				progressReporter
			);
		}
	}
	else if ((widthRatioSource - 1) % widthRatioTarget == 0) {
		LineAveragingReducer_RatioNXPlus1 lineReducer(widthRatioTarget, widthRatioSource, srcWidth);
		if (heightRatioTarget == 1) {
			return AveragingReduce_RatioNX<NumericT>(
				lineReducer,
				heightRatioSource,
				srcBuff, srcHeight, srcLineOffset,
				targetBuff, targetLineOffset,
				tmpBuff, bgColor,
				converter,
				progressReporter
			);
		}else {
			return AveragingReduce_RatioAny<NumericT>(
				lineReducer,
				heightRatioTarget, heightRatioSource,
				srcBuff, srcHeight, srcLineOffset,
				targetBuff, targetLineOffset,
				tmpBuff, bgColor,
				converter,
				progressReporter
			);
		}
	}	
	else if ((widthRatioSource + 1) % widthRatioTarget == 0) {
		LineAveragingReducer_RatioNXMinus1 lineReducer(widthRatioTarget, widthRatioSource, srcWidth);
		if (heightRatioTarget == 1) {
			return AveragingReduce_RatioNX<NumericT>(
				lineReducer,
				heightRatioSource,
				srcBuff, srcHeight, srcLineOffset,
				targetBuff, targetLineOffset,
				tmpBuff, bgColor,
				converter,
				progressReporter
			);
		}else {
			return AveragingReduce_RatioAny<NumericT>(
				lineReducer,
				heightRatioTarget, heightRatioSource,
				srcBuff, srcHeight, srcLineOffset,
				targetBuff, targetLineOffset,
				tmpBuff, bgColor,
				converter,
				progressReporter
			);
		}
	}
	else {
		LineAveragingReducer_RatioAny lineReducer(widthRatioTarget, widthRatioSource, srcWidth);
		if (heightRatioTarget == 1) {
			return AveragingReduce_RatioNX<NumericT>(
				lineReducer,
				heightRatioSource,
				srcBuff, srcHeight, srcLineOffset,
				targetBuff, targetLineOffset,
				tmpBuff, bgColor,
				converter,
				progressReporter
			);
		}else {
			return AveragingReduce_RatioAny<NumericT>(
				lineReducer,
				heightRatioTarget, heightRatioSource,
				srcBuff, srcHeight, srcLineOffset,
				targetBuff, targetLineOffset,
				tmpBuff, bgColor,
				converter,
				progressReporter
			);
		}
	}
}



}	// namespace gl
