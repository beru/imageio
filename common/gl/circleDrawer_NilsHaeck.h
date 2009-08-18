#pragma once

/*
	円描画処理

	http://www.simdesign.nl/tips/tip002.html
	
	に書かれている処理をC++で実装。

*/


#include "arrayutil.h"

namespace gl
{


__forceinline int Round( double v )
{
    assert( v >= INT_MIN && v <= INT_MAX );
    int result;
    __asm
    {
        fld    v      ; Push 'v' into st(0) of FPU stack
        fistp  result ; Convert and store st(0) to integer and pop
    }

    return result;
}

template <typename NumericT, typename NumericT2, typename ColorT, typename ColorBlenderT>
class CircleDrawer_NilsHaeck
{
private:
	Buffer2D<ColorT>* pBuff_;
	ColorBlenderT blender_;

	ColorT color_;
	NumericT lineWidth_;
	NumericT feather_;
	NumericT2* table_;

public:
	static const size_t TABLESIZE = 1024;
	CircleDrawer_NilsHaeck(Buffer2D<ColorT>* pBuff = NULL)
	{
		SetBuffer(pBuff);
		SetLineWidth(NumericT(1));
		SetFeather(NumericT(1));
		table_ = initTable<NumericT2, TABLESIZE>();
	}

	void SetBuffer(Buffer2D<ColorT>* pBuff)
	{
		if (pBuff == NULL)
			return;
		pBuff_ = pBuff;
	}
	
	// valid only for circle
	void SetLineWidth(NumericT width)
	{
		assert(NumericT(0) <= width);
		lineWidth_ = width;
	}
	
	void SetColor(ColorT col)
	{
		color_ = col;
	}
	
	void SetFeather(NumericT feather)
	{
		feather_ = feather;
	}

	void DrawDisc(NumericT cx, NumericT cy, NumericT radius)
	{
		NumericT halfFeather = div2<1>(feather_);
		NumericT rpf = radius + halfFeather;	// radius plus half feather
		NumericT rmf = radius - halfFeather;	// radius minus half feather

		// bounds... clipping
		NumericT lx = max(floor(cx - rpf), 0);
		NumericT rx = min(ceil(cx + rpf), pBuff_->GetWidth()-1);
		NumericT ly = max(floor(cy - rpf), 0);
		NumericT ry = min(ceil(cy + rpf), pBuff_->GetHeight()-1);

		NumericT rpf2 = sqr(rpf);				// square of radius plus half feather
		NumericT rmf2 = sqr(rmf);				// square of radius minus half feather
		NumericT feather = max(feather_, Epsilon(NumericT(1)));
		NumericT invFeather = NumericT(1) / feather;
		
		ColorT* lptr = pBuff_->GetPixelPtr(0, ly);
		const int lineOffset = pBuff_->GetLineOffset();
		
		NumericT xDiff;
		NumericT sqrtTarget;
		NumericT xDiffDiff;
		for (NumericT y=ly; y<ry; ++y) {
			NumericT yLen = (y - cy);
			NumericT sqrYLen = sqr(yLen);
			NumericT sqrXLen = max(0, rpf2 - sqrYLen);
			NumericT xLen = sqrt(sqrXLen);
			// 外側の空白をskip
			NumericT lx_OuterCircle = max(lx, cx - xLen);
			NumericT rx_OuterCircle = min(rx, cx + xLen);
			NumericT x = lx + ceil(lx_OuterCircle - lx);
			ColorT* pptr = lptr + ToInt(x);
			// featherの影響が及ばない内側の円の距離を求める
			NumericT xLen_InnerCircle = sqrt(rmf2 - min(rmf2, sqrYLen));
			NumericT lx_InnerCircle = max(lx, cx - xLen_InnerCircle);
			NumericT rx_InnerCircle = min(rx, cx + xLen_InnerCircle);
			assert(lx_OuterCircle <= lx_InnerCircle);
			
			// left feather

			// ((x+1)^2 - x^2) = x^2 + 2x + 1 - x^2 = 2x + 1
			// (x^2 - (x-1)^2) = x^2 - x^2 + 2x - 1 = 2x - 1
			// ((x+1)^2 - x^2) - (x^2 - (x-1)^2) = 2
			xDiff = (x - cx);
			sqrtTarget = sqrYLen + sqr(xDiff);			
			xDiffDiff = mul2<1>(xDiff) + NumericT(1);	// (xDiff + 1) - xDiff^2  = xDiff^2 + xDiff*2 + 1 -xDiff^2 = xDiff*2 + 1
			NumericT calculatedRadiusFactor = radius * invFeather + NumericT(0.5);
			for (x; x<lx_InnerCircle; ++x) {
				NumericT sqrtResult = sqrt(sqrtTarget);
				NumericT fact = calculatedRadiusFactor - sqrtResult * invFeather;
				sqrtTarget += xDiffDiff;
				xDiffDiff += NumericT(2);
				*pptr = blender_(color_, fact, *pptr);
				++pptr;
			}
			// center fill
			for (; x<rx_InnerCircle; ++x) {
				*pptr = blender_(color_, OneMinusEpsilon(NumericT(0.0)), *pptr);
				++pptr;
			}
			// right feather
			assert(rx_OuterCircle >= rx_InnerCircle);
			xDiff = (x - cx);
			sqrtTarget = sqrYLen + sqr(xDiff);
			xDiffDiff = mul2<1>(xDiff) + NumericT(1);
			for (; x<rx_OuterCircle; ++x) {
				NumericT sqrtResult = sqrt(sqrtTarget);
				NumericT fact = calculatedRadiusFactor - sqrtResult * invFeather;
				sqrtTarget += xDiffDiff;
				xDiffDiff += NumericT(2);
				*pptr = blender_(color_, fact, *pptr);
				++pptr;
			}
			OffsetPtr(lptr, lineOffset);
		}
	}
	
