#pragma once

/*!
	���w�֌W�̋L�q��⏕

*/

#include  <math.h>
#include <vector>
#include <numeric>

#define _USE_MATH_DEFINES

#define M_PI 3.14159265358979323846 

#include <math.h>

template <typename Type, Type from, Type to>
bool IsInRange(Type val)
{
	assert(from <= to);
	return (val >= from && val <= to);
}

template <typename T>
bool IsInRange(T from, T value, T to)
{
	if (from < to) {
		return (from <= value && value <= to);
	}else {
		return (to <= value && value <= from);
	}
}

//! �p�x�A�x����radian�ɕϊ�
template <typename T>
inline T degree2radian(const T& degree) {
	return degree * (M_PI / 180.0);
}

//! �p�x�Aradian����x�ɕϊ�
template <typename T>
inline T radian2degree(const T& radian) {
	return radian * (180.0 / M_PI);
}

/*!
	@brief 	�ȈՌv�Z�p�̐��w�֐�utility�Q
*/

template <typename T>
inline T HalfAdjust(T val)
{
	return (val + 0.5);
}

//! �����_1���l�̌ܓ�
template <typename T>
inline int HalfAdjustInt(T val)
{
	return (int) HalfAdjust(val);
}

//! ���������߂鎮 a^2 + b^2 = c^2
template <typename T>
inline T CalcLength(T xlen, T ylen)
{
	return sqrt(
		(xlen * xlen) + (ylen * ylen)
	);
}

//! RGB�l����P�x�����߂�B
inline double calcBrightnessFromRGB(double r, double g, double b)
{
	return (0.298912 * r) + (0.586611 * g) + (0.114478 * b);
}

//! �l����
template <typename T>
inline T limitValue(T val, T min, T max)
{
#if 0
	return std::max(min, std::min(val, max));
#else
	if (val < min) return min;
	if (val > max) return max;
	return val;
#endif
}

/*!
	�񌳘A��������������
	y1 = @a x1 + @B
	y2 = @a x2 + @B
*/
template <typename T>
inline void SolveSimultaneousLinearEquations(T x1, T y1, T x2, T y2, T& a, T& b)
{
	if (x1 == y1 && x2 == y2)	// �ϊ�����
	{
		a = 1;
		b = 0;
		return;
	}
	else if (x1 == x2)			// ���̕�������
	{
		a = 1;
		b = 0;
	}
	else
	{
		a = (y2 - y1) / (x2 - x1);
		b = y1 - (x1 * a);
	}
}


/*!
	width1, height1�̐}�`���Awidth2, height2��fit����ׂ̕ϊ�scale���Z�o
	���A�����A�������l���B
*/
double CalcFitScale(double width1, double height1, double width2, double height2);

/*
	@param val	0�`1�ɐ��K�����ꂽ�l
*/
template <typename T>
T gamma(const T& val, const T& gamma)
{
	return pow(val, gamma);
}

/*!
	@param src �z��̐擪�̃A�h���X
	@param cnt ��������z��̒���
	@param gamma �K�p����gamma�l
	@param min, max �ŏ��A�ő�
*/
template <typename T>
std::vector<T> gammaArray(
	const T* src, size_t cnt, const T& gamma,
	const T& min, const T& max)
{
	std::vector<T> ret(cnt);
	T diff = max - min;
	T factor = 1.0 / diff;
	for (size_t i=0; i<cnt; ++i)
	{
		// ���K�� 0.0 �` 1.0�͈̔͂ɐ��K��
		T normalized = (src[i] - min) * factor;
		if (normalized < 0.0)
			normalized = 0.0;
		T tmp = ::gamma(normalized, gamma);
		ret[i] = diff * tmp + min;
	}
	return ret;
}

/*!
	��_���Abegin�����_�Ƃ��āAend���A�O�p�`�̒��_�Ƃ����ꍇ�̊p�x�����߂�B
	@ret radian
*/
inline double CalcAngleBy2Point(POINT& begin, POINT& end)
{
	int x = end.x - begin.x;
	int y = -1 * (end.y - begin.y);
	double rad = atan2((double)y, (double)x);
	return rad;
}

/*!
	Gaussian �֐�
	
	N �K�i���ϐ�
	��^2 ���U
	
	
       1       x^2
f(x) = - exp(- �\�\ )
       N       2��^2

	@param [in] x
	@param [in] invDispersion ���U ��2�̋t��
*/
double gaussian(double x, double invDispersion);

/*!
	@param [out] pArray ���ʂ��������ޔz��
	@param [in] count ��������v�f��
	@param [in] n �K�i���萔
	@param [in] halfBandWidth ���l��
*/
void gaussianArray(double* pArray, size_t count, double n, double halfBandWidth);

/*!
	���������_�^�̊ۂ�
	@param decimal �����_�扽�����ۂ߂邩�B
*/
double fround(double val, size_t decimal);

/*!
	�����_�����܂Ŏc����
	���������_�^�ł͂Ȃ��ABCD�^�ɑ΂��Ă��ׂ������B�B
*/
double LimitDecimalPlace(double val, size_t decimalPlace);

// http://jeanne.wankuma.com/tips/math/rounddown.html
double roundDown(double val, size_t digits);

/*!
	�g�`�̔����𑽒i�K�ɋ��߂�֐�
	�ړ����ϖ@�Ŋۂ߂��������B
*/
template <typename T, typename PairInputIterator>
void CalcDerivations(
	const std::vector<T>& src,
	PairInputIterator movingAverageParaBegin, 	PairInputIterator movingAverageParaEnd,
	std::vector<std::vector<T> >& derivations)
{
	std::vector<T> wrk(src);
	std::vector<T> wrk2(wrk.size());
	size_t movingAverageParaSize = (movingAverageParaEnd - movingAverageParaBegin);
	for (size_t i=0; i<derivations.size(); ++i)
	{
		if (movingAverageParaSize > i)
			MovingAverage(&wrk[0], &wrk2[0], wrk.size(), (movingAverageParaBegin + i)->first, (movingAverageParaBegin + i)->second);
		else
			wrk2 = wrk;
		wrk.clear();
		std::adjacent_difference(wrk2.begin(), wrk2.end(), std::back_inserter(wrk));
		derivations[i] = wrk;
	}
}

