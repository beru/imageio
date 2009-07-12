#pragma once

/*

Scott R. Nelson が発表した Unweighted Area Sampling Weighted Area Sampling Gupta-Sproull Antialiased Lines

http://www.acm.org/jgt/papers/Nelson96/
http://www.acm.org/jgt/papers/Nelson97/
http://www.acm.org/jgt/papers/Nelson96/linequality/lqtest.html

公開されていたコードを参考にC++で実装。
*/

#include <math.h>
#include <algorithm>
#include "color.h"
#include "buffer2d.h"
#include "arrayutil.h"

#include "ILineDrawer.h"

namespace gl
{

template <typename NumericT, typename ColorT, typename ColorBlenderT, size_t slopeCorrectionTableSize = 256, size_t filterTableSize = 256>
class LineDrawer_Nelson : public IBufferLineDrawer<NumericT, ColorT>
{
private:
	Buffer2D<ColorT>*	pBuff_;
	
	const NumericT* slopeCorrectionTable_;
	const NumericT* filterTable_;	//!< Gaussian for antialiasing filter
	
	__forceinline void calcEndPointTable(NumericT ep_table[9], NumericT sf, NumericT ef)
	{
		/*-
		 * Compute end-point code (defined as follows):
		 *  0 =  0, 0: short, no boundary crossing
		 *  1 =  0, 1: short line overlap (< 1.0)
		 *  2 =  0, 2: 1st pixel of 1st endpoint
		 *  3 =  1, 0: short line overlap (< 1.0)
		 *  4 =  1, 1: short line overlap (> 1.0)
		 *  5 =  1, 2: 2nd pixel of 1st endpoint
		 *  6 =  2, 0: last of 2nd endpoint
		 *  7 =  2, 1: first of 2nd endpoint
		 *  8 =  2, 2: regular part of line
		 */
		sf *= NumericT(0.5);
		ef *= NumericT(0.5);
		ep_table[0] = NumericT(0);
		ep_table[1] = (ef - sf);
		ep_table[2] = (NumericT(0.5) - sf);
		ep_table[3] = (ef - sf);
		ep_table[4] = (ef - sf) + NumericT(0.5);
		ep_table[5] = (NumericT(1) - sf);
		ep_table[6] = ef;
		ep_table[7] = ef + NumericT(0.5);
		ep_table[8] = NumericT(1);
	}
	
	NumericT getEndPointCorrectValue(const NumericT ep_table[9], const int scount, const int ecount)
	{
		// OpenCVのやり方のが高速
		const size_t idx = (((scount >= 2) + 1) & (scount | 2)) * 3 +
					(((ecount >= 2) + 1) & (ecount | 2));
		return ep_table[idx];
	}
	
