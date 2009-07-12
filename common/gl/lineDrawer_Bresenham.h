#pragma once


/*
	ƒvƒŒƒ[ƒ“ƒnƒ€‚ÌƒAƒ‹ƒSƒŠƒYƒ€‚É‚æ‚éü•ª•`‰æ‚ÌÀ‘•
	
	http://www2.starcat.ne.jp/~fussy/algo/algo1-1.htm
	http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

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
class LineDrawer_Bresenham : public IBufferLineDrawer<NumericT, ColorT>
{
private:
	Buffer2D<ColorT>*	pBuff_;		//!< •`‰ææ
	
	inline void drawLine(ColorT color, NumericT width, NumericT height, NumericT x1, NumericT y1, NumericT x2, NumericT y2)
	{
		int dx = width;
		int dy = height;
		int sx = (x1 < x2) ? 1 : -1;
		int sy = (y1 < y2) ? 1 : -1;
		ColorT* ptr = pBuff_->GetPixelPtr(halfAdjust(x1), halfAdjust(y1));
		sy *= pBuff_->GetLineOffset();
		int dx2 = dx * 2;
		int dy2 = dy * 2;
		int E;

		// ŒX‚«‚ª1ˆÈ‰º‚Ìê‡
		if (height <= width) {
			E = -dx;
			for (size_t i=0; i<=dx; ++i) {
				*ptr = (*pBlender_)(color, *ptr);
				ptr += sx;
				E += dy2;
				if (0 <= E) {
					OffsetPtr(ptr, sy);
					E -= dx2;
				}
			}
		// ŒX‚«‚ª1‚æ‚è‘å‚«‚¢ê‡
		}else {
			E = -dy;
			for (size_t i=0; i<=dy; ++i) {
				*ptr = (*pBlender_)(color, *ptr);
				OffsetPtr(ptr, sy);
				E += dx2;
				if (0 <= E) {
					ptr += sx;
					E -= dy2;
				}
			}
		}

	}

public:
	LineDrawer_Bresenham(Buffer2D<ColorT>* pBuff = NULL)
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

} // namespace gl

