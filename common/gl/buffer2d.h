#pragma once

#include <assert.h>
#include "arrayutil.h"
#include <malloc.h>

#include "IBuffer2D.h"

namespace gl
{

template <typename ColorT>
class Buffer2D : public IBuffer2D
{
public:
	typedef ColorT value_type;
private:
	size_t width_;			//!< �`��̈�̉���
	size_t height_;			//!< �`��̈�̏c��
	int lineOffset_;		//!< Y���W��1�グ��̂ɕK�v�ȃo�C�g���B
	unsigned char* pBuff_;	//!< �`��̈�Ƃ��Ċm�ۂ����̈�̃A�h���X��ێ�����|�C���^�A���W�i0, 0)���w���B
	
	bool allocated_;	//!< ����instance��buff_���m�ۂ����̂��ǂ���
	
	// �w�肵�����W�̉�f���A�擪���牽�o�C�g�ڂɑ��݂��邩
	__forceinline size_t buffPos(size_t x, size_t y) const
	{
		assert(x+1 <= width_);
		assert(y+1 <= height_);
		return lineOffset_*y + sizeof(ColorT)*x;
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

#if 1
		lineOffset_	= width * sizeof(ColorT);
		size_t surplus = lineOffset_ % 16;
		if (0 < surplus) {
			lineOffset_ += (16 - surplus);
		}
		
		pBuff_		= (unsigned char*) _aligned_malloc(lineOffset_ * height, 16);
#else
		lineOffset_	= width * sizeof(ColorT);
		pBuff_		= new unsigned char[lineOffset_ * height];
#endif
		allocated_	= true;
	}
	
	//! �����ŕ`��̈���m�ۂ��Ȃ��ŁA�O����n���Ă������W�𗘗p
	Buffer2D(size_t width, size_t height, int lineOffset, void* pBuff)
	{
		width_		= width;
		height_		= height;
		lineOffset_	= lineOffset;

		assert(abs(lineOffset_) >= width * sizeof(ColorT));
		pBuff_		= (unsigned char*) pBuff;
		allocated_	= false;
	}
	
	~Buffer2D()
	{
		if (allocated_ && pBuff_) {
#if 1
			_aligned_free(pBuff_);
#else
			delete pBuff_;
#endif
			pBuff_ = 0;
		}
	}

	size_t	GetWidth() const								{	return width_;						}
	size_t	GetHeight() const								{	return height_;						}
	int		GetLineOffset() const 							{	return lineOffset_;					}
	
	void	SetPixel(size_t x, size_t y, ColorT col)		{	*(ColorT*)(pBuff_ + buffPos(x,y)) = col;	}
	ColorT	GetPixel(size_t x, size_t y) const				{	return *(ColorT*)(pBuff_ + buffPos(x,y));	}
	__forceinline ColorT*	GetPixelPtr(size_t x, size_t y)				{	return (ColorT*)(pBuff_ + buffPos(x,y));	}
	__forceinline const ColorT* GetPixelPtr(size_t x, size_t y) const	{	return (ColorT*)(pBuff_ + buffPos(x,y));	}
	
	// implements IBuffer2D
	void*	GetPixelVoidPtr(size_t x, size_t y) { return (void*) GetPixelPtr(x, y); }
	const void*	GetPixelVoidPtr(size_t x, size_t y) const  { return (const void*) GetPixelPtr(x, y); }

};

template <typename ColorT>
struct SetRect_Modifier
{
	SetRect_Modifier(ColorT col)
		:
		color_(col)
	{
	}

