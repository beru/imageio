#pragma once

#include <emmintrin.h>
#include <boost/cstdint.hpp>
using namespace boost;

/*
	�ʐϕ��ϖ@�i���ω�f�@�j�ɂ��摜�k������
	
	SSE2 �܂ł� intrinsics ���g�p���čœK��

	���́F32bit�A�e�F�v�f8bit�A4�v�f�Z�߂��F�̏���
	�o�́F32bit�A�e�F�v�f8bit�A4�v�f�Z�߂��F�̏���
*/

namespace gl
{

namespace intrinsics_sse2_inout4b
{

struct AveragingReduceParams
{
	uint16_t heightRatioTarget;
	uint16_t heightRatioSource;
	uint16_t srcHeight;

	uint16_t widthRatioTarget;
	uint16_t widthRatioSource;
	uint16_t srcWidth;
	
	const __m128i* srcBuff;
	int srcLineOffsetBytes;

	__m128i* targetBuff;
	int targetLineOffsetBytes;
	
	__m128i* tmpBuff;
};

class AveragingReducer;

class AveragingReducerCaller
{
public:
	AveragingReducerCaller();
	~AveragingReducerCaller();
	void Setup(const AveragingReduceParams* pParams, uint16_t parts);
	void Process(uint16_t part);
private:
	AveragingReducer* pImpl_;
};

}	// namespace intrinsics_sse2_inout4b

}	// namespace gl

