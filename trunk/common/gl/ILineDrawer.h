#pragma once

namespace gl
{

template <typename NumericT, typename ColorT>
class ILineDrawer
{
public:
	virtual void DrawLine(ColorT col, NumericT x1, NumericT y1, NumericT x2, NumericT y2) = 0;
};

template <typename NumericT, typename ColorT>
class IBufferLineDrawer : public ILineDrawer<NumericT, ColorT>
{
public:
	virtual void SetBuffer(Buffer2D<ColorT>* pBuff) = 0;
};


}	// namespace gl

