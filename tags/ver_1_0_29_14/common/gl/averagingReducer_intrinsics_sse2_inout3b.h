#pragma once

#include <emmintrin.h>

/*
	面積平均法（平均画素法）による画像縮小処理
	
	SSE2 までの intrinsics を使用して最適化

	入力：24bit、各色要素8bit、3要素纏めた色の処理
	出力：24bit、各色要素8bit、3要素纏めた色の処理
*/

namespace gl
{

namespace intrinsics_sse2_inout3b
{

void AveragingReduce(
	const size_t heightRatioTarget, const size_t heightRatioSource, const size_t srcHeight, 
	const size_t widthRatioTarget, const size_t widthRatioSource, const size_t srcWidth,
	const __m128i* srcBuff, const int srcLineOffsetBytes,
	__m128i* targetBuff, const int targetLineOffsetBytes,
	char* tmpBuff
);

}	// namespace intrinsics_sse2_inout3b

}	// namespace gl

