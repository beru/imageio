#pragma once

#include <boost/cstdint.hpp>
using namespace boost;

template <typename T>
bool InArray(const T& val, const T arr[])
{
	for (size_t i=0; i<ARRAY_SIZE(arr); ++i)
		if (*it == val)
			return true;
	return false;
}

template <typename T>
inline size_t Count(const T& arr)
{
	const size_t arrSize = sizeof(arr);
	const size_t elemSize = sizeof(arr[0]);
	return arrSize / elemSize;
}
// #define  ARRAY_SIZE(arr)  (sizeof(arr)/sizeof(arr[0]))


template <typename T>
__forceinline void OffsetPtr(T*& ptr, int offsetBytes)
{
	ptr = (T*) ((const char*)ptr + offsetBytes);
}

#include <assert.h>

/*!
	移動平均法で、配列の値を滑らかにする。ある１点の要素とその前後の要素の値を平均化する。
	@param srcArr				[in] 材料とする配列。要素数arrSize。
	@param resultArr			[out] 滑らかにした結果を書き込む配列。要素数arrSize。
	@param arrSize				[in] 配列の要素数
	@param smoothSampleBack		[in] 前の要素からサンプルする数
	@param smoothSampleFront	[in] 後の要素からサンプルする数
*/
template <typename T>
void MovingAverage
(
	const T* srcArr,
	T* resultArr,
	size_t arrSize,
	size_t smoothSampleBack,
	size_t smoothSampleFront)
{
    // 前後のsample数が存在する配列の要素数を超えていた場合、端の値を延長した配列を用意して計算に利用する。
	// その為、前端、本部分、後端、と処理を分ける。

	size_t sampleCount;
	T* tmpArr;

	/// 前端処理
	// src準備
	sampleCount = smoothSampleBack * 2 + smoothSampleFront;
	assert(sampleCount < arrSize);
	tmpArr = new T[sampleCount];
	std::fill(	// 前半埋め
		&tmpArr[0],
		&tmpArr[smoothSampleBack],
		srcArr[0]
	);
	std::copy(	// 後半copy
		&srcArr[smoothSampleBack],
		&srcArr[sampleCount],
		&tmpArr[smoothSampleBack]
	);
	MovingAverage_Routine(
		&tmpArr[0],
		&resultArr[0],
		smoothSampleBack,
		smoothSampleBack,
		smoothSampleFront
	);
	delete tmpArr;
 
	/// 本部分処理
	MovingAverage_Routine(
		&srcArr[0],
		&resultArr[smoothSampleBack],
		arrSize-smoothSampleFront-smoothSampleBack,
		smoothSampleBack,
		smoothSampleFront
	);

	/// 後端処理
	// src準備
	sampleCount = smoothSampleBack + smoothSampleFront * 2;
	assert(sampleCount < arrSize);
	tmpArr = new T[sampleCount];
	std::copy(	// 前半copy
		&srcArr[arrSize-smoothSampleFront],
		&srcArr[arrSize],
		&tmpArr[0]
	);
	std::fill(	// 後半埋め
		&tmpArr[smoothSampleFront],
		&tmpArr[sampleCount],
		srcArr[arrSize-1]
	);
	MovingAverage_Routine(
		&tmpArr[0],
		&resultArr[arrSize-smoothSampleFront],
		smoothSampleFront,
		smoothSampleBack,
		smoothSampleFront
	);
	delete tmpArr;
}

/*!
	移動平均法で、配列の値を滑らかにするroutine。ある１点の要素とその前後の要素の値を平均化する。
	@param srcArr				[in] 材料とする配列。要素数arrSize + smoothSampleBack + smoothSampleFront
	@param resultArr			[out] 滑らかにした結果を書き込む配列。要素数arrSize。
	@param arrSize				[in] 配列の要素数
	@param smoothSampleBack		[in] 前の要素からサンプルする数
	@param smoothSampleFront	[in] 後の要素からサンプルする数
*/
template <typename T>
static inline
void MovingAverage_Routine
(
	const T* srcArr,
	T* resultArr,
	size_t arrSize,
	size_t smoothSampleBack,
	size_t smoothSampleFront)
{
	size_t sampleCount = smoothSampleBack + 1 + smoothSampleFront;
	// 本部分処理
	T sum;
	for (size_t i=0; i<arrSize; ++i)
	{
		sum = 0;
		for (size_t idx=i; idx<i+sampleCount; ++idx)
		{
			sum += srcArr[idx];
		}
		resultArr[i] = sum / sampleCount;
	}
}

template <typename T, typename TARRAY>
bool InArray(const T& val, const TARRAY& arr)
{
	for (size_t i=0; i<ARRAY_SIZE(arr); ++i)
		if (arr[i] == val)
			return true;
	return false;
}

inline unsigned char ExtractByteFromBits(char* buff, size_t distanceBits)
{
	size_t distanceBytes = distanceBits / 8;
	size_t remainBits = distanceBits % 8;
	if (remainBits == 0) {
		return *(unsigned char*) (buff + distanceBytes);
	}else {
		unsigned char* v1 = (unsigned char*) (buff + distanceBytes);
		unsigned char* v2 = v1 + 1;
		return (*v1 << remainBits) | (*v2 >> (8 - remainBits));
	}
}

template <typename T>
T ExtractFromBits(char* buff, size_t distanceBits)
{
	size_t distanceBytes = distanceBits / 8;
	size_t remainBits = distanceBits % 8;
	if (remainBits == 0) {
		return *(T*) (buff + distanceBytes);
	}else {
		T* pv1 = (T*) (buff + distanceBytes);
		T v1 = *pv1;
		v1 = ConvertEndian(v1);
		v1 = (v1 << remainBits);
		v1 = ConvertEndian(v1);

		T* pv2 = pv1 + 1;
		T v2 = *pv2;
		v2 = ConvertEndian(v2);
		v2 = (v2 >> (sizeof(T)*8 - remainBits));
		v2 = ConvertEndian(v2);

		return v1 | v2;
	}
}

