#pragma once


/*!	@file
	@brief 二次元ベクトル
*/

#include <limits>

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
// #include "MathUtil.h"

namespace gl
{

/*!	@class
	二次元ベクトル用のclass template
*/
template <typename T>
class Vector2
{
public:
	T x;
	T y;
	
	Vector2()
	{}
	
	Vector2(const T& x_, const T& y_)
		:
		x(x_),
		y(y_)
	{
	}

	Vector2(const Vector2<T>& vec)
		:
		x(vec.x),
		y(vec.y)
	{
	}
	
	void Set(const T& x_, const T& y_) {
		x = x_;
		y = y_;
	}
	
	bool operator == (const Vector2& vec) const {
		return 1
			&& (abs(x - vec.x) <= DBL_EPSILON)
			&& (abs(y - vec.y) <= DBL_EPSILON)
		;
	}
	bool operator != (const Vector2& vec) const {
		return !operator==(vec);
	}

	Vector2 operator ! () const {
		return Vector2(-x, -y);
	}
	
	Vector2 operator += (const Vector2& vec) {
		x += vec.x;
		y += vec.y;
		return *this;
	}
	
	Vector2 operator + (const Vector2& vec) const {
		Vector2 ret = *this;
		return ret += vec;
	}
	
	
	Vector2 operator -= (const Vector2& vec) {
		x -= vec.x;
		y -= vec.y;
		return *this;
	}
	
	Vector2 operator - () const {
		return Vector2(-x, -y);
	}
	
	Vector2 operator - (const Vector2& vec) const {
		Vector2 ret = *this;
		return ret -= vec;
	}
	
	Vector2 operator *= (const T& t) {
		x *= t;
		y *= t;
		return *this;
	}
	
	Vector2 operator * (const T& t) const {
		return Vector2(x * t, y * t);
	}
	
	Vector2 operator /= (const T& t) {
		x /= t;
		y /= t;
		return *this;
	}
	
	Vector2 operator / (const T& t) const {
		Vector2 ret = *this;
		return ret /= t;
	}
	
	//! 内積
	T DotProduct(const Vector2& v) const {
		return Vector2::DotProduct(*this, v);
	}
	
	T DotProduct(const Vector2& lhs, const Vector2& rhs) const {
		return (lhs.x * rhs.x + lhs.y * rhs.y);
	}
	
	//! 外積
	T CrossProduct(const Vector2& vec) const {
		return Vector2::CrossProduct(*this, vec);
	}
	
	static T CrossProduct(const Vector2& lhs, const Vector2& rhs) {
		return lhs.x * rhs.y - lhs.y * rhs.x;
	}
	
	//! 長さの二乗
	T LengthSquare() const {
		return x*x + y*y;
	}
	
	//! 長さ
	T Length() const {
		return sqrt(LengthSquare());
	}
	
	T Scalar() const {
		return Length();
	}
	
	//! 正規化して、純粋に方向だけを表すベクトルにする。
	Vector2 Normalize() const {
		return *this / Scalar();
	}

/*	
	friend Vector2 operator + (T n, const Vector2& vec);
	friend Vector2 operator - (T n, const Vector2& vec);
	friend Vector2 operator * (T n, const Vector2& vec);
	friend Vector2 operator / (T n, const Vector2& vec);
*/

};

} // namespace gl
