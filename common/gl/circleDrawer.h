#pragma once

/*
	円描画処理
*/

#include "buffer2d.h"

double elapsed;

namespace gl {

template <typename NumericT, typename ColorT, typename ColorBlenderT>
class CircleDrawer
{
public:

	void DrawCircle(ColorT color, NumericT x, NumericT y, NumericT radius)
	{
		NumericT sqrRad = radius * radius;
		NumericT halfWay = sqrt(sqrRad / 2);
		int iHalfWay = int(halfWay+0.5);

		ColorT* pCenter = pBuff_->GetPixelPtr(x, y);
		int lineOffset = pBuff_->GetLineOffset();
		ColorT* curPixel = 0;

		const NumericT one = OneMinusEpsilon(NumericT(1.0));
		{
			NumericT distance = radius;
			int iDistance = int(distance);
			NumericT fraction = frac(distance);
			NumericT fracRemain = one - fraction;

			SetPixel(x - iDistance, y, color);
			SetPixel(x + iDistance, y, color);

			SetPixel(x, y - iDistance, color);
			SetPixel(x, y + iDistance, color);
		}

		ColorT* curLine = 0;
		for (size_t i=1; i<=iHalfWay; ++i) {
			NumericT sqrHeight = i * i;
			NumericT sqrWidth = sqrRad - sqrHeight;
			NumericT distance = sqrt(sqrWidth);
			int iDistance = int(distance);
			NumericT fraction = frac(distance);
			NumericT fracRemain = one - fraction;

			SetPixel(x - iDistance, y - i, color);
			SetPixel(x + iDistance, y - i, color);

			SetPixel(x - iDistance, y + i, color);
			SetPixel(x + iDistance, y + i, color);

			SetPixel(x - i, y - iDistance, color);
			SetPixel(x + i, y - iDistance, color);

			SetPixel(x - i, y + iDistance, color);
			SetPixel(x + i, y + iDistance, color);
		}
	}


	void DrawCircle_AA(ColorT color, NumericT x, NumericT y, NumericT radius)
	{
		NumericT sqrRad = radius * radius;
		NumericT halfWay = sqrt(sqrRad / 2);
		int iHalfWay = int(halfWay+0.5);

		ColorT* pCenter = pBuff_->GetPixelPtr(x, y);
		int lineOffset = pBuff_->GetLineOffset();
		ColorT* curPixel = 0;

		const NumericT one = OneMinusEpsilon(NumericT(1.0));
		{
			NumericT distance = radius;
			int iDistance = int(distance);
			NumericT fraction = frac(distance);
			NumericT fracRemain = one - fraction;

			SetPixel(x - iDistance + 0, y, color, fraction);
			SetPixel(x - iDistance + 1, y, color, fracRemain);
			SetPixel(x + iDistance - 1, y, color, fracRemain);
			SetPixel(x + iDistance + 0, y, color, 0.0 + fraction);

			SetPixel(x, y - iDistance + 0, color, fraction);
			SetPixel(x, y - iDistance + 1, color, fracRemain);
			SetPixel(x, y + iDistance - 1, color, fracRemain);
			SetPixel(x, y + iDistance + 0, color, fraction);
		}

		ColorT* curLine = 0;
		for (size_t i=1; i<=iHalfWay; ++i) {
			NumericT sqrHeight = i * i;
			NumericT sqrWidth = sqrRad - sqrHeight;
			NumericT distance = sqrt(sqrWidth);
			int iDistance = int(distance);
			NumericT fraction = frac(distance);
			NumericT fracRemain = one - fraction;

			SetPixel(x - iDistance + 0, y - i, color, fraction);
			SetPixel(x - iDistance + 1, y - i, color, fracRemain);
			SetPixel(x + iDistance - 1, y - i, color, fracRemain);
			SetPixel(x + iDistance + 0, y - i, color, 0.0 + fraction);

			SetPixel(x - iDistance + 0, y + i, color, fraction);
			SetPixel(x - iDistance + 1, y + i, color, fracRemain);
			SetPixel(x + iDistance - 1, y + i, color, fracRemain);
			SetPixel(x + iDistance + 0, y + i, color, fraction);

			SetPixel(x - i, y - iDistance + 0, color, fraction);
			SetPixel(x + i, y - iDistance + 0, color, fraction);
			SetPixel(x - i, y - iDistance + 1, color, fracRemain);
			SetPixel(x + i, y - iDistance + 1, color, fracRemain);

			SetPixel(x - i, y + iDistance - 1, color, fracRemain);
			SetPixel(x + i, y + iDistance - 1, color, fracRemain);
			SetPixel(x - i, y + iDistance + 0, color, fraction);
			SetPixel(x + i, y + iDistance + 0, color, fraction);
		}
	}

	void SetPixel(size_t x, size_t y, ColorT c)
	{
		if (x < pBuff_->GetWidth() && y < pBuff_->GetHeight()) {
			pBuff_->SetPixel(
				x, y,
				(*pBlender_)(c, pBuff_->GetPixel(x, y))
			);
		}
	}
	
	void SetPixel(size_t x, size_t y, ColorT c, NumericT a)
	{
		if (x < pBuff_->GetWidth() && y < pBuff_->GetHeight()) {
			pBuff_->SetPixel(
				x, y,
				(*pBlender_)(c, a, pBuff_->GetPixel(x, y))
			);
		}
	}
	
	Buffer2D<ColorT>* pBuff_;
	ColorBlenderT* pBlender_;

};

} // namespace gl {

