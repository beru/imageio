#pragma once

/*
	画像リサンプリング処理
*/

#define _USE_MATH_DEFINES
#include <math.h>

#include "mathutil.h"

namespace gl
{

template <typename NumericT, typename SrcColorT, typename TargetColorT, typename LineSamplerT, typename ProgressReporterT>
void ResampleImage(
	const SrcColorT* pSrc, size_t srcWidth, size_t srcHeight, int srcLineOffset,
	TargetColorT* pTarget, size_t targetWidth, size_t targetHeight, int targetLineOffset,
	LineSamplerT& lineSampler, ProgressReporterT& progressReporter)
{
	const NumericT xScale = NumericT(srcWidth) / NumericT(targetWidth);
	const NumericT yScale = NumericT(srcHeight) / NumericT(targetHeight);
	
	for (size_t y=0; y<targetHeight; ++y) {
		lineSampler(y, xScale, yScale, targetWidth, srcLineOffset, pSrc, pTarget);
		if (!progressReporter(y, targetHeight)) {
			break;
		}
		OffsetPtr(pTarget, targetLineOffset);
	}
}

class LineSampler_NearestNeighbor
{
public:
	template <typename NumericT, typename SrcColorT, typename TargetColorT>
	__forceinline void operator () (
		const size_t y, const NumericT xScale, const NumericT yScale, const size_t targetWidth, const int srcLineOffset,
		const SrcColorT* pSrcLine, TargetColorT* pTargetLine
		)
	{
		const SrcColorT* pSrc = pSrcLine;
		const int offsetBytes = srcLineOffset * int(NumericT(y) * yScale + NumericT(0.5));
		OffsetPtr(pSrc, offsetBytes);
		TargetColorT* pTarget = pTargetLine;
		for (size_t x=0; x<targetWidth; ++x) {
			int offset = (xScale * NumericT(x)) + NumericT(0.5);
			*pTarget = *(pSrc + offset);
			++pTarget;
		}
	}
};

template <typename TmpColorT>
class LineSampler_Bilinear
{
public:
	template <typename NumericT, typename SrcColorT, typename TargetColorT>
	__forceinline void operator () (
		const size_t y, const NumericT xScale, const NumericT yScale, const size_t targetWidth, const int srcLineOffset,
		const SrcColorT* pSrcLine, TargetColorT* pTargetLine
		)
	{
		const NumericT srcY = NumericT(y) * yScale;
		const SrcColorT* pSrc = pSrcLine;
		OffsetPtr(pSrc, srcLineOffset * int(srcY));
		TargetColorT* pTarget = pTargetLine;

		NumericT fracY1 = frac(srcY);
		NumericT fracY2 = NumericT(1.0) - fracY1;
		
		for (size_t x=0; x<targetWidth; ++x) {
			const NumericT srcX = NumericT(x) * xScale;
			int ix = srcX;
			const SrcColorT* pSrc1 = pSrc + ix;
			const SrcColorT* pSrc2 = pSrc1; OffsetPtr(pSrc1, srcLineOffset);
			TmpColorT p1 = *pSrc1;
			TmpColorT p2 = *(pSrc1 + 1);
			TmpColorT p3 = *pSrc2;
			TmpColorT p4 = *(pSrc2 + 1);
			NumericT fracX1 = frac(srcX);
			NumericT fracX2 = NumericT(1.0) - fracX1;
			
			*pTarget =	(p1 * (fracY2 * fracX2)) + (p2 * (fracY2 * fracX1)) +
						(p3 * (fracY1 * fracX2)) + (p4 * (fracY1 * fracX1));
			++pTarget;
		}
	}
};


template <typename T>
inline T sinc(T x)
{
	T tmp = M_PI * x;
	return sin(tmp) / tmp;
}

template <typename T>
struct lanczos3_function
{
	static T width() { return 3.0; }
	__forceinline T operator() (T x)
	{
		if (x <= std::numeric_limits<T>::epsilon()) {
			return T(1.0);
		}else if (x < T(3.0)) {
			return sinc(x) * sinc(x / T(3.0));
		}else {
			return T(0.0);
		}
	}
};

template <typename T>
struct lanczos2_function
{
	static T width() { return 2.0; }
	__forceinline T operator() (T x)
	{
		if (x <= std::numeric_limits<T>::epsilon()) {
			return T(1.0);
		}else if (x < T(2.0)) {
			return sinc(x) * sinc(x / T(2.0));
		}else {
			return T(0.0);
		}
	}
};

struct ResampleRange
{
	size_t start;
	size_t end;
};

template <typename NumericT>
struct ResampleTable
{
	ResampleRange ranges[32768];
	NumericT factors[32768 * 4];
};

template <typename NumericT, typename FilterT>
void SetupResampleTable(
	const size_t sourceLength, const size_t targetLength,
	const size_t sourceRatio, const size_t targetRatio,
	FilterT& filter, ResampleTable<NumericT>& resampleTable
	)
{
	const int count = 256 * filter.width();
	double sum = 0;
	for (int i=-count; i<count; ++i) {
		sum += filter(abs(i/256.0));
	}
	const double adjustRatio = 256.0 / sum;

	const double sourceTargetRatio = double(sourceRatio) / targetRatio;
	const double targetSourceRatio = double(targetRatio) / sourceRatio;
	
	double targetCenter = 0.5;
	double targetStart = targetCenter - filter.width();
	double targetEnd = targetCenter + filter.width();
	
	double srcStart = targetStart * sourceTargetRatio;
	double srcEnd = targetEnd * sourceTargetRatio;
	
	size_t factorIdx = 0;
	
	static const size_t SEGMENTS_COUNT = 16;
	static const double INVERSED_SEGMENTS_COUNT_X2 = 1.0 / (SEGMENTS_COUNT * 2);
	const double step = targetSourceRatio / SEGMENTS_COUNT;
	for (size_t i=0; i<targetLength; ++i) {
		const size_t fixedStart = std::max(srcStart, 0.0);
		const size_t fixedEnd = std::min(size_t(srcEnd)+1, sourceLength - 1);
		double targetPos = targetSourceRatio * fixedStart;

		ResampleRange& range = resampleTable.ranges[i];
		range.start = fixedStart;
		range.end = fixedEnd;
		
		double factor = filter(abs(targetCenter - targetPos));
		for (size_t j=fixedStart; j<fixedEnd; ++j) {
			double totalFactor = 0;
			for (size_t k=0; k<SEGMENTS_COUNT; ++k) {
				double nextFactor = filter(abs(targetCenter - targetPos));
				totalFactor += factor + nextFactor;
				factor = nextFactor;
				targetPos += step;
			}
			resampleTable.factors[factorIdx] = totalFactor * adjustRatio * INVERSED_SEGMENTS_COUNT_X2 * targetSourceRatio;
			++factorIdx;
		}
		targetCenter += 1.0;
		targetStart += 1.0;
		targetEnd += 1.0;
		srcStart += sourceTargetRatio;
		srcEnd += sourceTargetRatio;
	}
}

template <typename TargetColorT, typename SrcColorT>
inline TargetColorT convert(SrcColorT src)
{
	TargetColorT ret;
	ret.r = TargetColorT::value_type( limitValue(src.r, SrcColorT::value_type(0), gl::OneMinusEpsilon(SrcColorT::value_type(0))) );
	ret.g = TargetColorT::value_type( limitValue(src.g, SrcColorT::value_type(0), gl::OneMinusEpsilon(SrcColorT::value_type(0))) );
	ret.b = TargetColorT::value_type( limitValue(src.b, SrcColorT::value_type(0), gl::OneMinusEpsilon(SrcColorT::value_type(0))) );
	return ret;
}

template <typename SrcColorT, typename TargetColorT, typename TmpColorT, typename NumericT, typename FilterT>
void ResampleImage(
	const SrcColorT* srcBuff, const size_t srcWidth, const size_t srcHeight, const int srcLineOffsetBytes,
	TargetColorT* targetBuff, const int targetLineOffsetBytes,
	TmpColorT** tmpBuffs,
	const size_t srcWidthRatio, const size_t targetWidthRatio,
	const size_t srcHeightRatio, const size_t targetHeightRatio,
	FilterT& filter, ResampleTable<NumericT>& resampleTable
	)
{
	const size_t widthMul = srcWidth * targetWidthRatio;
	const size_t targetWidth = (widthMul / srcWidthRatio) + ((widthMul % srcWidthRatio) ? 1 : 0);
	assert(targetWidth != 0);
	
	const size_t heightMul = srcHeight * targetHeightRatio;
	const size_t targetHeight = (heightMul / srcHeightRatio) + ((heightMul % srcHeightRatio) ? 1 : 0);
	assert(targetHeight != 0);
	
	if (srcWidthRatio < targetWidthRatio) {
		return;
	}
	if (srcHeightRatio < targetHeightRatio) {
		return;
	}
	
	// horizontal decimation
	{
		const SrcColorT* pSrc = srcBuff;
		if (srcWidthRatio == targetWidthRatio) {
			for (size_t y=0; y<srcHeight; ++y) {
				TmpColorT* pTmp = tmpBuffs[y];
				for (size_t x=0; x<targetWidth; ++x) {
					pTmp[x] = pSrc[x];
				}
				OffsetPtr(pSrc, srcLineOffsetBytes);
			}
		}else {
			SetupResampleTable(srcWidth, targetWidth, srcWidthRatio, targetWidthRatio, filter, resampleTable);
			for (size_t y=0; y<srcHeight; ++y) {
				size_t factorIdx = 0;
				TmpColorT* pTmp = tmpBuffs[y];
				for (size_t x=0; x<targetWidth; ++x) {
					const ResampleRange& range = resampleTable.ranges[x];
					TmpColorT tmp(0);
					for (size_t i=range.start; i<range.end; ++i) {
						tmp += TmpColorT(pSrc[i]) * resampleTable.factors[factorIdx];
						++factorIdx;
					}
					pTmp[x] = tmp;
				}
				OffsetPtr(pSrc, srcLineOffsetBytes);
			}
		}
	}
	
	// vertical decimation
	{
		TargetColorT* pTarget = targetBuff;
		if (srcHeightRatio == targetHeightRatio) {
			for (size_t y=0; y<targetHeight; ++y) {
				const TmpColorT* pTmp = tmpBuffs[y];
				for (size_t x=0; x<targetWidth; ++x) {
					pTarget[x] = convert<TargetColorT>(pTmp[x]);
				}
				OffsetPtr(pTarget, targetLineOffsetBytes);
			}
		}else {
			SetupResampleTable(srcHeight, targetHeight, srcHeightRatio, targetHeightRatio, filter, resampleTable);
			for (size_t x=0; x<targetWidth; ++x) {
				pTarget = targetBuff;
				size_t factorIdx = 0;
				for (size_t y=0; y<targetHeight; ++y) {
					const ResampleRange& range = resampleTable.ranges[y];
					TmpColorT tmp(0);
					for (size_t i=range.start; i<range.end; ++i) {
						tmp += tmpBuffs[i][x] * resampleTable.factors[factorIdx];
						++factorIdx;
					}
					pTarget[x] = convert<TargetColorT>(tmp);
					OffsetPtr(pTarget, targetLineOffsetBytes);
				}
			}
		}
	}
}

} // namespace gl
