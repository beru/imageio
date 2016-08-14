#pragma once

/*
	Graphics Library の共通処理
*/

#include <assert.h>
#include <memory.h>
#include <math.h>
#include <float.h>

#include "fastmath.h"

#include <numeric>

#ifdef DEBUG
# define NODEFAULT   ASSERT(0)
#else
# define NODEFAULT   __assume(0)
#endif

namespace gl
{

template <int v> struct int2type { enum { value = v }; };
template <bool v> struct bool2type { enum { value = v }; };

// i*64bit塗りつぶす
inline void memfill(void *dst, int n32, unsigned long i)
{
#if defined(_M_IX86)
//	if (i % 2)
//		++i;
	__asm {
		movq mm0, n32
		punpckldq mm0, mm0
		mov edi, dst

loopwrite:

		movntq 0[edi], mm0
		movntq 8[edi], mm0
		//movntq 16[edi], mm0
		//movntq 24[edi], mm0
		//movntq 32[edi], mm0
		//movntq 40[edi], mm0
		//movntq 48[edi], mm0
		//movntq 56[edi], mm0

		add edi, 16
		sub i, 2
		jg loopwrite

		emms
	}
#else
#endif
}

inline int RoundValue(float param)
{
#if 0 //def _M_IX86
    // Uses the FloatToInt functionality
	// NVIDIA fastmath.cpp
    int a;
    int *int_pointer = &a;

    __asm  fld  param
    __asm  mov  edx,int_pointer
    __asm  FRNDINT
    __asm  fistp dword ptr [edx];
    return a;
#else
	return static_cast<int>(param + 0.5f);
#endif
}

#ifdef _M_IX86

// At the assembly level the recommended workaround for the second FIST bug is the same for the first; 
// inserting the FRNDINT instruction immediately preceding the FIST instruction. 

__forceinline void FloatToInt(int *int_pointer, float f) 
{
	__asm  fld  f
	__asm  mov  edx,int_pointer
	__asm  FRNDINT
	__asm  fistp dword ptr [edx];
}

#endif

template <typename T>
__forceinline int ToInt(T f) 
{
#if 1
	return f;
#else
	return Real2Int(f);
#endif
}

template <typename T>
inline T floor(T val)
{
	return ::floor(val);
}

template <typename T>
inline T frac(T val)
{
	T intPtr;
	return modf(val, &intPtr);
}

inline int halfdjust(int val)
{
	return val;
}

template <typename T>
inline int halfAdjust(T val)
{
#if 1
	return (int)(val + T(0.5));
#else
	return RoundValue(val);
#endif
}

/*!
	255以上は255に限定
*/
inline unsigned char limit255(unsigned int sum)
{
#if 0
	unsigned int b = sum & 0x100;	// 繰り上がりビット検出
	unsigned int c = b - (b >> 8);	// 繰り上がりビットが0の場合は、c = 0 - (0 >> 8) = 0, 1の場合は、c = 0x100 - (0x100 >> 8) = 0xff;
	return unsigned char(sum | c);
#else
	return (255 < sum) ? 255 : sum;
#endif
}

inline float OneMinusEpsilon(float dummy = 0)
{
	return  1.0f - FLT_EPSILON;
}

inline double OneMinusEpsilon(double dummy = 1.0)
{
	return 1.0 - 0.005;//DBL_EPSILON;
}

inline float Epsilon(float dummy = 0)
{
	return FLT_EPSILON;
}

inline double Epsilon(double dummy = 1.0)
{
	return DBL_EPSILON;
}

// 掛け算を行って右シフトする。固定小数点演算等に利用。
template <size_t shifts>
__forceinline int mulShift(int lhs, int rhs)
{
#ifdef _M_IX86
	// enum hack.. http://d.hatena.ne.jp/paserry/20050304
	enum
	{
		s1 = shifts,
		s2 = sizeof(int)*8 - shifts,
	};
	__asm {
		mov eax, lhs
		mov edx, rhs
		imul edx
		shr eax, s1
		shl edx, s2
		or eax, edx
	}
#else
	return (__int64(lhs) * rhs) >> shifts;
#endif
}

// 掛け算を行って右シフトする。固定小数点演算等に利用。
template <size_t shifts>
__forceinline unsigned int mulShift(unsigned int lhs, unsigned int rhs)
{
#ifdef _M_IX86
	// enum hack.. http://d.hatena.ne.jp/paserry/20050304
	enum
	{
		s1 = shifts,
		s2 = sizeof(int)*8 - shifts,
	};
	__asm {
		mov eax, lhs
		mov edx, rhs
		mul edx
		shr eax, s1
		shl edx, s2
		or eax, edx
	}
#else
	return (lhs * rhs) >> shifts;
#endif
}

// 割り算を行って左シフトする。固定小数点演算等に利用。
template <size_t shifts>
__forceinline int divShift(int lhs, int rhs)
{
#ifdef _M_IX86
	enum
	{
		s1 = shifts,
		s2 = sizeof(int)*8 - shifts,
	};
	__asm {
		mov edx, lhs
		mov eax, edx
		sar edx, s2
		sal eax, s1
		idiv rhs
	}
#else
	return (__int64(lhs) << shifts) / rhs;
#endif
}

template <size_t shifts>
__forceinline unsigned int divShift(unsigned int lhs, unsigned int rhs)
{
#ifdef _M_IX86
	enum
	{
		s1 = shifts,
		s2 = sizeof(int)*8 - shifts,
	};
	__asm {
		mov edx, lhs
		mov eax, edx
		sar edx, s2
		sal eax, s1
		div rhs
	}
#else
	return (__int64(lhs) << shifts) / rhs;
#endif
}

// expect bit shift optimisation
// do not calc huge number
template <int num, typename T>
inline T mul(T p)
{
	return (p * num);
}

// expect bit shift optimisation
// do not calc huge number
template <int num, typename T>
static T div(T p)
{
	return (p / num);
}

template <long X, int Y> 
struct power 
{ 
  static const long value = X * power<X, Y-1>::value; 
}; 

template <long X> 
struct power<X, 0> 
{ 
  static const long value = 1; 
};

template <int num, typename T>
inline T mul2(T p)
{
	return p * T(power<2, num>::value);
}

template <int num, typename T>
inline T div2(T p)
{
	return p / T(power<2, num>::value);
}

// http://www.hayasoft.com/haya/bit-enzan/technic.html

/*
// http://aggregate.org/MAGIC/#Integer%20Minimum%20or%20Maximum
inline unsigned int min(unsigned int x, unsigned int y)
{
	return x+(((y-x)>>(sizeof(unsigned int)*4-1))&(y-x));
}

inline unsigned int max(unsigned int x, unsigned int y)
{
	return x-(((x-y)>>(sizeof(unsigned int)*4-1))&(x-y));
}
*/

template <typename T>
inline T abs(T val)
{
	return ::abs(val);
}

inline unsigned int abs(unsigned int val) { return val; }

inline int abs(int val)
{
#if 0
	int mask = val >> 31;
	return (val ^ mask) - mask;
#else
	return ::abs(val);
#endif
}

// FAST INVERSE SQUARE ROOT, CHRIS LOMONT
inline float InvSqrt(float x)
{
	float xhalf = 0.5f*x;
	int i = *(int*)&x;		// get bits for floating value
	i = 0x5f375a86- (i>>1);	// gives initial guess y0
	x = *(float*)&i;		// convert bits back to float
	x = x*(1.5f-xhalf*x*x);	// Newton step, repeating increases accuracy
	return x;
}

template <typename T>
inline T sqrt(T val)
{
	return ::sqrt(val);
}

template <typename T>
inline T ceil(T val)
{
	return ::ceil(val);
}

template <typename T>
inline T sqr(T val)
{
	return val*val;
}

inline size_t align(size_t num, size_t align)
{
	size_t mod = num % align;
	if (mod != 0) {
		return num + align - mod;
	}
	return num;
}

};	// namespace gl

