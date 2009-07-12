#pragma once

#include <bitset>
#include "arrayutil.h"

namespace gl {

// ��l��
template <typename SrcColorT, typename BinaryColorT>
void Binary(const Buffer2D<SrcColorT>& src, Buffer2D<bool>& target, BinaryColorT binaryColor)
{
	const size_t srcWidth = src.GetWidth();
	const size_t srcHeight = src.GetHeight();
	const int srcLineOffset = src.GetLineOffset();
	const SrcColorT* pSrcBuff = src.GetPixelPtr(0,0);
	const SrcColorT* pSrcBuffWrk = pSrcBuff;
	const size_t VALUE_BITS = sizeof(gl::Buffer2D<bool>::value_type) * 8;
	int bits = 0;
	size_t surplus = srcWidth % VALUE_BITS;
	size_t roundFigure = srcWidth - surplus;

	const size_t targetWidth = target.GetWidth();
	const size_t targetHeight = target.GetHeight();
	assert(srcWidth <= targetWidth);
	assert(srcHeight <= targetHeight);
	const int targetLineOffset = target.GetLineOffset();
	Buffer2D<bool>::value_type* pTargetBuff = target.GetPixelPtr(0, 0);
	Buffer2D<bool>::value_type* pTargetBuffWrk = pTargetBuff;

	for (size_t y=0; y<srcHeight; ++y) {
		for (size_t x=0; x<roundFigure; x+=VALUE_BITS) {
			for (size_t i=0; i<VALUE_BITS; ++i) {
				bits |= binaryColor(*pSrcBuffWrk) << (31-i);
				++pSrcBuffWrk;
			}
			*pTargetBuffWrk = bits;
			++pTargetBuffWrk;
			bits = 0;
		}
		for (size_t i=0; i<surplus; ++i) {
			bits |= binaryColor(*pSrcBuffWrk) << (31 - i);
			++pSrcBuffWrk;
		}
		*pTargetBuffWrk = bits;

		OffsetPtr(pSrcBuff, srcLineOffset);
		pSrcBuffWrk = pSrcBuff;
		OffsetPtr(pTargetBuff, targetLineOffset);
		pTargetBuffWrk = pTargetBuff;
	}
}

// �q�X�e���V�X�ϊ��Ȑ���p������臒l���� ��l��
// VisualC#.NET & Visual Basic.NET�ɂ��f�B�W�^���摜�����̊�b�Ɖ��p -��{�T�O�����摜�F���܂�- ����K�s ��
// VisualBasic & VisualC++�ɂ��f�B�W�^���摜��������- ����K�s ��
template <typename NumericT, typename ValueT, typename ValueT2>
void Binary_DynamicThresholding(
	const Buffer2D< Color1<ValueT> >& src,
	const Buffer2D< Color1<ValueT2> >& blurredSrc,
	Buffer2D<bool>& target,
	NumericT hysteresisOn, NumericT hysteresisOff,
	bool initialState)
{
	const size_t srcWidth = src.GetWidth();
	const size_t srcHeight = src.GetHeight();
	const int srcLineOffset = src.GetLineOffset();
	const Color1<ValueT>* pSrcBuff = src.GetPixelPtr(0,0);
	const Color1<ValueT>* pSrcBuffWrk = pSrcBuff;

	const size_t blurredSrcWidth = blurredSrc.GetWidth();
	const size_t blurredSrcHeight = blurredSrc.GetHeight();
	const int blurredSrcLineOffset = blurredSrc.GetLineOffset();
	const Color1<ValueT2>* pBlurredSrcBuff = blurredSrc.GetPixelPtr(0,0);
	const Color1<ValueT2>* pBlurredSrcBuffWrk = pBlurredSrcBuff;

	const size_t VALUE_BITS = sizeof(gl::Buffer2D<bool>::value_type) * 8;
	size_t surplus = srcWidth % VALUE_BITS;
	size_t roundFigure = srcWidth - surplus;

	const size_t targetWidth = target.GetWidth();
	const size_t targetHeight = target.GetHeight();
	const int targetLineOffset = target.GetLineOffset();
	Buffer2D<bool>::value_type* pTargetBuff = target.GetPixelPtr(0, 0);
	Buffer2D<bool>::value_type* pTargetBuffWrk = pTargetBuff;

	assert(srcWidth <= blurredSrcWidth);
	assert(srcHeight <= blurredSrcHeight);
	assert(srcWidth <= targetWidth);
	assert(srcHeight <= targetHeight);
	
	for (size_t y=0; y<srcHeight; ++y) {
		bool bState = initialState;
		gl::Buffer2D<bool>::value_type bits = 0;
		for (size_t x=0; x<roundFigure; x+=VALUE_BITS) {
			for (size_t i=0; i<VALUE_BITS; ++i) {
				NumericT average = NumericT(pBlurredSrcBuffWrk->a);
				if (bState) {
					average += hysteresisOn;
				}else {
					average += hysteresisOff;
				}
				bState = (average < pSrcBuffWrk->a);
				bits |= (bState << (31-i));
				++pSrcBuffWrk;
				++pBlurredSrcBuffWrk;
			}
			*pTargetBuffWrk = bits;
			++pTargetBuffWrk;
			bits = 0;
		}
		for (size_t i=0; i<surplus; ++i) {
			NumericT average = NumericT(pBlurredSrcBuffWrk->a);
			if (bState) {
				average += hysteresisOn;
			}else {
				average += hysteresisOff;
			}
			bState = (average < pSrcBuffWrk->a);
			bits |= bState << (31-i);
			++pSrcBuffWrk;
			++pBlurredSrcBuffWrk;
		}
		*pTargetBuffWrk = bits;
		
		OffsetPtr(pSrcBuff, srcLineOffset);
		pSrcBuffWrk = pSrcBuff;
		OffsetPtr(pBlurredSrcBuff, blurredSrcLineOffset);
		pBlurredSrcBuffWrk = pBlurredSrcBuff;
		OffsetPtr(pTargetBuff, targetLineOffset);
		pTargetBuffWrk = pTargetBuff;
	}
}

}	// namespace gl














