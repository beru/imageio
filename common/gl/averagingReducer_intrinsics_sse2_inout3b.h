#pragma once

#include <emmintrin.h>

/*
	�ʐϕ��ϖ@�i���ω�f�@�j�ɂ��摜�k������
	
	SSE2 �܂ł� intrinsics ���g�p���čœK��

	���́F24bit�A�e�F�v�f8bit�A3�v�f�Z�߂��F�̏���
	�o�́F24bit�A�e�F�v�f8bit�A3�v�f�Z�߂��F�̏���
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

