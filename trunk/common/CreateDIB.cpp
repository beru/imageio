#include "stdafx.h"
#include "CreateDIB.h"

HBITMAP CreateDIB(int width, int height, size_t bitCount, int& lineOffset, BITMAPINFO& bmi, void*& pBits)
{
	BITMAPINFOHEADER& header = bmi.bmiHeader;
	header.biSize = sizeof(BITMAPINFOHEADER);
	header.biWidth = width;
	header.biHeight = height;
	header.biPlanes = 1;
	header.biBitCount = bitCount;
	header.biCompression = BI_RGB;
	int lineBytes = ((((header.biWidth * header.biBitCount) + 31) & ~31) / 8);
	lineOffset = (height > 0) ? -lineBytes : lineBytes;
	header.biSizeImage = lineBytes * abs(height);
	header.biXPelsPerMeter = 0;
	header.biYPelsPerMeter = 0;
	header.biClrUsed = 0;
	header.biClrImportant = 0;

	return ::CreateDIBSection(
		(HDC)0,
		&bmi,
		DIB_RGB_COLORS,
		&pBits,
		NULL,
		0
	);
}