	void drawLine(ColorT color, NumericT width, NumericT height, NumericT x1, NumericT y1, NumericT x2, NumericT y2)
	{
		assert(2 <= slopeCorrectionTableSize);
		assert(8 <= filterTableSize);
		static const size_t filterTableOffset = (filterTableSize / 8) + (filterTableSize / 2);
		const int lineOffset = pBuff_->GetLineOffset();
		
		NumericT ep_table[9];
		size_t scount = 0;
		NumericT alpha = NumericT(color.a);
		if (height < width) {
			if (x2 < x1) { // left;
				std::swap(x2, x1);
				std::swap(y2, y1);
			}
			const NumericT dy = y2 - y1;
			const NumericT gradient = dy / width;

			alpha *= slopeCorrectionTable_[ ToInt(mul<slopeCorrectionTableSize - 1>(abs(gradient))) ];

			NumericT y = y1 + (NumericT(0) - frac(x1)) * gradient + NumericT(0.5);
			++x2;

			calcEndPointTable(ep_table, frac(x1), frac(x2));

			int prev_iyf = ToInt(y);
			int loopCnt = floor(x2) - floor(x1);
			unsigned char* ptr = (unsigned char*)pBuff_->GetPixelPtr(ToInt(x1), prev_iyf);
			while (loopCnt >= 0) {
				NumericT epCorrectedAlpha = alpha * getEndPointCorrectValue(ep_table, scount, loopCnt);

				unsigned char* tptr = ptr;
				tptr -= lineOffset;
				ColorT* cptr = (ColorT*)tptr;
				size_t filterIndex = filterTableOffset + ToInt(mul<filterTableSize / 4>(frac(y)));
				*cptr = (*pBlender_)(color, (epCorrectedAlpha * filterIndex[filterTable_]), *cptr);
				tptr += lineOffset; cptr = (ColorT*)tptr;
				*cptr = (*pBlender_)(color, (epCorrectedAlpha * filterIndex[filterTable_ - (filterTableSize / 4)]), *cptr);
				tptr += lineOffset; cptr = (ColorT*)tptr;
				*cptr = (*pBlender_)(color, (epCorrectedAlpha * filterIndex[filterTable_ - (filterTableSize / 2)]), *cptr);

				y += gradient;
				ptr += (ToInt(y) - prev_iyf) * lineOffset + sizeof(ColorT);
				prev_iyf = ToInt(y);
				++scount;
				--loopCnt;
			}
		}else {
			if (y2 < y1) { // up
				std::swap(x2, x1);
				std::swap(y2, y1);
			}
			NumericT dx = x2 - x1;
			const NumericT gradient = dx / height;

			alpha *= slopeCorrectionTable_[ ToInt(mul<slopeCorrectionTableSize - 1>(abs(gradient))) ];

			NumericT x = x1 + (NumericT(0) - frac(y1)) * gradient + NumericT(0.5);
			++y2;

			calcEndPointTable(ep_table, frac(y1), frac(y2));

			int prev_ixf = ToInt(x);
			unsigned char* ptr = (unsigned char*)pBuff_->GetPixelPtr(prev_ixf, ToInt(y1));
			int loopCnt = ToInt(y2) - ToInt(y1);
			while (loopCnt >= 0) {
				NumericT epCorrectedAlpha = alpha * getEndPointCorrectValue(ep_table, scount, loopCnt);

				ColorT* cptr = (ColorT*)ptr - 1;
				size_t filterIndex = filterTableOffset + ToInt(mul<filterTableSize / 4>(frac(x)));
				*cptr = (*pBlender_)(color, (epCorrectedAlpha * filterIndex[filterTable_]), *cptr);
				++cptr;
				*cptr = (*pBlender_)(color, (epCorrectedAlpha * filterIndex[filterTable_ - (filterTableSize / 4)]), *cptr);
				++cptr;
				*cptr = (*pBlender_)(color, (epCorrectedAlpha * filterIndex[filterTable_ - (filterTableSize / 2)]), *cptr);

				x += gradient;
				ptr += (ToInt(x) - prev_ixf) * sizeof(ColorT) + lineOffset;
				prev_ixf = ToInt(x);
				++scount;
				--loopCnt;
			}
		}
	}
	
public:

	LineDrawer_Nelson(Buffer2D<ColorT>* pBuff = NULL)
		:
		slopeCorrectionTable_(NULL),
		filterTable_(NULL)
	{
		SetBuffer(pBuff);
	}
	
	ColorBlenderT* pBlender_;
	void SetSlopeCorrectionTable(const NumericT* slopeCorrectionTable)		{	slopeCorrectionTable_ = slopeCorrectionTable;	}
	void SetFilterTable(const NumericT* filterTable)						{	filterTable_ = filterTable;	}
	
	void SetBuffer(Buffer2D<ColorT>* pBuff)
	{
		if (pBuff == NULL)
			return;
		pBuff_ = pBuff;
	}

	void DrawLine(ColorT color, NumericT x1, NumericT y1, NumericT x2, NumericT y2)
	{
		NumericT width = abs(x2 - x1);
		NumericT height = abs(y2 - y1);
		if (width + height == NumericT(0))
			return;
		drawLine(color, width, height, x1, y1, x2, y2);
	}

};

template <typename NumericT>
inline void SetupSlopeCorrectionTable(NumericT* table, const size_t tableSize)
{
	for (size_t i=0; i<tableSize; ++i) {
		double m = (i + 0.5) / tableSize;
		table[i] = sqrt(m*m + 1) * 0.707106781; /* (m+1)^2 / sqrt(2) */
	}
}

template <typename NumericT>
inline void SetupFilterTable(NumericT* table, const size_t tableSize, const double filterWidth = 0.75)
{
	size_t halfSize = tableSize / 2;
	for (size_t i=0; i<halfSize; ++i) {
		double d = (i + 0.5)/ (halfSize / 2.0);
		d = d / filterWidth;
		table[halfSize+i] = 1.0 / exp(d * d);
	}
	for (size_t i=0; i<halfSize; ++i) {
		table[i] = table[tableSize-i]; //tableSize-i-1 ?
	}
}

} // namespace gl

