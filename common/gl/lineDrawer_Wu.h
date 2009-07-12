#pragma once

/*

Wu, Xiaolin

http://freespace.virgin.net/hugo.elias/graphics/x_wuline.htm
http://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm

http://www.cs.nps.navy.mil/people/faculty/capps/iap/class1/lines/lines.html
を参考にC++で実装。

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
class LineDrawer_Wu : public IBufferLineDrawer<NumericT, ColorT>
{
protected:
	Buffer2D<ColorT>*	pBuff_;		//!< 描画先
	static const NumericT zero;
	typedef typename ColorT::value_type ComponentT;
	
	//! Correction table depent on the slope
	static const size_t SLOPECORR_TABLE_SIZE = 512;
	NumericT slopeCorrTable_[SLOPECORR_TABLE_SIZE];	
	
	void initTable()
	{
		initSlopeCorrTable();
	}

	void initSlopeCorrTable()
	{
		for (size_t i=0; i<SLOPECORR_TABLE_SIZE; ++i) {
			double m = (i + 0.5) / SLOPECORR_TABLE_SIZE;
			slopeCorrTable_[i] = sqrt(m*m + 1) * 0.707106781; /* (m+1)^2 / sqrt(2) */
		}
	}
	
public:
	
	// ピクセルの中央位置を仮に整数座標と認識している。
	inline void drawLine(ColorT color, NumericT width, NumericT height, NumericT x1, NumericT y1, NumericT x2, NumericT y2)
	{
		NumericT palpha = NumericT(color.a);	// 描画色の透明度
		const int lineOffset = pBuff_->GetLineOffset();
		if (height < width) {
			if (x2 < x1) {
				std::swap(x1, x2);
				std::swap(y1, y2);
			}
			NumericT gradient = (y2 - y1) / width;
			NumericT slope = slopeCorrTable_[ ToInt( abs(gradient) * NumericT(SLOPECORR_TABLE_SIZE-1) ) ];
			palpha *= slope;

			unsigned char* ptr;
			NumericT yf;
			size_t prev_iyf;

			// Start Point
			{
				NumericT xgap = NumericT(1) - frac(x1 + NumericT(0.5));		// ピクセルの横占有率
				NumericT xend = halfAdjust(x1);								// 最近傍の実X座標を求める
				NumericT yend = y1 + gradient * (xend - x1);				// ピクセルの中央位置（仮想開始位置）までXを移動した場合のY座標を求める
				ptr = (unsigned char*) pBuff_->GetPixelPtr(xend, ToInt(yend));

				NumericT dist = frac(yend);									// 仮想ピクセル中央Y座標からの距離
				ColorT* cptr = (ColorT*)ptr;
				*cptr = (*pBlender_)(color, palpha * xgap * (NumericT(1) - dist), *cptr);
				cptr = (ColorT*)(ptr + lineOffset);
				*cptr = (*pBlender_)(color, (palpha * xgap * dist), *cptr);
				
				yf = yend;
				prev_iyf = ToInt(yf);
				yf += gradient;
				size_t cur_iyf = ToInt(yf);
				ptr += lineOffset * (cur_iyf - prev_iyf) + sizeof(ColorT);
				prev_iyf = cur_iyf;
			}

			// Mid Points
			size_t loopCnt = halfAdjust(x2) - halfAdjust(x1);
			if (loopCnt == 0) {
				return;
			}
			while (--loopCnt) {
				NumericT dist = frac(yf);
				ColorT* cptr = (ColorT*) ptr;
				*cptr = (*pBlender_)(color, palpha * (NumericT(1)-dist), *cptr);
				cptr = (ColorT*)(ptr + lineOffset);
				*cptr = (*pBlender_)(color, (palpha * dist), *cptr);

				yf += gradient;
				size_t cur_iyf = ToInt(yf);
				ptr += lineOffset * (cur_iyf - prev_iyf) + sizeof(ColorT);
				prev_iyf = cur_iyf;
			}

			// End Point
			{
				NumericT xgap = frac(x2 + NumericT(0.5));			// ピクセルの横占有率
				NumericT xend = halfAdjust(x2);						// 最近傍の実X座標を求める
				NumericT yend = y2 + gradient * (xend - x2);		// ピクセルの中央位置（仮想開始位置）までXを移動した場合のY座標を求める

				NumericT dist = frac(yf);							// 仮想ピクセル中央Y座標からの距離
				ColorT* cptr = (ColorT*)ptr;
				*cptr = (*pBlender_)(color, palpha * xgap * (NumericT(1) - dist), *cptr);
				cptr = (ColorT*)(ptr + lineOffset);
				*cptr = (*pBlender_)(color, (palpha * xgap * dist), *cptr);
			}

		}
		else {
			if (y2 < y1) {
				std::swap(x1, x2);
				std::swap(y1, y2);
			}
			NumericT gradient = (x2 - x1) / height;
			NumericT slope = slopeCorrTable_[ ToInt( abs(gradient) * NumericT(SLOPECORR_TABLE_SIZE-1) ) ];
			palpha *= slope;

			unsigned char* ptr;
			NumericT xf;
			size_t prev_ixf;

			// Start Point
			{
				NumericT ygap = NumericT(1) - frac(y1 + NumericT(0.5));		// ピクセルの縦占有率
				NumericT yend = halfAdjust(y1);								// 最近傍の実Y座標を求める
				NumericT xend = x1 + gradient * (yend - y1);				// ピクセルの中央位置（仮想開始位置）までYを移動した場合のX座標を求める
				ptr = (unsigned char*) pBuff_->GetPixelPtr(ToInt(xend), yend);

				NumericT dist = frac(xend);									// 仮想ピクセル中央X座標からの距離
				ColorT* cptr = (ColorT*)ptr;
				*cptr = (*pBlender_)(color, palpha * ygap * (NumericT(1) - dist), *cptr);
				++cptr;
				*cptr = (*pBlender_)(color, (palpha * ygap * dist), *cptr);
				
				xf = xend;
				prev_ixf = ToInt(xf);
				xf += gradient;
				size_t cur_ixf = ToInt(xf);
				ptr += lineOffset + (cur_ixf - prev_ixf) * sizeof(ColorT);
				prev_ixf = cur_ixf;
			}

			// Mid Points
			size_t loopCnt = halfAdjust(y2) - halfAdjust(y1);
			if (loopCnt == 0) {
				return;
			}
			while (--loopCnt) {
				NumericT dist = frac(xf);
				ColorT* cptr = (ColorT*) ptr;
				*cptr = (*pBlender_)(color, palpha * (NumericT(1)-dist), *cptr);
				++cptr;
				*cptr = (*pBlender_)(color, (palpha * dist), *cptr);

				xf += gradient;
				size_t cur_ixf = ToInt(xf);
				ptr += lineOffset + (cur_ixf - prev_ixf) * sizeof(ColorT);
				prev_ixf = cur_ixf;
			}

			// End Point
			{
				NumericT ygap = frac(y2 + NumericT(0.5));			// ピクセルの縦占有率
				NumericT yend = halfAdjust(y2);						// 最近傍の実Y座標を求める
				NumericT xend = x2 + gradient * (yend - y2);		// ピクセルの中央位置（仮想開始位置）までYを移動した場合のX座標を求める

				NumericT dist = frac(xf);							// 仮想ピクセル中央X座標からの距離
				ColorT* cptr = (ColorT*)ptr;
				*cptr = (*pBlender_)(color, palpha * ygap * (NumericT(1) - dist), *cptr);
				++cptr;
				*cptr = (*pBlender_)(color, (palpha * ygap * dist), *cptr);
			}
		}

	}
	
public:
	LineDrawer_Wu(Buffer2D<ColorT>* pBuff = NULL)
		:
		pBuff_(pBuff)
	{
		initTable();
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

