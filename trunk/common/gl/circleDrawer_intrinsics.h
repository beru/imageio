#pragma once

#include <emmintrin.h>

namespace gl
{

void DrawSphere_A8_intrinsics(
	__m128i* buffer, const size_t width, const size_t height, const int lineOffset,
	float cx, float cy, float radius
);

} // namespace gl

