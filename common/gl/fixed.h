#pragma once

/*
	固定小数点型
	
	http://shinh.skr.jp/template/gamenum.html
	を参考に実装。
	
*/

#include "common.h"
#include <math.h>

#include "xs_Float.h"

#include <memory>
#include <type_traits>
#include <limits>

namespace gl
{

template<int shifts, typename T = int>
class fixed
{
private:
	template <bool equal, bool leftLarge, int leftShifts, int rightShifts, typename RetT>
	struct ConvertHelper;

	template <int leftShifts, int rightShifts, typename RetT>
	struct ConvertHelper<true, false, leftShifts, rightShifts, RetT>
	{
		static RetT run(RetT v) { return v; }
	};

	template <int leftShifts, int rightShifts, typename RetT>
	struct ConvertHelper<false, true, leftShifts, rightShifts, RetT>
	{
		static RetT run(RetT v) { return v << (leftShifts - rightShifts); }
	};

	template <int leftShifts, int rightShifts, typename RetT>
	struct ConvertHelper<false, false, leftShifts, rightShifts, RetT>
	{
		static RetT run(RetT v) { return v >> (rightShifts - leftShifts); }
	};

	template <bool RetT, typename FixedT>
	struct Converter;

	template <typename FixedT>
	struct Converter<true, FixedT>
	{
		static T convert(FixedT v)
		{
			return ConvertHelper<
				(shifts == FixedT::shifts),
				(shifts > FixedT::shifts),
				shifts,
				FixedT::shifts,
				T
			>::run(v.value);
		}
	};

	template <typename FixedT>
	struct Converter<false, FixedT>
	{
		static typename FixedT::value_type convert(FixedT v)
		{
			return ConvertHelper<
				(shifts == FixedT::shifts),
				(shifts > FixedT::shifts),
				shifts,
				FixedT::shifts,
				FixedT::value_type
			>::run(v.value);			
		}
	};

public:
	static const T intFlag = (0xFFFFFFFFFFFFFFFF >> shifts) << shifts;
	static const T fracFlag = 0xFFFFFFFFFFFFFFFF ^ intFlag;

	typedef T value_type;
	static const int shifts = shifts;
	T value;

//-- constructors --/
	fixed() {}

	template <int rshifts, typename T2>
	fixed(fixed<rshifts,T2> val)
		:
		value(
			Converter<(sizeof(T) > sizeof(fixed<rshifts,T2>::value_type)), fixed<rshifts,T2> >::convert(val)
		)
	{}

	fixed(int v) : value(v << shifts) {}
	fixed(unsigned int v) : value(v << shifts) {}
#if defined(_M_IX86)
#else
	fixed(size_t v) : value(v << shifts) {}
#endif

	fixed(double v)
		:
		value( std::min<T>(T(v * (1i64 << shifts) + 0.5), std::numeric_limits<T>::max()) )
//		value( xs_Fix<shifts>::ToFix(v) )
	{
	}
	
//--- operators ---//
	fixed& operator = (T v) { value = v << shifts; return *this; }


	fixed& operator = (double v) { const double shifted = 1 << shifts; value = ToInt(v*shifted); return *this; }
	operator double() const { return value / (double)(1i64 << shifts); }
	operator float() const { return value / (float)(1i64 << shifts); }
	operator char() const { return value >> shifts; }
	operator unsigned char() const { return value >> shifts; }
	operator size_t() const { return value >> shifts; }
	operator int() const { return (value >> shifts); }

	template <int rshifts, typename T2>
	fixed& operator += (fixed<rshifts,T2> v)
	{
		value += Converter<(sizeof(T) > sizeof(fixed<rshifts,T2>::value_type)), fixed<rshifts,T2> >::convert(v);
		return *this;
	}

	template <int rshifts, typename T2>
	fixed operator + (fixed<rshifts,T2> v) const	{ fixed p=*this; return p+=v;; }

	fixed& operator ++ ()		{ value += (1 << shifts); return *this; }
	fixed operator ++ (int)	{ fixed tmp = *this; ++*this; return tmp; }
	
	template <int rshifts, typename T2>
	fixed& operator -= (fixed<rshifts,T2> v)
	{
		value -= Converter<(sizeof(T) > sizeof(fixed<rshifts,T2>::value_type)), fixed<rshifts,T2> >::convert(v);
		return *this;
	}

	template <int rshifts, typename T2>
	fixed operator - (fixed<rshifts,T2> v) const  { fixed p = *this; return p -= v; }

	fixed operator - () const { fixed p = *this; p.value = -p.value; return p; }
	fixed& operator -- ()		{ value -= (1 << shifts); return *this; }
	fixed& operator -- (int)	{ fixed tmp = *this; --*this; return tmp; }
	
	template <int rshifts, typename T2>
	__forceinline fixed& operator *= (fixed<rshifts,T2> v)
	{
		if (rshifts >= 32) {
			value = (__int64(value) * v.value) >> rshifts;
		}else {
			value = mulShift<rshifts>((int)value, (int)v.value);
		}
		return *this;
	}
	template <typename T>
	__forceinline fixed& operator *= (T v)
	{
		value *= v;
		return *this;
	}
	
	template <int rshifts, typename T2>
	fixed operator * (fixed<rshifts,T2> v) const	{ fixed p = *this; p*=v; return p; }
	
	template <int rshifts, typename T2>
	__forceinline fixed& operator /= (fixed<rshifts,T2> v)
	{
		static_assert(sizeof(int) <= sizeof(T2), "forgot the reason");
		value = divShift<rshifts>(value, v.value);
		return *this;
	}
	template <typename T>
	fixed& operator /= (T v)
	{
		value /= v;
		return *this;
	}

