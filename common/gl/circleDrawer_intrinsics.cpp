#include "CircleDrawer_intrinsics.h"

#include "common.h"

namespace gl
{

void DrawSphere_A8_intrinsics(
	__m128i* buffer, const size_t width, const size_t height, const int lineOffset,
	float cx, float cy, float radius)
{
		// bounds... clipping
		float lx = max(floor(cx - radius), 0);
		float rx = min(ceil(cx + radius), width-1);
		float ly = max(floor(cy - radius), 0);
		float ry = min(ceil(cy + radius), height-1);

		__m128 vec_plus4 = _mm_set_ps1(4.0);
		__m128 vec_plus16 = _mm_set_ps1(16.0);
		__m128 vec_1 = _mm_set_ps1(1.0);
		__m128 vec_256 = _mm_set_ps1(255.0);
		__m128 vec_cx = _mm_set1_ps(cx);
		__m128 vec_radius = _mm_set1_ps(radius);
		__m128 vec_invRadius = _mm_set1_ps(1.0 / radius);
		__m128 vec_invLen = _mm_set1_ps( 1.0 / (radius*radius) );
		
		__m128i* lptr = (__m128i*) ((char*)buffer + int(ly) * lineOffset);
		for (float y=ly; y<ry; ++y) {
			float yLen = y - cy;
			float sqrYLen = sqr(yLen);
			float sqrXLen = max(0, sqr(radius) - sqrYLen);
			float maxXLen = sqrt(sqrXLen);
			// ŠO‘¤‚Ì‹ó”’‚ðskip
			float lx2 = max(lx, cx - maxXLen);
			float rx2 = min(rx, cx + maxXLen);
			size_t ilx2 = (size_t) lx2;
			size_t irx2 = (size_t) rx2;
			__m128 vec_sqrYLen = _mm_set1_ps(sqrYLen);
			__m128 vec_x1 = _mm_set_ps(lx2, lx2+1, lx2+2, lx2+3);
			__m128 vec_x2 = _mm_add_ps(vec_x1, vec_plus4);
			__m128 vec_x3 = _mm_add_ps(vec_x2, vec_plus4);
			__m128 vec_x4 = _mm_add_ps(vec_x3, vec_plus4);
			char* ptr = ((char*)lptr + ilx2);
			for (size_t x=ilx2; x<irx2; x+=16) {
//				float fact = sqrt(sqr(y-cy) + sqr(x-cx)) / radius;

				__m128 tmp1;
				tmp1 = _mm_sub_ps(vec_x1, vec_cx);
				tmp1 = _mm_add_ps(vec_sqrYLen, _mm_mul_ps(tmp1, tmp1));
				tmp1 = _mm_mul_ps(tmp1, vec_invLen);
				tmp1 = _mm_sub_ps(vec_1, tmp1);
				tmp1 = _mm_mul_ps(tmp1, vec_256);
				__m128 tmp2;
				tmp2 = _mm_sub_ps(vec_x2, vec_cx);
				tmp2 = _mm_add_ps(vec_sqrYLen, _mm_mul_ps(tmp2, tmp2));
				tmp2 = _mm_mul_ps(tmp2, vec_invLen);
				tmp2 = _mm_sub_ps(vec_1, tmp2);
				tmp2 = _mm_mul_ps(tmp2, vec_256);
				__m128 tmp3;
				tmp3 = _mm_sub_ps(vec_x3, vec_cx);
				tmp3 = _mm_add_ps(vec_sqrYLen, _mm_mul_ps(tmp3, tmp3));
				tmp3 = _mm_mul_ps(tmp3, vec_invLen);
				tmp3 = _mm_sub_ps(vec_1, tmp3);
				tmp3 = _mm_mul_ps(tmp3, vec_256);
				__m128 tmp4;
				tmp4 = _mm_sub_ps(vec_x4, vec_cx);
				tmp4 = _mm_add_ps(vec_sqrYLen, _mm_mul_ps(tmp4, tmp4));
				tmp4 = _mm_mul_ps(tmp4, vec_invLen);
				tmp4 = _mm_sub_ps(vec_1, tmp4);
				tmp4 = _mm_mul_ps(tmp4, vec_256);
				
				__m128i tmp11 = _mm_cvtps_epi32(tmp1);
				__m128i tmp21 = _mm_cvtps_epi32(tmp2);
				__m128i tmp31 = _mm_cvtps_epi32(tmp3);
				__m128i tmp41 = _mm_cvtps_epi32(tmp4);
				__m128i packed1 = _mm_packs_epi32(tmp11, tmp21);
				__m128i packed2 = _mm_packs_epi32(tmp31, tmp41);
				__m128i packed3 = _mm_packus_epi16(packed1, packed2);
				_mm_storeu_si128((__m128i*)ptr, packed3);
				ptr += 16;
				vec_x1 = _mm_add_ps(vec_x1, vec_plus16);
				vec_x2 = _mm_add_ps(vec_x2, vec_plus16);
				vec_x3 = _mm_add_ps(vec_x3, vec_plus16);
				vec_x4 = _mm_add_ps(vec_x4, vec_plus16);
			}
			lptr = (__m128i*) ((char*)lptr + lineOffset);
		}

	
	
}

} // namespace gl

