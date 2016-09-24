#pragma once

#include "glcolor.h"
#include "glfixed.h"

#include <dvec.h>

#include <boost/cstdint.hpp>

namespace gl
{

template<int shifts>
struct Color4 < ColorBGRA <fixed<shifts, uint32_t> > > : ColorBGRA <fixed<shifts, boost::uint32_t> >
{
	typedef typename ColorBGRA <fixed<shifts, uint32_t> >::value_type T;

	Color4(T v=OneMinusEpsilon(T(1)), T alpha = OneMinusEpsilon(T(1)))
	{
		r = (v);
		g = (v);
		b = (v);
		a = (alpha);
	}

	Color4(T red, T green, T blue, T alpha = OneMinusEpsilon(T(1)))
	{
		r = (red);
		g = (green);
		b = (blue);
		a = (alpha);
	}

	template <typename BaseColorT>
	Color4(const Color3<BaseColorT>& c)
	{
		r = T(c.r);
		g = T(c.g);
		b = T(c.b);
	}

	Color4& operator -= (Color4 col)
	{
		r -= col.r;
		g -= col.g;
		b -= col.b;
		a -= col.a;
		return *this;
	}

	Color4 operator - (Color4 col) const
	{
		Color4 tmp = *this;
		return tmp -= col;
	}

	Color4& operator += (Color4 col)
	{
		r += col.r;
		g += col.g;
		b += col.b;
		a += col.a;
		return *this;
	}

	template <typename BaseColorT>
	Color4& operator += (const Color3<BaseColorT>& col)
	{
		r += col.r;
		g += col.g;
		b += col.b;
		return *this;
	}

	Color4 operator + (Color4 col) const
	{
		Color4 tmp = *this;
		return tmp += col;
	}
	
	template <typename NumericT>
	Color4& operator /= (NumericT num)
	{
		r /= col.r;
		g /= col.g;
		b /= col.b;
		a /= col.a;
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


}	// namespace gl