	template <int rshifts, typename T2>
	fixed operator / (fixed<rshifts,T2> v) const { fixed p = *this; p/=v; return p; }
	
	template <int rshifts, typename T2> bool operator == (fixed<rshifts,T2> rhs) const
	{
		return value == Converter<(sizeof(T) > sizeof(fixed<rshifts,T2>::value_type)), fixed<rshifts,T2> >::convert(rhs);
	}
	template <int rshifts, typename T2>	bool operator != (fixed<rshifts,T2> rhs) const
	{
		return value != Converter<(sizeof(T) > sizeof(fixed<rshifts,T2>::value_type)), fixed<rshifts,T2> >::convert(rhs);
	}
	
	template <int rshifts, typename T2> bool operator > (fixed<rshifts,T2> v) const
	{
		return value > Converter<(sizeof(T) > sizeof(fixed<rshifts,T2>::value_type)), fixed<rshifts,T2> >::convert(v);
	}
	template <int rshifts, typename T2> bool operator < (fixed<rshifts,T2> v) const
	{
		return value < Converter<(sizeof(T) > sizeof(fixed<rshifts,T2>::value_type)), fixed<rshifts,T2> >::convert(v);
	}
	template <int rshifts, typename T2> bool operator >= (fixed<rshifts,T2> v) const
	{
		return value >= Converter<(sizeof(T) > sizeof(fixed<rshifts,T2>::value_type)), fixed<rshifts,T2> >::convert(v);
	}
	template <int rshifts, typename T2> bool operator <= (fixed<rshifts,T2> v) const
	{
		return value <= Converter<(sizeof(T) > sizeof(fixed<rshifts,T2>::value_type)), fixed<rshifts,T2> >::convert(v);
	}

};

template <typename TargetT, int shifts, typename T>
inline typename TargetT operator += (TargetT& lhs, fixed<shifts, T> rhs)
{
	lhs += TargetT(rhs);
	return lhs;
}

/*
template <int shifts, typename T>
inline void swap(fixed<shifts, T>& lhs, fixed<shifts, T>& rhs)
{
	lhs ^= rhs;
	rhs ^= lhs;
	lhs ^= rhs;
}
*/

//! 絶対値
template<int shifts, typename T>
inline fixed<shifts, T> pow(fixed<shifts, T> v1, fixed<shifts, T> v2)
{
	double val = ::pow(double(v1), double(v2));
	return fixed<shifts, T>(val);
}

//! 絶対値
template<int shifts, typename T>
inline fixed<shifts, T> abs(fixed<shifts, T> p)
{
	p.value = ::abs(p.value);
	return p;
}

template<int shifts>
inline fixed<shifts, __int64> abs(fixed<shifts, __int64> p)
{
#ifdef abs64
	p.value = ::abs64(p.value);
#else
	p.value = (p.value < 0) ? -p.value : p.value;
#endif
	return p;
}

// 少数部取り出し
template<int shifts, typename T>
inline fixed<shifts, T> frac(fixed<shifts, T> p)
{
	p.value &= fixed<shifts, T>::fracFlag;
	return p;
}

// 小数部、四捨五入して整数化
template<int shifts, typename T>
static fixed<shifts, T> halfAdjust(fixed<shifts, T> p)
{
	p.value = (p.value + (1 << (shifts-1))) & fixed<shifts, T>::intFlag;
	return p;
}

template <int shifts, typename T>
inline fixed<shifts, T> floor(fixed<shifts, T> p)
{
	p.value &= fixed<shifts, T>::intFlag;
	return p;
}

// 切捨て整数化
template<int shifts, typename T>
__forceinline int ToInt(fixed<shifts, T> p)
{
	return (p.value & fixed<shifts, T>::intFlag) >> shifts;
}

template<int shifts, typename T>
const fixed<shifts, T> OneMinusEpsilon(fixed<shifts, T> dummy = 0)
{
	fixed<shifts, T> p;
	p.value = (1 << shifts) - 1;
	return p;
}

template<int shifts, typename T>
const fixed<shifts, T> Epsilon(fixed<shifts, T> v)
{
	v.value = 1;
	return v;
}

// expect bit shift optimisation
// do not calc huge number
template <int num, int shifts, typename T>
static fixed<shifts, T> mul(fixed<shifts, T> p)
{
	return (p.value * num) >> shifts;
}

// expect bit shift optimisation
// do not calc huge number
template <int num, int shifts, typename T>
static fixed<shifts, T> div(fixed<shifts, T> p)
{
	return (p.value / num) >> shifts;
}

// expect bit shift optimisation
// do not calc huge number
template <int num, int shifts, typename T>
static fixed<shifts, T> div2(fixed<shifts, T> p)
{
	p.value >>= num;
	return p;
}

template <int shifts, typename T>
__forceinline fixed<shifts, T> min(fixed<shifts, T> a, fixed<shifts, T> b)
{
	a.value = gl::min(a.value, b.value);
	return a;
}

template <int shifts, typename T>
__forceinline fixed<shifts, T> max(fixed<shifts, T> a, fixed<shifts, T> b)
{
	a.value = gl::max(a.value, b.value);
	return a;
}

template <int shifts, typename T>
fixed<shifts, T> sqrt(fixed<shifts, T> val)
{
#if 0
	return 1.0f / InvSqrt((float)val);
#else
	return ::sqrt((double)val);
#endif
}

template <int shifts, typename T>
fixed<shifts, T> ceil(fixed<shifts, T> val)
{
	return ToInt(val) + 1;
}

} // namespace gl
