#pragma once

namespace gl
{

class IBuffer2D
{
public:
	virtual size_t	GetWidth() const = 0;
	virtual size_t	GetHeight() const = 0;
	virtual int		GetLineOffset() const = 0;
	virtual void*	GetPixelVoidPtr(size_t x, size_t y) = 0;
	virtual const void*	GetPixelVoidPtr(size_t x, size_t y) const = 0;
};

}	// namespace gl