	/*
		sqrt は処理に時間が掛かりすぎる。そこで、
		中心からのX距離とY距離の二乗の和を、0.0から1.0の中心からの距離の値に変換し利用する。

	*/
	template <typename NumericT, size_t SIZE>
	static NumericT* initTable()
	{
		static NumericT table[SIZE+1];
		for (size_t i=0; i<=SIZE+1; ++i) {
			table[i] = 0.99999 - sqrt(i/(double)SIZE);
		}
		return table;
	}
	
	void DrawSphere(NumericT cx, NumericT cy, NumericT radius)
	{
		// bounds... clipping
		NumericT lx = max(floor(cx - radius), 0);
		NumericT rx = min(ceil(cx + radius), pBuff_->GetWidth()-1);
		NumericT ly = max(floor(cy - radius), 0);
		NumericT ry = min(ceil(cy + radius), pBuff_->GetHeight()-1);
		
		NumericT sqrRadius = sqr(radius);
		ColorT* lptr = pBuff_->GetPixelPtr(0, ly);
		const int lineOffset = pBuff_->GetLineOffset();
		const NumericT2 invRadius = NumericT2(1.0) / radius;
		const NumericT2 tableIndexFactor = TABLESIZE / double(sqrRadius);
		for (NumericT y=ly; y<ry; ++y) {
			NumericT yLen = (y - cy);
			NumericT sqrYLen = sqr(yLen);
			NumericT sqrXLen = max(0, sqrRadius - sqrYLen);
			NumericT maxXLen = sqrt(sqrXLen);
			// 外側の空白をskip
			NumericT lx_OuterCircle = max(lx, cx - maxXLen);
			NumericT rx_OuterCircle = min(rx, cx + maxXLen);
			NumericT x = lx + ceil(lx_OuterCircle - lx);
			ColorT* pptr = lptr + ToInt(x);
			
#if 1
			NumericT xDiff = x - cx;
			NumericT sqrLenSum = sqrYLen + sqr(xDiff);
			NumericT xDiffDiff = mul2<1>(x - cx) + NumericT(1);	// (xDiff + 1) - xDiff^2  = xDiff^2 + xDiff*2 + 1 -xDiff^2 = xDiff*2 + 1
			for (x; x<rx_OuterCircle; ++x) {
#if 0
				NumericT2 tablePos = tableIndexFactor; 	tablePos *= sqrLenSum;
				size_t index = size_t(tablePos);
				NumericT2 fraction = frac(tablePos);
				NumericT2 remain = NumericT2(1.0) - fraction;
//				NumericT2 fact = (table_[index] * (NumericT2(1.0)-fraction) + table_[index+1] * fraction);
				NumericT2 fact = table_[index]; 
				fact *= remain;
				fraction *= table_[index+1];
				fact += fraction;
				
				*pptr = blender_(color_, fact, *pptr);
				++pptr;
				
				sqrLenSum += xDiffDiff;
				xDiffDiff += NumericT(2);
#else
#if 1
				NumericT2 tablePos = tableIndexFactor;
				tablePos *= sqrLenSum;
				tablePos += NumericT2(0.5);
				size_t index = size_t(tablePos);
				NumericT2 fact = table_[index]; 
				*pptr = blender_(color_, fact, *pptr);
				++pptr;
				
				sqrLenSum += xDiffDiff;
				xDiffDiff += NumericT(2);

#else
				*pptr = blender_(color_, NumericT(0.5), *pptr);
				++pptr;
#endif
#endif
			}
#else
			for (x; x<rx_OuterCircle; ++x) {
				NumericT2 fact = sqrt(sqrYLen + sqr(x-cx)) * invRadius;
				*pptr = blender_(color_, fact, *pptr);
				++pptr;
			}
#endif
			lptr = (ColorT*)( (char*)(lptr) + lineOffset);
		}
	}
	
