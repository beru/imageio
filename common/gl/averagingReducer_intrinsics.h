#pragma once

//#include <tmmintrin.h>
#include <emmintrin.h>
//#include <intrin.h>

namespace gl
{

//#define SSE3

__forceinline __m128i load_unaligned_128(const __m128i* pSrc)
{
#ifdef SSE3
	return _mm_lddqu_si128(pSrc);
#else
	return _mm_loadu_si128(pSrc);
#endif
}

__forceinline __m128 _mm_load_ps(const __m128* p)
{
	return ::_mm_load_ps((float*)p);
}

__forceinline __m128 _mm_loadu_ps(const __m128* p)
{
	return ::_mm_loadu_ps((float*)p);
}

__forceinline void _mm_store_ps(__m128* p, __m128 a)
{
	::_mm_store_ps((float*)p, a);
}

__forceinline void _mm_storeu_ps(__m128* p, __m128 a)
{
	::_mm_storeu_ps((float*)p, a);
}

__forceinline __m128i _mm_set1_epi16_ex(unsigned short v)
{
#if 1
	return _mm_set1_epi16(v);
#else
	__m64 m64 = _mm_shuffle_pi16(_mm_cvtsi32_si64(v), 0);
	return _mm_set1_epi64(m64);
#endif
}

__forceinline void multiply8_16(__m128i a, __m128 b, __m128& c, __m128& d)
{
	__m128i t1 = _mm_unpacklo_epi16(a, _mm_setzero_si128());
	__m128i t2 = _mm_unpackhi_epi16(a, _mm_setzero_si128());
	c = _mm_cvtepi32_ps(t1);
	d = _mm_cvtepi32_ps(t2);
	c = _mm_mul_ps(c, b);
	d = _mm_mul_ps(d, b);
}

__forceinline void multiply8_16(__m128i a, __m128i b, __m128i& c, __m128i& d)
{
	__m128i resultLo = _mm_mullo_epi16(a, b);
	__m128i resultHi = _mm_mulhi_epu16(a, b);
	// interleave
	c = _mm_unpacklo_epi16(resultLo, resultHi);
	d = _mm_unpackhi_epi16(resultLo, resultHi);
}

/*
inline __m128i multiply8_16(__m128i a, __m128i b)
{
	__m128i c, d;
	multiply8_16(a, b, c, d);
	return _mm_packs_epi32(c, d);
}
*/


__forceinline __m128 shorts_to_floats(__m128i shorts)
{
	return _mm_cvtepi32_ps(_mm_unpacklo_epi16(shorts, _mm_setzero_si128()));
}

__forceinline __m128i shorts_to_ints(__m128i shorts)
{
	return _mm_unpacklo_epi16(shorts, _mm_setzero_si128());
}

__forceinline __m128 truncate_fractions(__m128 floats)
{
	return _mm_cvtepi32_ps(_mm_cvttps_epi32(floats));
}

template <typename T>
__forceinline __m128i load_32(T* p)
{
	return *(__m128i*)(&_mm_load_ss((const float*)(p)));
}

__forceinline __m128i set1_epi16_low(int v)
{
	return _mm_shufflelo_epi16(_mm_cvtsi32_si128(v), 0);
}

__forceinline __m128i set1f_epi16_low(float v)
{
	__m128 floats = _mm_load1_ps(&v);
	__m128i ints = _mm_cvtps_epi32(floats);
	__m128i shorts = _mm_unpacklo_epi16(ints, ints);
	return shorts;
}

template <int sel>
__forceinline void prefetch(const void* p)
{
	_mm_prefetch((const char*)p, sel);
}

}	// namespace gl

