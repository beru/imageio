#pragma once

/*
	色の情報をカプセル化。

*/

#include <memory.h>
#define NOMINMAX
#include <limits>
#undef min
#undef max

#include "common.h"

namespace gl
{

template <typename ComponentT>
struct ColorRGBA
{
	typedef typename ComponentT value_type;
	ComponentT r,g,b,a;
};

template <typename ComponentT>
struct ColorBGRA
{
	typedef typename ComponentT value_type;
	ComponentT b,g,r,a;
};

template <typename ComponentT>
struct ColorARGB
{
	typedef typename ComponentT value_type;
	ComponentT a,r,g,b;
};

template <typename ComponentT>
struct ColorABGR
{
	typedef typename ComponentT value_type;
	ComponentT a,b,g,r;
};

template <typename ComponentT>
struct ColorRGB
{
	typedef typename ComponentT value_type;
	ComponentT r,g,b;
};

template <typename ComponentT>
struct ColorBGR
{
	typedef typename ComponentT value_type;
	ComponentT b,g,r;
};

template <typename T>
struct Color1
{
	static const size_t samplesPerPixel = 1;
	typedef typename T value_type;
	T a;
	
	Color1(
		T a = OneMinusEpsilon(T(1))
		)
		:
		a(a)
	{
	}

	Color1& operator -= (Color1 col)
	{
		a -= col.a;
		return *this;
	}

	Color1 operator - (const Color1 col) const
	{
		Color1 tmp = *this;
		return tmp -= col;
	}

	Color1& operator += (const Color1 col)
	{
		a += col.a;
		return *this;
	}

	Color1 operator + (const Color1 col) const
	{
		Color1 tmp = *this;
		return tmp += col;
	}
	
	template <typename NumericT>
	Color1& operator /= (const NumericT num)
	{
		a /= num;
		return *this;
	}

	template <typename NumericT>
	Color1 operator / (const NumericT num) const
	{
		Color1 tmp = *this;
		return tmp /= num;
	}

	template <typename NumericT>
	Color1& operator *= (const NumericT num)
	{
		a *= num;
		return *this;
	}

	template <typename NumericT>
	Color1 operator * (const NumericT num) const
	{
		Color1 tmp = *this;
		return tmp *= num;
	}
	
	template <typename T>
	bool operator < (Color1<T> c) const
	{
		return a < c.a;
	}
	
};

template <typename ColorBase>
struct Color3;

template <typename ColorBase>
struct Color4 : public ColorBase
{
	static const size_t samplesPerPixel = 4;
	typedef typename ColorBase::value_type T;

	Color4(T v=OneMinusEpsilon(T(1)), T alpha = OneMinusEpsilon(T(1)))
	{
		r = (v);
		g = (v);
		b = (v);
		a = (alpha);
	}

	template <typename ColorBase2T>
	Color4(const Color3<ColorBase2T>& c, T alpha = OneMinusEpsilon(T(1)))
	{
		r = T(c.r);
		g = T(c.g);
		b = T(c.b);
		a = alpha;
	}

	template <typename ColorBase2T>
	Color4(const Color4<ColorBase2T>& c)
	{
		r = T(c.r);
		g = T(c.g);
		b = T(c.b);
		a = T(c.a);
	}

	template <typename ValueT>
	Color4(const Color1<ValueT>& c)
	{
		r = g = b = a = T(c.a);
	}

	Color4(T red, T green, T blue, T alpha = OneMinusEpsilon(T(1)))
	{
		r = (red);
		g = (green);
		b = (blue);
		a = (alpha);
	}

	Color4& operator -= (const Color4& col)
	{
		r -= col.r;
		g -= col.g;
		b -= col.b;
		a -= col.a;
		return *this;
	}

	Color4 operator - (const Color4& col) const
	{
		Color4 tmp = *this;
		return tmp -= col;
	}

	template <typename ColorBaseT>
	Color4& operator += (const Color3<ColorBaseT>& col)
	{
		r += col.r;
		g += col.g;
		b += col.b;
		return *this;
	}
		
	Color4& operator += (const Color4& col)
	{
		r += col.r;
		g += col.g;
		b += col.b;
		a += col.a;
		return *this;
	}

	Color4 operator + (const Color4& col) const
	{
		Color4 tmp = *this;
		return tmp += col;
	}
	
	template <typename NumericT>
	Color4& operator /= (NumericT num)
	{
		r /= num;
		g /= num;
		b /= num;
		a /= num;
		return *this;
	}

	template <typename NumericT>
	Color4 operator / (NumericT num) const
	{
		Color4 tmp = *this;
		return tmp /= num;
	}