	void DrawCircle(NumericT cx, NumericT cy, NumericT radius)
	{
		NumericT halfLineWidth = lineWidth_ / NumericT(2);
		NumericT outerRadius = radius + halfLineWidth;
		NumericT innerRadius = radius - halfLineWidth;
		NumericT halfFeather = feather_ / NumericT(2);
		NumericT ropf = outerRadius + halfFeather;
		NumericT ropf2 = sqr(ropf);
		NumericT romf = outerRadius - halfFeather;
		NumericT romf2 = sqr(romf);
		NumericT ripf = innerRadius + halfFeather;
		NumericT ripf2 = sqr(ripf);
		NumericT rimf = innerRadius - halfFeather;
		NumericT rimf2 = sqr(rimf);

		// bounds... clipping
		NumericT lx = max(floor(cx - ropf), 0);
		NumericT rx = min(ceil(cx + ropf), pBuff_->GetWidth()-1);
		NumericT ly = max(floor(cy - ropf), 0);
		NumericT ry = min(ceil(cy + ropf), pBuff_->GetHeight()-1);

		NumericT feather = min(feather_, lineWidth_);

		NumericT invFeather = (feather_ == NumericT(0)) ? 0 : (NumericT(1) / feather_);
		ColorT* lptr = pBuff_->GetPixelPtr(lx, ly);
		const int lineOffset = pBuff_->GetLineOffset();
		for (NumericT y=ly; y<ry; ++y) {
			NumericT ylen = (y - cy);
			NumericT sqrYLen = sqr(ylen);
			ColorT* pptr = lptr;
			size_t cnt = 0;
			for (NumericT x=lx; x<rx; ++x) {
				NumericT sqrDistance = sqrYLen + sqr(x-cx);
				if (sqrDistance >= rimf2) {
					if (sqrDistance < ropf2) {
						if (sqrDistance < romf2) {
							if (sqrDistance < ripf2) {
								NumericT fact = (sqrt(sqrDistance) - innerRadius) * invFeather + NumericT(0.5);
								*pptr = blender_(color_, fact, *pptr);
							}else {
								*pptr = blender_(color_, NumericT(1), *pptr);
							}
						}else {
							NumericT fact = (outerRadius - sqrt(sqrDistance)) * invFeather + NumericT(0.5);
//							NumericT fact = (outerRadius*outerRadius - sqrDistance) * invFeather*invFeather + NumericT(0.5);
							*pptr = blender_(color_, fact, *pptr);
						}
					}
				}
				++pptr;
				++cnt;
			}
			OffsetPtr(lptr, lineOffset);
		}
		


	}
	
};

};

