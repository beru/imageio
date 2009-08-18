#pragma once

/*

四元正方行列

http://www12.plala.or.jp/ksp/formula/mathFormula/html/node79.html

*/

#include "glvector4.h"

#include <cmath>

namespace gl {

template <typename T>
class Matrix4
{
public:
	__declspec(align(16))
	union {
		struct {
			float
				n11, n12, n13, n14,
				n21, n22, n23, n24,
				n31, n32, n33, n34,
				n41, n42, n43, n44
			;
		};
		T t[4][4];
		T a[16];
	};

	T& operator () (int ri, int ci);
	const T& operator () (int ri, int ci) const;
	
	template <typename T2>
	Matrix4& operator *= (T2 v);
	
	template <typename T2>
	Matrix4& operator /= (T2 v);
	
	Matrix4& operator *= (const Matrix4& m);
	
	void Identity();
	Matrix4& Transpose();
	
	static Matrix4& Multiply(const Matrix4& a, const Matrix4& b, Matrix4& c);
	static Matrix4& MultiplyTranspose(const Matrix4& a, const Matrix4& b, Matrix4& c);
	
	template <typename T2>
	static Matrix4& RotationX(Matrix4& m, T2 angle);
	
	template <typename T2>
	static Matrix4& RotationY(Matrix4& m, T2 angle);
	
	template <typename T2>
	static Matrix4& RotationZ(Matrix4& m, T2 angle);
	
	template <typename T2>
	static Matrix4& Scaling(Matrix4& m, T2 sx, T2 sy, T2 sz);
	
