#include "ImageReader.h"

#include "arrayutil.h"

namespace ImageIO {

bool ImageReader::ReadSourceInformation(const BITMAPINFOHEADER& bmih, const RGBQUAD* bmiColors)
{
	imageInfo_.width = bmih.biWidth;
	imageInfo_.height = bmih.biHeight;
	int lineBytes = ((((bmih.biWidth * bmih.biBitCount) + 31) & ~31) >> 3);
	imageInfo_.lineOffset = (bmih.biHeight < 0) ? lineBytes : -lineBytes;
	imageInfo_.bitsPerSample = 8;
	switch (bmih.biBitCount) {
	case 1:
	case 4:
	case 8:
	case 16:
	case 24:
		imageInfo_.samplesPerPixel = 3;
		imageInfo_.colorFormat = ColorFormat_BGR;
		imageInfo_.bitsPerPixel = 24;
		break;
	case 32:
		imageInfo_.samplesPerPixel = 4;
		imageInfo_.colorFormat = ColorFormat_BGRA;
		imageInfo_.bitsPerPixel = 32;
		break;
	default:
		return false;
	}
	return true;
}

void InterchangePaletteIndexsToValues_1(unsigned char* buff, RGBQUAD* palettes, size_t width)
{
	for (int i=width-1; i>=0; --i) {
		const size_t bytes = i / 8;
		const size_t remain = i % 8;
		unsigned char b = buff[bytes];
		b >>= 7 - remain;
		b &= 1;
		const RGBQUAD col = palettes[b];
		buff[i*3+0] = col.rgbBlue;
		buff[i*3+1] = col.rgbGreen;
		buff[i*3+2] = col.rgbRed;
	}
}

void InterchangePaletteIndexsToValues_4(unsigned char* buff, RGBQUAD* palettes, size_t width)
{
	for (int i=width-1; i>=0; --i) {
		const size_t i2 = i * 4;
		const size_t bytes = i2 / 8;
		const size_t remain = i2 % 8;
		unsigned char b = buff[bytes];
		if (remain) {
			b &= 0x0F;
		}else {
			b >>= 4;
		}
		const RGBQUAD col = palettes[b];
		buff[i*3+0] = col.rgbBlue;
		buff[i*3+1] = col.rgbGreen;
		buff[i*3+2] = col.rgbRed;
	}
}

void InterchangePaletteIndexsToValues_8(unsigned char* buff, RGBQUAD* palettes, size_t width)
{
	for (int i=width-1; i>=0; --i) {
		const size_t idx = buff[i];
		const RGBQUAD col = palettes[idx];
		buff[i*3+0] = col.rgbBlue;
		buff[i*3+1] = col.rgbGreen;
		buff[i*3+2] = col.rgbRed;
	}
}

void InterchangePaletteIndexsToValues(size_t bitCount, unsigned char* buff, RGBQUAD* palettes, size_t width)
{
	switch (bitCount) {
	case 1:
		InterchangePaletteIndexsToValues_1(buff, palettes, width);
		break;
	case 4:
		InterchangePaletteIndexsToValues_4(buff, palettes, width);
		break;
	case 8:
		InterchangePaletteIndexsToValues_8(buff, palettes, width);
		break;
	}
}

bool ImageReader::ReadBMP(IFile& file)
{
	imageFormatType_ = ImageFormatType_BMP;

	DWORD fileSize = file.Size();
	if (fileSize <= 54) {
		lastErrorMessage_ = "file size <= 54 bytes.";
		return false;
	}
	BITMAPFILEHEADER bmpFileHeader;
	DWORD readBytes;
	file.Read(&bmpFileHeader, 14, readBytes);
	assert(readBytes == 14);
	if (bmpFileHeader.bfType != 19778) {
		lastErrorMessage_ = "file hedear bfType != 19778";
		return false;
	}
	
	BITMAPINFOHEADER bmih;
	file.Read(&bmih, 40, readBytes);
	if (readBytes != 40) {
		lastErrorMessage_ = "bmih shortage.";
		return false;
	}
	RGBQUAD bmiColors[256];
	size_t bmiColorsLen = 0;
	switch (bmih.biBitCount) {
	case 1:
		bmiColorsLen = 2;
		break;
	case 4:
		bmiColorsLen = 16;
		break;
	case 8:
		bmiColorsLen = 256;
		break;
	case 16:
		lastErrorMessage_ = "16 bit BMP not supported.";
		return false;
		break;
	case 32:
		if (bmih.biCompression == BI_BITFIELDS) {
			bmiColorsLen = 3;
		}
	}
	if (bmiColorsLen) {
		size_t needBytes = /*sizeof(RGBQUAD)*/4*bmiColorsLen;
		file.Read(bmiColors, needBytes, readBytes);
		if (readBytes != needBytes) {
			lastErrorMessage_ = "bmiColors read failed.";
			return false;
		}
	}
	if (!ReadSourceInformation(bmih, bmiColors)) {
		return false;
	}
	if (!Setup()) {
		return false;
	}
	
	int lineBytes = ((((bmih.biWidth * bmih.biBitCount) + 31) & ~31) >> 3);
	int lineIdx = (bmih.biHeight < 0) ? 0 : (bmih.biHeight-1);
	int lineOffset = (bmih.biHeight < 0) ? 1 : -1;
	size_t height = abs(bmih.biHeight);
	
	for (size_t y=0; y<height; ++y) {
		char* buff = GetLineBuffer(lineIdx);
		if (buff == NULL) {
			return false;
		}
		file.Read(buff, lineBytes, readBytes);
		if (readBytes == 0) {
			lastErrorMessage_ = "read bytes = 0.";
			return false;
		}
		switch (bmih.biBitCount) {
		case 1:
		case 4:
		case 8:
			InterchangePaletteIndexsToValues(bmih.biBitCount, (unsigned char*)buff, bmiColors, bmih.biWidth);
			break;
		case 16:
			// TODO: convert to 24 bits
			break;
		case 24:
		case 32:
			// do nothing
			break;
		}
		if (!ProcessLineBuffer(lineIdx, buff)) {
			return false;
		}
		lineIdx += lineOffset;
	}
	return true;
}

}	// namespace ImageIO
