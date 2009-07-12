#pragma once

/*
	GDI”C‚¹‚Ìü•ª•`‰æˆ—
*/

#include <math.h>
#include <algorithm>
#include "glcolor.h"
#include "glbuffer2d.h"
#include "arrayutil.h"

#include "glILineDrawer.h"
#include "Windows.h"

namespace gl
{

template <typename NumericT, typename ColorT>
class LineDrawer_GDI : public ILineDrawer<NumericT, ColorT>
{
protected:
	Buffer2D<ColorT>*	pBuff_;		//!< •`‰ææ
	
	ColorT color_;
	NumericT lineWidth_;

	HDC hDC_;
public:
	void SetDC(HDC hDC)
	{
		hDC_ = hDC;
	}

	void DrawLine(NumericT x1, NumericT y1, NumericT x2, NumericT y2)
	{
		::MoveToEx(hDC_, x1, y1, NULL);
		::LineTo(hDC_, x2, y2);
	}

	/*virtual*/
	void SetLineWidth(const NumericT width)
	{
	}

	/*virtual*/
	void SetColor(const ColorT col)
	{
		HPEN hPen = ::CreatePen(PS_SOLID, 1, RGB(255,255,255));
		::SelectObject(hDC_, hPen);
		color_ = col;
	}

};

}	// namespace gl