	template <typename T2>
	static Matrix4& Translation(Matrix4& m, T2 x, T2 y, T2 z);
	
};

template <typename T>
T& Matrix4<T>::operator () (int ri, int ci)
{
	return t[ri][ci];
}

template <typename T>
const T& Matrix4<T>::operator () (int ri, int ci) const
{
	return t[ri][ci];
}

template <typename T>
template <typename T2>
Matrix4<T>& Matrix4<T>::operator *= (T2 v)
{
	for (size_t i=0; i<16; ++i) {
		a[i] *= v;
	}
	return *this;
}

template <typename T>
template <typename T2>
Matrix4<T>& Matrix4<T>::operator /= (T2 v)
{
	for (size_t i=0; i<16; ++i) {
		a[i] /= v;
	}
	return *this;
}

template <typename T>
Matrix4<T> operator * (const Matrix4<T>& a, const Matrix4<T>& b)
{
	Matrix4 c;
	Matrix4::Multiply(a, b, c);
	return c;
}

template <typename T, typename T2>
Matrix4<T> operator * (const Matrix4<T>& a, T2 b)
{
	Matrix4<T> c = a;
	return c *= b;
}

template <typename T, typename T2>
Matrix4<T> operator * (T2 a, const Matrix4<T>& b)
{
	Matrix4<T> c = b;
	return b *= a;
}

template <typename T, typename T2>
Matrix4<T> operator / (const Matrix4<T>& a, T2 b)
{
	Matrix4<T> c = a;
	return c /= b;
}

template <typename T, typename T2>
Matrix4<T> operator / (T2 a, const Matrix4<T>& b)
{
	Matrix4<T> c = b;
	return b /= a;
}

template <typename T>
Matrix4<T>& Matrix4<T>::operator *= (const Matrix4<T>& m)
{
	Matrix tmp = *this;
	*this = tmp * m;
}

template <typename T>
void Matrix4<T>::Identity()
{
	t[0][0] = 1; t[0][1] = 0; t[0][2] = 0; t[0][3] = 0;
	t[1][0] = 0; t[1][1] = 1; t[1][2] = 0; t[1][3] = 0;
	t[2][0] = 0; t[2][1] = 0; t[2][2] = 1; t[2][3] = 0;
	t[3][0] = 0; t[3][1] = 0; t[3][2] = 0; t[3][3] = 1;
}

template <typename T>
Matrix4<T>& Matrix4<T>::Transpose()
{
	std::swap(a[0][1], a[1][0]);
	std::swap(a[0][2], a[2][0]);
	std::swap(a[0][3], a[3][0]);
	std::swap(a[1][2], a[2][1]);
	std::swap(a[1][3], a[3][1]);
	std::swap(a[2][3], a[3][2]);
}

template <typename T>
Matrix4<T>& Matrix4<T>::Multiply(const Matrix4<T>& a, const Matrix4<T>& b, Matrix4<T>& c)
{
	for (size_t r=0; r<4; ++r) {
		for (size_t c=0; c<4; ++c) {
			c(r, c) = a(r,0)*b(0,c) + a(r,1)*b(1,c) + a(r,2)*b(2,c) + a(r,3)*b(3,c);
		}
	}
	return c;
}

template <typename T>
Matrix4<T>& Matrix4<T>::MultiplyTranspose(const Matrix4<T>& a, const Matrix4<T>& b, Matrix4<T>& c)
{
	Matrix4::Multiply(a, b, c);
	c.Transpose();
	return c;
}

template <typename T>
Vector4<T> operator * (const Matrix4<T>& m, const Vector4<T>& v)
{
	return
		Vector4<T>(
			m.n11*v.x + m.n12*v.y + m.n13*v.z + m.n14,
			m.n21*v.x + m.n22*v.y + m.n23*v.z + m.n24,
			m.n31*v.x + m.n32*v.y + m.n33*v.z + m.n34,
			m.n41*v.x + m.n42*v.y + m.n43*v.z + m.n44
		);
}

// X軸中心回転
// 1    0    0    0
// 0    cos  -sin 0
// 0    sin  cos  0
// 0    0    0    1
template <typename T>
template <typename T2>
Matrix4<T>& Matrix4<T>::RotationX(Matrix4<T>& m, T2 angle)
{
	m.Identity();
	T s = sin(angle);
	T c = cos(angle);
	m.n22 = c;
	m.n23 = -s;
	m.n32 = s;
	m.n33 = c;
	return m;
}

// Y軸中心回転
// cos  0  sin 0
// 0    1  0   0
// -sin 0  cos 0
// 0    0  0   1
template <typename T>
template <typename T2>
Matrix4<T>& Matrix4<T>::RotationY(Matrix4<T>& m, T2 angle)
{
	m.Identity();
	T s = sin(angle);
	T c = cos(angle);
	m.n11 = c;
	m.n13 = s;
	m.n31 = -s;
	m.n33 = c;
	return m;
}

// Z軸中心回転
// cos -sin 0 0
// sin cos  0 0
// 0   0    1 0
// 0   0    0 1
template <typename T>
template <typename T2>
Matrix4<T>& Matrix4<T>::RotationZ(Matrix4<T>& m, T2 angle)
{
	m.Identity();
	T s = sin(angle);
	T c = cos(angle);
	m.n11 = c;
	m.n12 = -s;
	m.n21 = s;
	m.n22 = c;
	return m;
}

// 拡大縮小行列
// a 0 0 0
// 0 b 0 0
// 0 0 c 0
// 0 0 0 1
template <typename T>
template <typename T2>
Matrix4<T>& Matrix4<T>::Scaling(Matrix4<T>& m, T2 sx, T2 sy, T2 sz)
{
	m.Identity();
	m.n11 = sx;
	m.n22 = sy;
	m.n33 = sz;
	return m;
}

// 平行移動行列
// 1 0 0 x
// 0 1 0 y
// 0 0 1 z
// 0 0 0 1
template <typename T>
template <typename T2>
Matrix4<T>& Matrix4<T>::Translation(Matrix4<T>& m, T2 x, T2 y, T2 z)
{
	m.Identity();
	m.n14 = x;
	m.n24 = y;
	m.n34 = z;
	return m;
}

} // namespace gl

