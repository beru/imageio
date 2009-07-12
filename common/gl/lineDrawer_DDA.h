#pragma once

/*
	Digital Differential Analyzerによる線分描画処理
*/

#include <math.h>
#include <algorithm>
#include "glcolor.h"
#include "glbuffer2d.h"
#include "arrayutil.h"

#include "glILineDrawer.h"

namespace gl
{

template <typename NumericT, typename ColorT, typename ColorBlenderT>
class LineDrawer_DDA : public IBufferLineDrawer<NumericT, ColorT>
{
protected:
	Buffer2D<ColorT>*	pBuff_;		//!< 描画先
	
	__forceinline
	void drawLine(ColorT color, NumericT width, NumericT height, NumericT x1, NumericT y1, NumericT x2, NumericT y2)
	{
		const int lineOffset = pBuff_->GetLineOffset();
		if (height <= width) {
			if (x2 < x1) {
				std::swap(x1, x2);
				std::swap(y1, y2);
			}
			NumericT a = (y2 - y1) / width;
			NumericT x = x1;
			NumericT y = y1;
			ColorT* ptr = pBuff_->GetPixelPtr(x, y);
			size_t previy = ToInt(y);
			size_t loopCnt = ToInt(width) + 2;
			while (--loopCnt) {
				y += a;
				*ptr = (*pBlender_)(color, *ptr);
				OffsetPtr(ptr, sizeof(ColorT) + (ToInt(y) - previy) * lineOffset);
				previy = ToInt(y);
			}
		}else {
			if (y2 < y1) {
				std::swap(x1, x2);
				std::swap(y1, y2);
			}
			NumericT a = (x2 - x1) / height;
			NumericT y = y1;
			NumericT x = x1;
			ColorT* ptr = pBuff_->GetPixelPtr(x, y);
			size_t previx = ToInt(x);
			size_t loopCnt = ToInt(height) + 2;
			while (--loopCnt) {
				x += a;
				*ptr = (*pBlender_)(color, *ptr);
				OffsetPtr(ptr, lineOffset + (ToInt(x) - previx));
				previx = ToInt(x);
			}
		}
	}

public:
	LineDrawer_DDA(Buffer2D<ColorT>* pBuff = NULL)
	{
		SetBuffer(pBuff);
	}

	void SetBuffer(Buffer2D<ColorT>* pBuff)
	{
		if (pBuff != NULL) {
			pBuff_ = pBuff;
		}
	}
	
	ColorBlenderT* pBlender_;
	
	void DrawLine(ColorT color, NumericT x1, NumericT y1, NumericT x2, NumericT y2)
	{
		NumericT width = abs(x2 - x1);
		NumericT height = abs(y2 - y1);
		if (width + height == NumericT(0))
			return;
		drawLine(color, width, height, x1, y1, x2, y2);
	}
	
};

}	// namespace gl

