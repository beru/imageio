
#define NOMINMAX
#include "windows.h"
#include "buffer2d.h"

namespace gl
{

IBuffer2D* BuildBuffer2DFromBMP(const BITMAPINFOHEADER& header, void* pBits)
{
	int lineBytes = 0;
	int lineOffset = 0;
	char* pFirstLine = NULL;
	if (header.biBitCount == 32) {
		lineBytes = header.biWidth * 4;
		if (header.biHeight > 0) {
			lineOffset = -lineBytes;
			pFirstLine = (char*)(pBits) + lineBytes * (header.biHeight - 1);
		}else {
			lineOffset = lineBytes;
			pFirstLine = (char*)pBits;
		}
		return new Buffer2D<WinColor4>(header.biWidth, abs(header.biHeight), lineOffset, pFirstLine);
	}else if (header.biBitCount == 24) {
		lineBytes = (3*header.biWidth+3)&~3;
		if (header.biHeight > 0) {
			lineOffset = -lineBytes;
			pFirstLine = (char*)(pBits) + lineBytes * (header.biHeight - 1);
		}else {
			lineOffset = lineBytes;
			pFirstLine = (char*)pBits;
		}
		return new Buffer2D<WinColor3>(header.biWidth, abs(header.biHeight), lineOffset, pFirstLine);
	}else {
		assert(false);
		return NULL;
	}

}

}	// namespace gl
