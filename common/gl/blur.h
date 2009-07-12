#pragma once

#include <numeric>

/*
	移動平均による画像ぼかし処理
*/

namespace gl
{

template <
	size_t nSamples,
	typename NumericT,
	typename WorkColorT,
	typename SrcColorT,
	typename TargetColorT,
	typename SrcToWorkConverterT,
	typename WorkToTargetConverterT
>
void MovingAverageHorizontal(
	const SrcColorT* pSrc,
	TargetColorT* pTarget,
	size_t count,
	SrcToWorkConverterT& srcToWorkConverter,
	WorkToTargetConverterT& workToTargetConverter)
{
	const NumericT divFactor = NumericT(1) / NumericT(nSamples * 2 + 1);
	
	// 端っこのPixelを延長する
	const SrcColorT* pSrcWrk = pSrc;
	TargetColorT* pTargetWrk = pTarget;
	
	// 最初のsampleは左端をsample分追加
	const WorkColorT firstPixel = srcToWorkConverter(*pSrcWrk);
	WorkColorT total = firstPixel * (1+nSamples);
	++pSrcWrk;
	size_t x = 1;
	for (; x<1+nSamples; ++x) {
		total += srcToWorkConverter(*pSrcWrk);
		++pSrcWrk;
	}
	*pTargetWrk = workToTargetConverter(total * divFactor);
	++pTargetWrk;
	
	// 始めのうちのsampleは左端を延長して使うので左端を引く。
	for (; x<1+nSamples*2; ++x) {
		total -= firstPixel;
		total += srcToWorkConverter(*pSrcWrk);
		*pTargetWrk = workToTargetConverter(total * divFactor);
		++pSrcWrk;
		++pTargetWrk;
	}
	
	// 中間は普通に処理する。
	const SrcColorT* pSrcOld = pSrc;
	for (; x<count-nSamples; ++x) {
		total -= srcToWorkConverter(*pSrcOld);
		total += srcToWorkConverter(*pSrcWrk);
		*pTargetWrk = workToTargetConverter(total * divFactor);
		++pSrcOld;
		++pSrcWrk;
		++pTargetWrk;
	}
	
	// 終わりの方は右端を延長して使うので右端を足す。
	const WorkColorT lastPixel = srcToWorkConverter(*(pSrc + count - 1));
	for (; x<count+nSamples; ++x) {
		total -= srcToWorkConverter(*pSrcOld);
		total += lastPixel;
		*pTargetWrk = workToTargetConverter(total * divFactor);
		++pSrcOld;
		++pTargetWrk;
	}
}

template <
	size_t nSamples,
	typename NumericT,
	typename WorkColorT,
	typename SrcColorT,
	typename TargetColorT,
	typename SrcToWorkConverterT,
	typename WorkToTargetConverterT
>
void BlurHorizontal(
	const Buffer2D<SrcColorT>& src,
	Buffer2D<TargetColorT>& target,
	SrcToWorkConverterT& srcToWorkConverter,
	WorkToTargetConverterT& workToTargetConverter)
{
	const size_t sh = src.GetHeight();
	const size_t sw = src.GetWidth();
	const int slo = src.GetLineOffset();
	const SrcColorT* pSrc = src.GetPixelPtr(0,0);
	
	const size_t th = target.GetHeight();
	const size_t tw = target.GetWidth();
	const int tlo = target.GetLineOffset();
	TargetColorT* pTarget = target.GetPixelPtr(0,0);

	assert(nSamples*2+1 < sw);
	assert(sh <= th);
	assert(sw <= tw);
	for (size_t y=0; y<sh; ++y) {
		MovingAverageHorizontal<nSamples, NumericT, WorkColorT>(pSrc, pTarget, sw, srcToWorkConverter, workToTargetConverter);
		OffsetPtr(pSrc, slo);
		OffsetPtr(pTarget, tlo);
	}
}

template <
	size_t nSamples,
	typename NumericT,
	typename WorkColorT,
	typename SrcColorT,
	typename TargetColorT,
	typename SrcToWorkConverterT,
	typename WorkToTargetConverterT
>
void MovingAverageVertical(
	const SrcColorT* pSrc,
	TargetColorT* pTarget,
	size_t count,
	int srcLineOffset,
	int targetLineOffset,
	SrcToWorkConverterT& srcToWorkConverter,
	WorkToTargetConverterT& workToTargetConverter)
{
	const NumericT divFactor = NumericT(1) / NumericT(nSamples * 2 + 1);
	
	// 端っこのPixelを延長する
	const SrcColorT* pSrcWrk = pSrc;
	TargetColorT* pTargetWrk = pTarget;
	
	// 最初のsampleは左端をsample分追加
	const WorkColorT firstPixel = srcToWorkConverter(*pSrcWrk);
	WorkColorT total = firstPixel * (1+nSamples);
	OffsetPtr(pSrcWrk, srcLineOffset);
	size_t x = 1;
	for (; x<1+nSamples; ++x) {
		total += srcToWorkConverter(*pSrcWrk);
		OffsetPtr(pSrcWrk, srcLineOffset);
	}
	*pTargetWrk = workToTargetConverter(total * divFactor);
	OffsetPtr(pTargetWrk, targetLineOffset);
	
	// 始めのうちのsampleは左端を延長して使うので左端を引く。
	for (; x<1+nSamples*2; ++x) {
		total -= firstPixel;
		total += srcToWorkConverter(*pSrcWrk);
		*pTargetWrk = workToTargetConverter(total * divFactor);
		OffsetPtr(pSrcWrk, srcLineOffset);
		OffsetPtr(pTargetWrk, targetLineOffset);
	}
	
	// 中間は普通に処理する。
	const SrcColorT* pSrcOld = pSrc;
	for (; x<count-nSamples; ++x) {
		total -= srcToWorkConverter(*pSrcOld);
		total += srcToWorkConverter(*pSrcWrk);
		*pTargetWrk = workToTargetConverter(total * divFactor);
		OffsetPtr(pSrcOld, srcLineOffset);
		OffsetPtr(pSrcWrk, srcLineOffset);
		OffsetPtr(pTargetWrk, targetLineOffset);
	}
	
	// 終わりの方は右端を延長して使うので右端を足す。
	const SrcColorT* pLast = pSrc;
	OffsetPtr(pLast, srcLineOffset * (count - 1));
	const WorkColorT lastPixel = srcToWorkConverter(*pLast);
	for (; x<count+nSamples; ++x) {
		total -= srcToWorkConverter(*pSrcOld);
		total += lastPixel;
		*pTargetWrk = workToTargetConverter(total * divFactor);
		OffsetPtr(pSrcOld, srcLineOffset);
		OffsetPtr(pTargetWrk, targetLineOffset);
	}
}

template <
	size_t nSamples,
	typename NumericT,
	typename WorkColorT,
	typename SrcColorT,
	typename TargetColorT,
	typename SrcToWorkConverterT,
	typename WorkToTargetConverterT
>
void BlurVertical(
	const Buffer2D<SrcColorT>& src,
	Buffer2D<TargetColorT>& target,
	SrcToWorkConverterT& srcToWorkConverter,
	WorkToTargetConverterT& workToTargetConverter)
{
	const size_t sh = src.GetHeight();
	const size_t sw = src.GetWidth();
	const int slo = src.GetLineOffset();
	const SrcColorT* pSrc = src.GetPixelPtr(0,0);
	
	const size_t th = target.GetHeight();
	const size_t tw = target.GetWidth();
	const int tlo = target.GetLineOffset();
	TargetColorT* pTarget = target.GetPixelPtr(0,0);

	assert(nSamples*2+1 < sh);
	assert(sh <= th);
	assert(sw <= tw);
	for (size_t x=0; x<sw; ++x) {
		MovingAverageVertical<nSamples, NumericT, WorkColorT>(pSrc, pTarget, sh, slo, tlo, srcToWorkConverter, workToTargetConverter);
		++pSrc;
		++pTarget;
	}
}

}	// namespace gl
