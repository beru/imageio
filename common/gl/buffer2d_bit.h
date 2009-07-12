#pragma once

#include "buffer2D.h"
#include <boost/cstdint.hpp>

namespace gl
{

// �����n��int�̒P�ʂœǂݏ�������悤��
template <>
class Buffer2D<bool> : public IBuffer2D
{
public:
	typedef boost::uint_fast32_t value_type;
private:
	size_t width_;			//!< �`��̈�̉���
	size_t height_;			//!< �`��̈�̏c��
	int lineOffset_;		//!< Y���W��1�グ��̂ɕK�v�ȃo�C�g���B

	value_type* pBuff_;			//!< �`��̈�̐擪���W���w��
	
	bool allocated_;	//!< ����instance��buff_���m�ۂ����̂��ǂ���

	static const size_t VALUE_BITS = sizeof(value_type) * 8;
	static const size_t VALUE_BYTES = sizeof(value_type);
	
	// �w�肵�����W�̉�f���A�擪���牽�o�C�g�ڂɑ��݂��邩
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
	
	//! �����ŕ`��̈���m�ۂ��Ȃ��ŁA�O����n���Ă������W�𗘗p
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
