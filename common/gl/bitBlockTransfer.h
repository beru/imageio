#pragma once

#include "arrayutil.h"
#include "gl/rect.h"
#include "gl/buffer2D_Bit.h"

namespace gl {

template <typename ConverterT, typename TargetColorT>
void BitBlockTransfer(
	const Buffer2D<bool>& src, Buffer2D<TargetColorT>& target,
	int srcX, int srcY, int targetX, int targetY,
	size_t width, size_t height,
	ConverterT& converter)
{
	// TODO:
	const Buffer2D<bool>::value_type* pSrc = src.GetPixelPtr(0, 0);
	TargetColorT* pTarget = target.GetPixelPtr(0, 0);
	int srcLineOffset = src.GetLineOffset();
	int targetLineOffset = target.GetLineOffset();
	
	size_t srcWidth = src.GetWidth();
	size_t srcHeight = src.GetHeight();
	const size_t VALUE_BITS = sizeof(Buffer2D<bool>::value_type) * 8;
	Buffer2D<bool>::value_type mask = 0;
	for (size_t y=0; y<srcHeight; ++y) {
		const Buffer2D<bool>::value_type* pSrc2 = pSrc;
		TargetColorT* pTarget2 = pTarget;
		for (size_t x=0; x<srcWidth; x+=VALUE_BITS) {
			const Buffer2D<bool>::value_type srcVal = *pSrc2;
			for (size_t i=0; i<VALUE_BITS; ++i) {
				mask = (1 << (VALUE_BITS-1-i));
				*pTarget2 = converter((srcVal & mask) != 0);
				++pTarget2;
			}
			++pSrc2;
		}
		OffsetPtr(pSrc, srcLineOffset);
		OffsetPtr(pTarget, targetLineOffset);
	}
}


// TODO: need to fix. runs too slow...
template <typename ConverterT, typename SrcColorT, typename TargetColorT>
void BitBlockTransfer(
	const Buffer2D<SrcColorT>& src, Buffer2D<TargetColorT>& target,
	int srcX, int srcY, int targetX, int targetY,
	size_t width, size_t height,
	ConverterT& converter
	)
{
	// <KISS>
	if (0
		|| width == 0
		|| height == 0
		|| srcX >= int(src.GetWidth())
		|| srcX + width < 0
		|| srcY >= int(src.GetHeight())
		|| srcY + height < 0
		|| targetX >= int(target.GetWidth())
		|| targetX + width < 0
		|| targetY >= int(target.GetHeight())
		|| targetY + height < 0
	)
		return;
	
	// parameter fix
	if (srcX < 0) {
		width += srcX;
		targetX -= srcX;
		srcX = 0;
	}
	if (targetX < 0) {
		width += targetX;
		srcX -= targetX;
		targetX = 0;
	}
	if (srcX + width > src.GetWidth())
		width -= srcX + width - src.GetWidth();
	if (targetX + width > target.GetWidth())
		width -= targetX + width - target.GetWidth();
	if (srcY < 0) {
		height += srcY;
		targetY -= srcY;
		srcY = 0;
	}
	if (targetY < 0) {
		height += targetY;
		srcY -= targetY;
		targetY = 0;
	}
	if (srcY + height > src.GetHeight())
		height -= srcY + height - src.GetHeight();
	if (targetY + height > target.GetHeight())
		height -= targetY + height - target.GetHeight();
	// </KISS>
	
	const SrcColorT* pSrc = src.GetPixelPtr(srcX, srcY);
	TargetColorT* pTarget = target.GetPixelPtr(targetX, targetY);
	int srcLineOffset = src.GetLineOffset();
	int targetLineOffset = target.GetLineOffset();
	
	const SrcColorT* pSrc2;
	TargetColorT* pTarget2;
	for (size_t y=0; y<height; ++y) {
		pSrc2 = pSrc;
		pTarget2 = pTarget;
		for (size_t x=0; x<width; ++x) {
			*pTarget2 = converter(*pSrc2);
			++pSrc2;
			++pTarget2;
		}
		OffsetPtr(pSrc, srcLineOffset);
		OffsetPtr(pTarget, targetLineOffset);
	}

}

// nearest neighbor
template <typename NumericT, typename Converter, typename SrcColorT, typename TargetColorT>
void StretchBitBlockTransfer(
	const Buffer2D<SrcColorT>& src, const Rect<NumericT> srcRect,
	Buffer2D<TargetColorT>& target, const Rect<NumericT> targetRect,
	Converter& converter
	)
{
	double xa, xb, ya, yb;

	SolveSimultaneousLinearEquations(
		targetRect.x,				srcRect.x,
		targetRect.x+targetRect.w,	srcRect.x+srcRect.w,
		xa,	xb);
	SolveSimultaneousLinearEquations(
		targetRect.y,				srcRect.y,
		targetRect.y+targetRect.h,	srcRect.y+srcRect.h,
		ya, yb);
	
	char* pTargetLine_Char = (char*) target.GetPixelPtr(targetRect.x, targetRect.y);
	int targetLineBytes = target.GetLineOffset();
	double startX = targetRect.x;
	double startY = targetRect.y;
	double curY = startY;
	for (size_t y=0; y<targetRect.h; ++y) {
		TargetColorT* pTarget = (TargetColorT*) pTargetLine_Char;
		double srcX = startX * xa + xb;
		double srcY = curY * ya + yb;
		const SrcColorT* pSrc = src.GetPixelPtr(srcX, srcY);
		for (size_t x=0; x<targetRect.w; ++x) {
			const SrcColorT* pSample = pSrc + int(x * xa);
			*pTarget = converter(*pSample);
			++pTarget;
		}
		pTargetLine_Char += targetLineBytes;
		++curY;
	}
	
}

#if 0
inline void StretchBitBlockTransfer(
	int srcWidth, int srcHeight, int srcX, int srcY, int srcWidth2, int srcHeight2, int srcLineBytes, char* srcBits,
	int targetWidth, int targetHeight, int targetX, int targetY, int targetWidth2, int targetHeight2, int targetLineBytes, char* tarGetShiftedBits1_31)
{
	Fixed factorX((double)srcWidth2 / targetWidth2);
	Fixed factorY((double)srcHeight2 / targetHeight2);

	ASSERT(srcBits);
	ASSERT(tarGetShiftedBits1_31);
	ASSERT(0 < srcWidth);
	ASSERT(0 < srcHeight);
	ASSERT(0 < srcWidth2);
	ASSERT(0 < srcHeight2);
	ASSERT(0 < targetWidth);
	ASSERT(0 < targetHeight);
	ASSERT(0 < targetWidth2);
	ASSERT(0 < targetHeight2);
	if (srcX < 0) {
		if (srcX + srcWidth2 < 0)
			return;
		srcWidth2 += srcX;
		srcX = 0;
	}
	if (srcY < 0) {
		if (srcY + srcHeight2 < 0)
			return;
		srcHeight2 += srcY;
		srcY = 0;
	}
	if (targetX < 0) {
		if (targetX + targetWidth2 < 0)
			return;
		targetWidth2 += targetX;
		targetX = 0;
	}
	if (targetY + targetHeight2 < 0)
		return;
	if (targetHeight < targetY)
		return;
	if (targetY < 0) {
		double factor = abs(targetY) / (double)targetHeight2;
		srcY += (srcHeight2 * factor) + 0.5;
		srcHeight2 -= (srcHeight2 * factor);
		targetHeight2 += targetY;
		targetY = 0;
	}
	if (targetHeight < targetY + targetHeight2) {
		double factor = abs(targetHeight - targetY) / (double)targetHeight2;
		srcHeight2 = (srcHeight2 * factor);
		targetHeight2 = targetHeight - targetY;
	}
	ASSERT(srcX >= 0);
	ASSERT(srcY >= 0);
	ASSERT(targetX >= 0);
	ASSERT(targetY >= 0);
	ASSERT(srcWidth >= srcX+srcWidth2);
//	ASSERT(srcHeight >= srcY+srcHeight2);
	ASSERT(targetWidth >= targetX+targetWidth2);
	ASSERT(targetHeight >= targetY+targetHeight2);

/*
	if (srcX + srcWidth2 > srcWidth)
		srcWidth2 = srcWidth - srcX;
	if (srcY + srcHeight2 > srcHeight)
		srcHeight2 = srcHeight - srcY;
	
	if (targetX + targetWidth2 > targetWidth)
		targetWidth2 = targetWidth - targetX;
	if (targetY + targetHeight2 > targetHeight)
		targetHeight2 = targetHeight - targetY;
*/
	
	char* pTargetLine = (char*) tarGetShiftedBits1_31;
	double fFactorY = factorY.GetDouble();
	pTargetLine += (targetY * targetLineBytes) + targetX * 3;
	for (size_t y=0; y<targetHeight2; ++y) {
		char* pTargetPixel = pTargetLine;
		char* pSrcLine = srcBits;
		int dy = srcY + (y * factorY).GetLong();
		ASSERT(dy < (srcY + srcHeight2));
		pSrcLine += (dy * srcLineBytes) + (srcX * factorX).GetLong() * 3;
		for (size_t x=0; x<targetWidth2; ++x) {
			char* pSrcPixel = pSrcLine + (x * factorX).GetLong() * 3;
			pTargetPixel[0] = pSrcPixel[0];
			pTargetPixel[1] = pSrcPixel[1];
			pTargetPixel[2] = pSrcPixel[2];
			pTargetPixel += 3;
		}
		pTargetLine += targetLineBytes;
	}
}
#endif

}	// namespace gl

