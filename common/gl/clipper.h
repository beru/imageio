#pragma once

/*
	������Clipping�����p
	
	http://www2.starcat.ne.jp/~fussy/algo/algo1-2.htm
	
	���Q�l�Ɏ����B
*/


#include "common.h"

namespace gl
{

template <typename T>
class Clipper
{
private:
	T minX_, minY_, maxX_, maxY_;
	
	enum {
		None = 0,
		Left = 1 << 1,
		Right = 1 << 2,
		Top = 1 << 3,
		Bottom = 1 << 4,
	};
	
	int calcSeqCode(T x, T y)
	{
		int code = None;
		if (x < minX_) code |= Left;
		if (x > maxX_) code |= Right;
		if (y < minY_) code |= Top;
		if (y > maxY_) code |= Bottom;
		return code;
	}
	
	int calcClippedPoint(int code, const T& x1, const T& y1, const T& x2, const T& y2, T* x, T* y)
	{
		T cx, cy;
		/* �E�B���h�E�̍��[���O���ɂ��� */
		if (code & Left) {
			if (x2 - x1 == T(0)) {
				cy = y1;
			}else {
				cy = (y2 - y1) * ((minX_ - x1) / (x2 - x1)) + y1;	/* ...(1) */
			}
			if (minY_<=cy && cy<=maxY_) {
				*x = minX_;
				*y = cy;
				return 1;  /* �G���A���Ɏ��܂�����I�� */
			}
		}

		/* �E�B���h�E�̉E�[���O���ɂ��� */
		if (code & Right) {
			if (x2 - x1 == T(0)) {
				cy = y1;
			}else {
				cy = (y2 - y1) * ((maxX_ - x1) / (x2 - x1)) + y1;	/* ...(1) */
			}
			if (minY_<=cy && cy<=maxY_) {
				*x = maxX_;
				*y = cy;
				return 1;  /* �G���A���Ɏ��܂�����I�� */
			}
		}

		/* �E�B���h�E�̏�[���O���ɂ��� */
		if (code & Top) {
			if (y2 - y1 == T(0)) {
				cx = x1;
			}else {
				cx = (x2 - x1) * ((minY_ - y1) / (y2 - y1)) + x1;
			}
			if (minX_<=cx && cx<=maxX_) {
				*x = cx;
				*y = minY_;	
				return 1;  /* �G���A���Ɏ��܂�����I�� */
			}
		}

		/* �E�B���h�E�̉��[���O���ɂ��� */
		if (code & Bottom) {
			if (y2 - y1 == T(0)) {
				cx = x1;
			}else {
				cx = (x2 - x1) * ((maxY_ - y1) / (y2 - y1)) + x1;
			}
			if (minX_<=cx && cx<=maxX_) {
				*x = cx;
				*y = maxY_;
				return 1;  /* �G���A���Ɏ��܂�����I�� */
			}
		}
		return -1;  /* �N���b�s���O����Ȃ������ꍇ�A�����͊��S�ɕs�� */
	}

public:
	Clipper(T minX, T minY, T maxX, T maxY)
	{
		setClipRegion(minX, minY, maxX, maxY);
	}

	Clipper()
	{
	}

	~Clipper() {}

	void SetClipRegion(T minX, T minY, T maxX, T maxY)
	{
		if (maxX < minX)
			std::swap(maxX, minX);
		if (maxY < minY)
			std::swap(maxY, minY);
		assert(minX >= T(0));
		assert(minY >= T(0));
		assert(maxX > T(0));
		assert(maxY > T(0));
		minX_ = minX;
		minY_ = minY;
		maxX_ = maxX;
		maxY_ = maxY;
	}
	
	T GetXMin() { return minX_; }
	T GetYMin() { return minY_; }
	T GetXMax() { return maxX_; }
	T GetYMax() { return maxY_; }

	/* �N���b�s���O���C�����[�`��
	>0 ... �N���b�s���O���ꂽ
	0  ... �N���b�s���O�̕K�v�Ȃ�
	<0 ... �����͊��S�ɕs�� */
	int ClipLinePoint(
		T* x1, T* y1,	/* �n�_ */
		T* x2, T* y2	/* �I�_ */
	)
	{
		/* �[�_���ރR�[�h */
		int code0, code1;
		code0 = calcSeqCode( *x1, *y1 );  /* �n�_�̒[�_���ރR�[�h�����߂� */
		code1 = calcSeqCode( *x2, *y2 );  /* �I�_�̒[�_���ރR�[�h�����߂� */

		/* �[�_���ރR�[�h���ǂ����0000�Ȃ�΃N���b�s���O�͕K�v�Ȃ� */
		if (code0==None && code1==None) return 0;
		
		/* �n�_��I�_�̒[�_���ރR�[�h�̘_���ς���O�Ȃ�ΐ����͊��S�ɕs�� */
		if (code0 & code1)
			return -1;
		/* �n�_�̃N���b�s���O */
		if (code0)
			if (calcClippedPoint( code0, *x1, *y1, *x2, *y2, x1, y1 ) < 0)
				return -1;

		/* �I�_�̃N���b�s���O */
		if (code1)
			if (calcClippedPoint( code1, *x1, *y1, *x2, *y2, x2, y2 ) < 0)
				return -1;

		return 1;
	}
	
};

} // namespace gl
