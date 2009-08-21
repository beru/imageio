#pragma once

/*!
	数学関係の記述を補助

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

//! 角度、度からradianに変換
template <typename T>
inline T degree2radian(const T& degree) {
	return degree * (M_PI / 180.0);
}

//! 角度、radianから度に変換
template <typename T>
inline T radian2degree(const T& radian) {
	return radian * (180.0 / M_PI);
}

/*!
	@brief 	簡易計算用の数学関数utility群
*/

template <typename T>
inline T HalfAdjust(T val)
{
	return (val + 0.5);
}

//! 小数点1桁四捨五入
template <typename T>
inline int HalfAdjustInt(T val)
{
	return (int) HalfAdjust(val);
}

//! 長さを求める式 a^2 + b^2 = c^2
template <typename T>
inline T CalcLength(T xlen, T ylen)
{
	return sqrt(
		(xlen * xlen) + (ylen * ylen)
	);
}

//! RGB値から輝度を求める。
inline double calcBrightnessFromRGB(double r, double g, double b)
{
	return (0.298912 * r) + (0.586611 * g) + (0.114478 * b);
}

//! 値制限
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
	二元連立方程式を解く
	y1 = @a x1 + @B
	y2 = @a x2 + @B
*/
template <typename T>
inline void SolveSimultaneousLinearEquations(T x1, T y1, T x2, T y2, T& a, T& b)
{
	if (x1 == y1 && x2 == y2)	// 変換無し
	{
		a = 1;
		b = 0;
		return;
	}
	else if (x1 == x2)			// 元の幅が無し
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
	width1, height1の図形が、width2, height2にfitする為の変換scaleを算出
	幅、高さ、両方を考慮。
*/
double CalcFitScale(double width1, double height1, double width2, double height2);

/*
	@param val	0～1に正規化された値
*/
template <typename T>
T gamma(const T& val, const T& gamma)
{
	return pow(val, gamma);
}

/*!
	@param src 配列の先頭のアドレス
	@param cnt 処理する配列の長さ
	@param gamma 適用するgamma値
	@param min, max 最小、最大
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
		// 正規化 0.0 ～ 1.0の範囲に正規化
		T normalized = (src[i] - min) * factor;
		if (normalized < 0.0)
			normalized = 0.0;
		T tmp = ::gamma(normalized, gamma);
		ret[i] = diff * tmp + min;
	}
	return ret;
}

/*!
	二点を、beginを原点として、endを、三角形の頂点とした場合の角度を求める。
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
	Gaussian 関数
	
	N 規格化変数
	σ^2 分散
	
	
       1       x^2
f(x) = - exp(- ―― )
       N       2σ^2

	@param [in] x
	@param [in] invDispersion 分散 σ2の逆数
*/
double gaussian(double x, double invDispersion);

/*!
	@param [out] pArray 結果を書き込む配列
	@param [in] count 処理する要素数
	@param [in] n 規格化定数
	@param [in] halfBandWidth 半値幅
*/
void gaussianArray(double* pArray, size_t count, double n, double halfBandWidth);

/*!
	浮動小数点型の丸め
	@param decimal 小数点第何桁を丸めるか。
*/
double fround(double val, size_t decimal);

/*!
	小数点何桁まで残すか
	浮動小数点型ではなく、BCD型に対してやるべきかも。。
*/
double LimitDecimalPlace(double val, size_t decimalPlace);

// http://jeanne.wankuma.com/tips/math/rounddown.html
double roundDown(double val, size_t digits);

/*!
	波形の微分を多段階に求める関数
	移動平均法で丸めたりもする。
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

