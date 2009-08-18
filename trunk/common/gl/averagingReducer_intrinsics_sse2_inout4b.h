#pragma once

#include <emmintrin.h>
#include <boost/cstdint.hpp>
using namespace boost;

/*
	面積平均法（平均画素法）による画像縮小処理
	
	SSE2 までの intrinsics を使用して最適化

	入力：32bit、各色要素8bit、4要素纏めた色の処理
	出力：32bit、各色要素8bit、4要素纏めた色の処理
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

