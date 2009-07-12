#pragma once

/*!
	四元ベクトル
*/

namespace gl {

template <typename T>
class Vector4
{
public:
	__declspec(align(16))
	union {
		struct {
			T x,y,z,w;
		};
		T values[4];
	};
	
	Vector4(T x = 0, T y = 0, T z = 0, T w = 0);

	const Vector4& operator += (const Vector4& v);
	Vector4 operator + () const;
	Vector4 operator + (const Vector4& v) const;
	
	const Vector4& operator -= (const Vector4& v);
	Vector4 operator - () const;
	Vector4 operator - (const Vector4& v) const;

	template <typename T2>
	Vector4 operator *= (T2 v);
	template <typename T2>
	Vector4 operator /= (T2 v);

	T DotProduct(const Vector4& v) const;
	
};

template <typename T>
Vector4<T>::Vector4(T x, T y, T z, T w)
	:
	x(x),
	y(y),
	z(z),
	w(w)
{
}

template <typename T>
const Vector4<T>& Vector4<T>::operator += (const Vector4<T>& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
	return *this;
}

template <typename T>
Vector4<T> Vector4<T>::operator + () const
{
	return *this;
}

template <typename T>
Vector4<T> Vector4<T>::operator + (const Vector4<T>& v) const
{
	Vector4<T> a = *this;
	return a+=v;
}

template <typename T>
const Vector4<T>& Vector4<T>::operator -= (const Vector4<T>& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
	return *this;
}

template <typename T>
Vector4<T> Vector4<T>::operator - () const
{
	return Vector4(-x,-y,-z,-w);
}

template <typename T>
Vector4<T> Vector4<T>::operator - (const Vector4<T>& v) const
{
	Vector4<T> a = *this;
	return a-=v;
}

template <typename T>
T Vector4<T>::DotProduct(const Vector4<T>& v) const
{
	return x*v.x + y*v.y + z*v.z + w*v.w;
}

template <typename T>
template <typename T2>
Vector4<T> Vector4<T>::operator *= (T2 v)
{
	x *= v;
	y *= v;
	z *= v;
	w *= v;
	return *this;
}

template <typename T>
template <typename T2>
Vector4<T> Vector4<T>::operator /= (T2 v)
{
	x /= v;
	y /= v;
	z /= v;
	w /= v;
	return *this;
}

template <typename T, typename T2>
Vector4<T> operator * (const Vector4<T>& v, T2 s)
{
	Vector4<T> v2 = v;
	return v2 *= s;
}

template <typename T, typename T2>
Vector4<T> operator * (T2 s, const Vector4<T>& v)
{
	return v * s;
}

template <typename T, typename T2>
Vector4<T> operator / (const Vector4<T>& v, T2 s)
{
	Vector4<T> v2 = v;
	return v2 /= s;
}

template <typename T, typename T2>
Vector4<T> operator / (T2 s, const Vector4<T>& v)
{
	return v / s;
}


} // namespace gl
