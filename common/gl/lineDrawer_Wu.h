#pragma once

/*

Wu, Xiaolin

http://freespace.virgin.net/hugo.elias/graphics/x_wuline.htm
http://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm

http://www.cs.nps.navy.mil/people/faculty/capps/iap/class1/lines/lines.html
���Q�l��C++�Ŏ����B

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
	Buffer2D<ColorT>*	pBuff_;		//!< �`���
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
	
	// �s�N�Z���̒����ʒu�����ɐ������W�ƔF�����Ă���B
	inline void drawLine(ColorT color, NumericT width, NumericT height, NumericT x1, NumericT y1, NumericT x2, NumericT y2)
	{
		NumericT palpha = NumericT(color.a);	// �`��F�̓����x
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
				NumericT xgap = NumericT(1) - frac(x1 + NumericT(0.5));		// �s�N�Z���̉���L��
				NumericT xend = halfAdjust(x1);								// �ŋߖT�̎�X���W�����߂�
				NumericT yend = y1 + gradient * (xend - x1);				// �s�N�Z���̒����ʒu�i���z�J�n�ʒu�j�܂�X���ړ������ꍇ��Y���W�����߂�
				ptr = (unsigned char*) pBuff_->GetPixelPtr(xend, ToInt(yend));

				NumericT dist = frac(yend);									// ���z�s�N�Z������Y���W����̋���
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
				NumericT xgap = frac(x2 + NumericT(0.5));			// �s�N�Z���̉���L��
				NumericT xend = halfAdjust(x2);						// �ŋߖT�̎�X���W�����߂�
				NumericT yend = y2 + gradient * (xend - x2);		// �s�N�Z���̒����ʒu�i���z�J�n�ʒu�j�܂�X���ړ������ꍇ��Y���W�����߂�

				NumericT dist = frac(yf);							// ���z�s�N�Z������Y���W����̋���
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
				NumericT ygap = NumericT(1) - frac(y1 + NumericT(0.5));		// �s�N�Z���̏c��L��
				NumericT yend = halfAdjust(y1);								// �ŋߖT�̎�Y���W�����߂�
				NumericT xend = x1 + gradient * (yend - y1);				// �s�N�Z���̒����ʒu�i���z�J�n�ʒu�j�܂�Y���ړ������ꍇ��X���W�����߂�
				ptr = (unsigned char*) pBuff_->GetPixelPtr(ToInt(xend), yend);

				NumericT dist = frac(xend);									// ���z�s�N�Z������X���W����̋���
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
				NumericT ygap = frac(y2 + NumericT(0.5));			// �s�N�Z���̏c��L��
				NumericT yend = halfAdjust(y2);						// �ŋߖT�̎�Y���W�����߂�
				NumericT xend = x2 + gradient * (yend - y2);		// �s�N�Z���̒����ʒu�i���z�J�n�ʒu�j�܂�Y���ړ������ꍇ��X���W�����߂�

				NumericT dist = frac(xf);							// ���z�s�N�Z������X���W����̋���
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

