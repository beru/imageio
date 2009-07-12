#pragma once


/*!	@file
	@brief �񎟌��x�N�g��
*/

#include <limits>

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
// #include "MathUtil.h"

namespace gl
{

/*!	@class
	�񎟌��x�N�g���p��class template
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
	
	//! ����
	T DotProduct(const Vector2& v) const {
		return Vector2::DotProduct(*this, v);
	}
	
	T DotProduct(const Vector2& lhs, const Vector2& rhs) const {
		return (lhs.x * rhs.x + lhs.y * rhs.y);
	}
	
	//! �O��
	T CrossProduct(const Vector2& vec) const {
		return Vector2::CrossProduct(*this, vec);
	}
	
	static T CrossProduct(const Vector2& lhs, const Vector2& rhs) {
		return lhs.x * rhs.y - lhs.y * rhs.x;
	}
	
	//! �����̓��
	T LengthSquare() const {
		return x*x + y*y;
	}
	
	//! ����
	T Length() const {
		return sqrt(LengthSquare());
	}
	
	T Scalar() const {
		return Length();
	}
	
	//! ���K�����āA�����ɕ���������\���x�N�g���ɂ���B
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