	__forceinline void operator () (size_t x, size_t y, size_t width, size_t height, ColorT* pLine, int lineOffset)
	{
		int n32 = 0;
		size_t sz = sizeof(ColorT);
		size_t iterationCnt = width;
		switch (sz) {
#if 0
		case 1:
			n32 = *(char*)(&color_);
			n32 |= (n32 << 24) | (n32 << 16) | (n32 << 8);
			iterationCnt /= 4*2;
			break;
		case 2:
			n32 = *(short*)(&color_);
			n32 |= (n32 << 16);
			iterationCnt /= 2*2;
			break;
		case 4:
			n32 = *(int*)(&color_);
			iterationCnt /= 2;
			break;
#endif
		default:
			{
				for (size_t iy=0; iy<height; ++iy) {
					ColorT* pTarget = pLine;
					std::fill(pLine, pLine+width, color_);
					OffsetPtr(pLine, lineOffset);
				}
				return;
			}
		}
		for (size_t iy=0; iy<height; ++iy) {
			memfill(pLine, n32, iterationCnt);
			OffsetPtr(pLine, lineOffset);
		}
	}

private:
	ColorT color_;
};

template <typename ColorT>
__forceinline void Buffer2D_Fill(Buffer2D<ColorT>& buff, ColorT col)
{
	Buffer2D_ModifyRect(buff, SetRect_Modifier<ColorT>(col));
}

template <typename ColorT>
void Buffer2D_Fill(Buffer2D<ColorT>& buff, size_t x, size_t y, size_t w, size_t h, ColorT col)
{
	if (w + h == 0)
		return;
	int bw = buff.GetWidth();
	int bh = buff.GetHeight();
	int nx = min<int>(x,bw-1);
	int ny = min<int>(y,bh-1);
	int nw = w;
	int nh = h;
	if (bw <= abs(nx) + nw)
		nw = bw - abs(nx) - 1;
	if (bh <= abs(ny) + nh)
		nh = bh - abs(ny) - 1;
	nx = max(0, nx);
	ny = max(0, ny);
	nw = max(0, nw);
	nh = max(0, nh);
	Buffer2D_ModifyRect(buff, nx, ny, nw, nh, SetRect_Modifier<ColorT>(col));
}

template <typename ColorT, typename ModifierT>
__forceinline void Buffer2D_ModifyPixels(Buffer2D<ColorT>& buff, ModifierT& modifier)
{
	Buffer2D_ModifyPixels(buff, 0, 0, buff.GetWidth(), buff.GetHeight(), modifier);
}

template <typename ColorT, typename ModifierT>
void Buffer2D_ModifyPixels(Buffer2D<ColorT>& buff, size_t x, size_t y, size_t width, size_t height, ModifierT& modifier)
{
	ColorT* pLine = buff.GetPixelPtr(x, y);
	int lineOffset = buff.GetLineOffset();
	for (size_t iy=0; iy<height; ++iy) {
		ColorT* pTarget = pLine;
		for (size_t ix=0; ix<width; ++ix) {
			modifier(pTarget, ix, iy);
			++pTarget;
		}
		OffsetPtr(pLine, lineOffset);
	}
}

template <typename ColorT, typename ModifierT>
__forceinline void Buffer2D_ModifyLines(Buffer2D<ColorT>& buff, ModifierT& modifier)
{
	Buffer2D_ModifyLines(buff, 0, 0, buff.GetWidth(), buff.GetHeight(), modifier);
}

template <typename ColorT, typename ModifierT>
void Buffer2D_ModifyLines(Buffer2D<ColorT>& buff, size_t x, size_t y, size_t width, size_t height, ModifierT& modifier)
{
	ColorT* pLine = buff.GetPixelPtr(x, y);
	int lineOffset = buff.GetLineOffset();
	for (size_t iy=0; iy<height; ++iy) {
		ColorT* pTarget = pLine;
		modifier(pTarget, x, iy);
		OffsetPtr(pLine, lineOffset);
	}
}

template <typename ColorT, typename ModifierT>
__forceinline void Buffer2D_ModifyRect(Buffer2D<ColorT>& buff, ModifierT& modifier)
{
	Buffer2D_ModifyRect(buff, 0, 0, buff.GetWidth(), buff.GetHeight(), modifier);
}

template <typename ColorT, typename ModifierT>
void Buffer2D_ModifyRect(Buffer2D<ColorT>& buff, size_t x, size_t y, size_t width, size_t height, ModifierT& modifier)
{
	ColorT* pLine = buff.GetPixelPtr(x, y);
	int lineOffset = buff.GetLineOffset();
	modifier(x,y,width,height, pLine, lineOffset);
}

}	// namespace gl