	template <typename NumericT>
	Color4& operator *= (NumericT num)
	{
		r *= num;
		g *= num;
		b *= num;
		a *= num;
		return *this;
	}

	template <typename NumericT>
	Color4 operator * (NumericT num) const
	{
		Color4 tmp = *this;
		tmp *= num;
		return tmp;
	}
	
};

template <typename ColorBase>
struct Color3 : public ColorBase
{
	static const size_t samplesPerPixel = 3;
	typedef typename ColorBase::value_type T;
	
	Color3()
	{
	}

	Color3(T v)
	{
		r = v;
		g = v;
		b = v;
	}

	Color3(T red, T green, T blue)
	{
		r = red;
		g = green;
		b = blue;
	}

	Color3(T red, T green, T blue, T alpha)
	{
		r = red;
		g = green;
		b = blue;
	}
	
	template <typename ValueT>
	Color3(Color1<ValueT> c)
	{
		r = T(c.a);
		g = T(c.a);
		b = T(c.a);
	}

	template <typename ColorBaseT>
	Color3(const Color3<ColorBaseT>& c)
	{
		r = T(c.r);
		g = T(c.g);
		b = T(c.b);
	}

	template <typename ColorBaseT>
	Color3(const Color4<ColorBaseT>& c)
	{
		r = T(c.r);
		g = T(c.g);
		b = T(c.b);
	}

	__forceinline Color3& operator -= (const Color3& col)
	{
		r -= col.r;
		g -= col.g;
		b -= col.b;
		return *this;
	}

	__forceinline Color3 operator - (const Color3& col) const
	{
		Color3 tmp = *this;
		return tmp -= col;
	}

	template <typename ColorBaseT>
	__forceinline Color3& operator += (const Color3<ColorBaseT>& col)
	{
		r += col.r;
		g += col.g;
		b += col.b;
		return *this;
	}

	__forceinline Color3 operator + (const Color3& col) const
	{
		Color3 tmp = *this;
		return tmp += col;
	}
	
	template <typename NumericT>
	Color3& operator /= (NumericT num)
	{
		r /= num;
		g /= num;
		b /= num;
		return *this;
	}

	template <typename NumericT>
	Color3 operator / (NumericT num) const
	{
		Color3 tmp = *this;
		return tmp /= num;
	}

	template <typename NumericT>
	__forceinline Color3& operator *= (NumericT num)
	{
		r *= num;
		g *= num;
		b *= num;
		return *this;
	}

	template <typename ColorBaseT>
	__forceinline Color3& operator *= (const Color3<ColorBaseT>& c)
	{
		r *= c.r;
		g *= c.g;
		b *= c.b;
		return *this;
	}

