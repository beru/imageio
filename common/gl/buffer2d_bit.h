#pragma once

#include "buffer2D.h"
#include <boost/cstdint.hpp>

namespace gl
{

// 処理系のintの単位で読み書きするように
template <>
class Buffer2D<bool> : public IBuffer2D
{
public:
	typedef boost::uint_fast32_t value_type;
private:
	size_t width_;			//!< 描画領域の横幅
	size_t height_;			//!< 描画領域の縦幅
	int lineOffset_;		//!< Y座標を1つ上げるのに必要なバイト数。

	value_type* pBuff_;			//!< 描画領域の先頭座標を指す
	
	bool allocated_;	//!< このinstanceがbuff_を確保したのかどうか

	static const size_t VALUE_BITS = sizeof(value_type) * 8;
	static const size_t VALUE_BYTES = sizeof(value_type);
	
	// 指定した座標の画素が、先頭から何バイト目に存在するか
	__forceinline size_t buffPos(size_t x, size_t y) const
	{
		assert(x+1 <= width_);
		assert(y+1 <= height_);
		assert((x % VALUE_BITS) == 0);
		return lineOffset_*y + x / VALUE_BITS;
	}
	
public:
	Buffer2D()
	{
		width_		= 0;
		height_		= 0;
		lineOffset_	= 0;
		pBuff_		= NULL;
		allocated_	= false;
	}
	
	Buffer2D(size_t width, size_t height)
	{
		width_		= width;
		height_		= height;
		lineOffset_	= (width / VALUE_BITS) * VALUE_BYTES;
		if (width % VALUE_BITS)
			lineOffset_ += VALUE_BYTES;
		pBuff_		= new value_type[lineOffset_ * height];
		allocated_	= true;
	}
	
	//! 内部で描画領域を確保しないで、外から渡ってきた座標を利用
	Buffer2D(size_t width, int height, int lineOffset, void* pBuff)
	{
		width_		= width;
		height_		= height;
		lineOffset_	= lineOffset;
		pBuff_		= (value_type*) pBuff;
		allocated_	= false;
	}
	
	~Buffer2D()
	{
		if (allocated_ && pBuff_) {
			delete pBuff_;
			pBuff_ = NULL;
		}
	}

	size_t	GetWidth() const								{	return width_;						}
	size_t	GetHeight() const								{	return height_;						}
	int		GetLineOffset() const 							{	return lineOffset_;					}
	
	__forceinline value_type*	GetPixelPtr(size_t x, size_t y)
	{
		return (value_type*)(pBuff_ + buffPos(x,y));
	}
	
	__forceinline const value_type* GetPixelPtr(size_t x, size_t y) const
	{
		return (value_type*)((const char*)pBuff_ + buffPos(x,y));
	}

	void*	GetPixelVoidPtr(size_t x, size_t y) { return (void*) GetPixelPtr(x, y); }
	const void*	GetPixelVoidPtr(size_t x, size_t y) const  { return (const void*) GetPixelPtr(x, y); }
};

}	// namespace gl