	template <typename NumericT>
	__forceinline Color3 operator * (NumericT num) const
	{
		Color3 tmp = *this;
		tmp *= num;
		return tmp;
	}
	
};

// http://ja.wikipedia.org/wiki/%E3%82%B3%E3%83%B3%E3%83%9D%E3%83%BC%E3%83%8D%E3%83%B3%E3%83%88%E6%98%A0%E5%83%8F%E4%BF%A1%E5%8F%B7
template <typename NumericT>
NumericT RGB_Luminosity(NumericT r, NumericT g, NumericT b)
{
	return double(r) * 0.299 + double(g) * 0.587 + double(b) * 0.114;
}

template <typename NumericT>
inline void RGB_2_YCbCr(
	NumericT r, NumericT g, NumericT b,
	NumericT& y, NumericT& cb, NumericT& cr
	)
{
	y = double(r) * 0.299 + double(g) * 0.587 + double(b) * 0.114;
	cb = 0.564 * (b - y);
	cr = 0.713 * (r -y);
}

template <typename NumericT>
inline void YCbCr_2_RGB(
	NumericT y, NumericT cb, NumericT cr,
	NumericT& r, NumericT& g, NumericT& b
	)
{
	r = cr / 0.713 + y;
	b = cb / 0.564 + y;
	g = (y - 0.299 * r - 0.114 * b) / 0.587;
}

// http://image-d.isp.jp/commentary/color_cformula/HLS.html
template <typename NumericT>
NumericT RGB_Saturation(NumericT r, NumericT g, NumericT b)
{
	NumericT max = std::max(std::max(r,g),b);
	NumericT min = std::min(std::min(r,g),b);
	NumericT s = 0;
	if (max != min) {
		NumericT l = (max + min) / NumericT(2);
		if (l <= NumericT(0.5)) {
			s = (max - min) / (max + min);
		}else {
			s = (max - min) / (NumericT(2) - max - min);
		}
	}
	return s;
}

// http://www.cvl.iis.u-tokyo.ac.jp/~miyazaki/tech/tech01.html
template <typename NumericT>
inline void RGB_2_HSV(
	NumericT r, NumericT g, NumericT b,
	NumericT& h, NumericT& s, NumericT& v
	)
{
	NumericT cmax = max(r,g,b);
	NumericT cmin = min(r,g,b);
	NumericT cMaxMinusMin = cmax - cmin;
	
	v = cmax;
	
	if (cmax == 0) {
		s = 0;
	}else {
		s = cMaxMinusMin / cmax;
	}
	
	if (s == 0) {
		h = 0;
	}else {
		if (r == cmax) {
			h = (g - b) / cMaxMinusMin;
		}else if (g == cmax) {
			h = 2 + (b - r) / cMaxMinusMin;
		}else {
			h = 4 + (r - g) / cMaxMinusMin;
		}
		h *= 60;
		if (h < 0) {
			h += 360;
		}
	}
}

template <typename NumericT>
inline void HSV_2_RGB(
	NumericT h, NumericT s, NumericT v,
	NumericT& r, NumericT& g, NumericT& b
	)
{
	NumericT fint;
	NumericT f = modf(h / 60, &fint);
	size_t intVal = (size_t) fint;
	
	NumericT p1 = v * (1 - s);
	NumericT p2 = v * (1 - s * f);
	NumericT p3 = v * (1 - s * (1 - f));
	
	switch (intVal) {
	case 0:
		r = v;
		g = p3;
		b = p1;
		break;
	case 1:
		r = p2;
		g = v;
		b = p1;
		break;
	case 2:
		r = p1;
		g = v;
		b = p3;
		break;
	case 3:
		r = p1;
		g = p2;
		b = v;
		break;
	case 4:
		r = p3;
		g = p1;
		b = v;
		break;
	case 5:
		r = v;
		g = p1;
		b = p2;
		break;
	default:
		assert(false);
	}
	
}

// http://image-d.isp.jp/commentary/color_cformula/HLS.html
template <typename NumericT>
inline void RGB_2_HLS(
	NumericT r, NumericT g, NumericT b,
	NumericT& h, NumericT& l, NumericT& s
	)
{
	NumericT vmax = max(r, g, b);
	NumericT vmin = min(r, g, b);
	
	l = (vmax + vmin) * NumericT(0.5);
	s = 0;
	h = 0;
	
	NumericT vMaxMinusMin = vmax - vmin;
	if (vMaxMinusMin < std::numeric_limits<NumericT>::epsilon()) {
		return;
	}
	
	if (l <= NumericT(0.5)) {
		s = vMaxMinusMin / (vmax + vmin);
	}else {
		s = vMaxMinusMin / (2 - vMaxMinusMin);
	}
	NumericT cr = (vmax - r) / vMaxMinusMin;
	NumericT cg = (vmax - g) / vMaxMinusMin;
	NumericT cb = (vmax - b) / vMaxMinusMin;
	
	if (r == vmax) {
		h = cg - cb;
	}else if (g == vmax) {
		h = 2 + cr - cb;
	}else {
		h = 4 + cg - cr;
	}
	h *= 60;
	if (h < 0) {
		h += 360;
	}
}

template <typename NumericT>
inline void HLS_2_RGB(
	NumericT oh, NumericT ol, NumericT os,
	NumericT& r, NumericT& g, NumericT& b
	)
{
	if (os < std::numeric_limits<NumericT>::epsilon()) {
		r = g = b = ol;
		return;
	}
	
	NumericT vmax = (ol <= 0.5) ? (ol * (1 + os)) : (ol * (1 - os) + os);
	NumericT vmin = 2 * ol - vmax;
	NumericT vMaxMinusMin = vmax - vmin;
	
	NumericT h;
	
	// R
	h = oh + 120;
	if (360 <= h) {
		h -= 360;
	}
	if (h < 60) {
		r = vmin + vMaxMinusMin * (h / 60);
	}else if (h < 180) {
		r = vmax;
	}else if (h < 240) {
		r = vmin + vMaxMinusMin * (240 - h) / 60;
	}else {
		r = vmin;
	}
	
	// G
	h = oh;
	if (h < 60) {
		g = vmin + vMaxMinusMin * (h / 60);
	}else if (h < 180) {
		g = vmax;
	}else if (h < 240) {
		g = vmin + vMaxMinusMin * (240 - h) / 60;
	}else {
		g = vmin;
	}
	
	// B
	h = oh - 120;
	if (h < 0) {
		h += 360;
	}
	if (h < 60) {
		g = vmin + vMaxMinusMin * (h / 60);
	}else if (h < 180) {
		g = vmax;
	}else if (h < 240) {
		g = vmin + vMaxMinusMin * (240 - h) / 60;
	}else {
		g = vmin;
	}
	
}

}	// namespace gl

