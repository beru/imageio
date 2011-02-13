#include "stdafx.h"

#include "TiffReader.h"
#include "ImageReader.h"

#include "arrayutil.h"

#include <vector>

__forceinline char SwapByteBits(char c)
{
	c = (c >> 4) | (c << 4);
	c = ((c & 0xcc) >> 2 ) | ((c & 0x33) << 2);
	c = ((c & 0xaa) >> 1 ) | ((c & 0x55) << 1);
}

void InvertSamples(char* buff, size_t bps, size_t spp, size_t width)
{
	size_t nSamples = width * spp;
	if (bps <= 8) {
		for (size_t i=0; i<nSamples; ++i) {
			buff[i] = uint8_t(~0) - buff[i];
		}
	}else if (bps <= 16) {
		uint16_t* buff2 = (uint16_t*) buff;
		for (size_t i=0; i<nSamples; ++i) {
			buff2[i] = uint16_t(~0) - buff2[i];
		}
	}else if (bps <= 32) {
		uint32_t* buff2 = (uint32_t*) buff;
		for (size_t i=0; i<nSamples; ++i) {
			buff2[i] = uint32_t(~0) - buff2[i];
		}
	}
}

// TODO: x64対応
unsigned int ConvertEndian(unsigned int val)
{
#ifdef _M_IX86
    __asm {
      mov eax, val
      bswap eax
      mov val, eax
    }
#else
#endif
    return val;
}

unsigned long ConvertEndian(unsigned long val)
{
#ifdef _M_IX86
    __asm {
      mov eax, val
      bswap eax
      mov val, eax
    }
#else
#endif
    return val;
}

unsigned short ConvertEndian(unsigned short val)
{
#ifdef _M_IX86
    __asm {
      mov ax, val
      rol ax, 8
      mov val, ax
    }
#else
#endif
    return val;
}

void DoNothing(char* buff, size_t bps, size_t totalSamples)
{
}

void BitsToUInt8(char* buff, size_t bps, size_t totalSamples)
{
	const uint8_t mask = (~0) << (sizeof(uint8_t)*8-bps);
	uint8_t* targetBuff = (uint8_t*) buff;
	for (int i=totalSamples-1; i>=0; --i) {
		targetBuff[i] = ExtractByteFromBits(buff, bps*i) & mask;
	}
}

void BitsToUInt16(char* buff, size_t bps, size_t totalSamples)
{
	uint16_t mask = (~0) << (sizeof(uint16_t)*8-bps);
	uint16_t* targetBuff = (uint16_t*) buff;
	for (int i=totalSamples-1; i>=0; --i) {
		uint16_t value = ExtractFromBits<uint16_t>(buff, bps*i);
		value = ConvertEndian(value);
		targetBuff[i] = value & mask;
	}
}

void TripleBytesToUInt32(char* buff, size_t bps, size_t totalSamples)
{
	const uint32_t mask = 0xFFFFFF00;
	uint32_t* targetBuff = (uint32_t*) buff;
	for (int i=totalSamples-1; i>=0; --i) {
		targetBuff[i] = (*(uint32_t*)(buff+3*i-1)) & mask;
	}
}

void BitsToUInt32(char* buff, size_t bps, size_t totalSamples)
{
	const size_t shifts = sizeof(uint32_t) * 8 - bps;
	const uint32_t mask = (~0) << shifts;
	uint32_t* targetBuff = (uint32_t*) buff;
	for (int i=totalSamples-1; i>=0; --i) {
		targetBuff[i] = ExtractFromBits<uint32_t>(buff, bps*i) & mask;
	}
}

struct PaletteConverter
{
	void Convert(char* buff, size_t bps, size_t totalSamples)
	{
		// TODO: 9～15, 17～bit以上のパレット画像に対応
		uint32_t mask = 0;
		if (bps <= 8) {
			uint8_t m = (~0);
			m <<= (8-bps);
			mask = m;
		}else if (bps <= 16) {
			mask = uint16_t(~0) << (16-bps);
		}else {
			assert(false);
		}
		uint16_t* targetBuff = (uint16_t*) buff;
		for (int i=totalSamples-1; i>=0; --i) {
			uint32_t idx = 0;
			if (bps <= 8) {
				uint8_t val = ExtractByteFromBits(buff, bps*i);
				idx = val & mask;
				idx >>= (8-bps);
			}else {
				idx = ExtractFromBits<uint16_t>(buff, bps*i);
//				idx = ConvertEndian(idx);
				idx &= mask;
			}
			targetBuff[i*3+0] = rPalette[idx];
			targetBuff[i*3+1] = gPalette[idx];
			targetBuff[i*3+2] = bPalette[idx];
		}
	}
	uint16* rPalette;
	uint16* gPalette;
	uint16* bPalette;
};

void CollectSamples_Under8(char* targetBuff, char* sourceBuff, size_t spp, size_t bps, size_t scanLineSize, size_t width)
{
	const uint8_t mask = (~0) << (sizeof(uint8_t)*8-bps);
	for (size_t i=0; i<width; ++i) {
		for (size_t j=0; j<spp; ++j) {
			targetBuff[spp*i+j] = ExtractByteFromBits(sourceBuff, j*scanLineSize*8 + i*bps) & mask;
		}
	}
}

void CollectSamples_8(char* targetBuff, char* sourceBuff, size_t spp, size_t bps, size_t scanLineSize, size_t width)
{
	for (size_t i=0; i<width; ++i) {
		for (size_t j=0; j<spp; ++j) {
			targetBuff[spp*i+j] = sourceBuff[j*scanLineSize+i];
		}
	}
}

void CollectSamples_Under16(char* targetBuff, char* sourceBuff, size_t spp, size_t bps, size_t scanLineSize, size_t width)
{
	uint16_t mask = (~0) << (sizeof(uint16_t)*8-bps);
	uint16_t* pTarget = (uint16_t*) targetBuff;
	for (size_t i=0; i<width; ++i) {
		for (size_t j=0; j<spp; ++j) {
			uint16_t value = ExtractFromBits<uint16_t>(sourceBuff, j*scanLineSize*8 + i*bps);
			value = ConvertEndian(value);
			pTarget[spp*i+j] = value & mask;
		}
	}
}

void CollectSamples_16(char* targetBuff, char* sourceBuff, size_t spp, size_t bps, size_t scanLineSize, size_t width)
{
	uint16_t* pTarget = (uint16_t*) targetBuff;
	for (size_t i=0; i<width; ++i) {
		for (size_t j=0; j<spp; ++j) {
			pTarget[spp*i+j] = *(uint16_t*)(sourceBuff + j*scanLineSize + i*2);
		}
	}
}

void CollectSamples_24(char* targetBuff, char* sourceBuff, size_t spp, size_t bps, size_t scanLineSize, size_t width)
{
	const uint32_t mask = 0xFFFFFF00;
	uint32_t* pTarget = (uint32_t*) targetBuff;
	for (int i=width-1; i>=0; --i) {
		for (size_t j=0; j<spp; ++j) {
			pTarget[spp*i+j] = (*(uint32_t*)(sourceBuff + j*scanLineSize + i*3-1)) & mask;
		}
	}
}

void CollectSamples_32(char* targetBuff, char* sourceBuff, size_t spp, size_t bps, size_t scanLineSize, size_t width)
{
	uint32_t* pTarget = (uint32_t*) targetBuff;
	for (size_t i=0; i<width; ++i) {
		for (size_t j=0; j<spp; ++j) {
			pTarget[spp*i+j] = *(uint32_t*)(sourceBuff + j*scanLineSize + i*4);
		}
	}
}

void CollectSamples_64(char* targetBuff, char* sourceBuff, size_t spp, size_t bps, size_t scanLineSize, size_t width)
{
	double* pTarget = (double*) targetBuff;
	for (size_t i=0; i<width; ++i) {
		for (size_t j=0; j<spp; ++j) {
			double* pSrc = (double*)(sourceBuff + j*scanLineSize + i*8);
			pTarget[spp*i+j] = *pSrc;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ImageIO {

bool TIFFReader::ReadTiled()
{
	uint32 tileWidth = 0;
	uint32 tileLength = 0;
	uint32 tileDepth = 0;

	if (TIFFGetField(ptiff, TIFFTAG_TILEWIDTH, &tileWidth) == 0) {
		pManager->lastErrorMessage_ = "failed to read tile width.";
		return false;
	}
	if (TIFFGetField(ptiff, TIFFTAG_TILELENGTH, &tileLength) == 0) {
		pManager->lastErrorMessage_ = "failed to read tile width.";
		return false;
	}
	TIFFGetField(ptiff, TIFFTAG_TILEDEPTH, &tileDepth);
	
	if (tileWidth % 16) {
		pManager->lastErrorMessage_ = "TileWidth must be a multiple of 16.";
		return false;
	}
	if (tileLength % 16) {
		pManager->lastErrorMessage_ = "TileLength must be a multiple of 16.";
		return false;
	}
	
	const uint32 tilesAcross = (width + tileWidth - 1) / tileWidth;
	const uint32 tilesDown = (length + tileLength - 1) / tileLength;
	const ttile_t numOfTiles = TIFFNumberOfTiles(ptiff);
	
	const uint32 horizFullTiles = width / tileWidth;
	const uint32 horizRemain = width % tileWidth;
	
	const uint32 vertFullTiles = length / tileLength;
	const uint32 vertRemain = length % tileLength;
	
	const tsize_t tileSize = TIFFTileSize(ptiff);
	std::vector<uint8_t> tmpBuff(tileSize);
	
	if (bps % 8) {
		pManager->lastErrorMessage_ = "bps is not multiple of 8.";
		return false;
	}
	const uint32 tileLineBytes = tileWidth * spp * (bps/8);
	const int targetLineOffset = pManager->GetLineOffset();

	if (planarConfig == PLANARCONFIG_CONTIG) {
		for (uint32 ty=0; ty<tilesDown; ++ty) {
			const uint32 yBegin = tileLength * ty;
			const uint32 yEnd = std::min<uint32>(yBegin + tileLength, length);
			char* pTargetLines = pManager->GetLinesBuffer(yBegin, yEnd);
			if (!pTargetLines) {
				return false;
			}
			char* pTargetLineTmp = pTargetLines;
			for (uint32 tx=0; tx<tilesAcross; ++tx) {
				tsize_t ret = TIFFReadEncodedTile(ptiff, ty*tilesAcross+tx, &tmpBuff[0], -1);
				if (ret == -1) {
					return false;
				}
				if (photometric == PHOTOMETRIC_MINISWHITE) {
					InvertSamples((char*)&tmpBuff[0], bps, spp, tileWidth*tileLength);
				}
				const uint32 xBegin = tileWidth * tx;
				const uint32 xEnd = std::min<uint32>(xBegin + tileWidth, width);
				const uint8_t* srcPos = &tmpBuff[0];
				uint8_t* targetPos = (uint8_t*) pTargetLineTmp;
				const size_t xBytes = (xEnd-xBegin) * spp * (bps/8);
				for (size_t y=yBegin; y<yEnd; ++y) {
					std::copy(srcPos, srcPos + xBytes, targetPos);
					srcPos += tileLineBytes;
					targetPos += targetLineOffset;
				}
				pTargetLineTmp += xBytes;
			}
			if (!pManager->ProcessLinesBuffer(yBegin, yEnd, pTargetLines, targetLineOffset)) {
				return false;
			}
		}
	}else if (planarConfig == PLANARCONFIG_SEPARATE) {
		pManager->lastErrorMessage_ = "separate plane tile images are not supported.";
		return false;
	}else {
		assert(false);
		return false;
	}
	return true;
}

bool TIFFReader::ReadStrippedContiguous_PhotometricPalette()
{
	PaletteConverter pc;
	if (TIFFGetField(ptiff, TIFFTAG_COLORMAP, &pc.rPalette, &pc.gPalette, &pc.bPalette) == 0) {
		pManager->lastErrorMessage_ = "color map not found.";
		return false;
	}
	for (size_t row=0; row<length; ++row) {
		char* buff = pManager->GetLineBuffer(row);
		if (buff == NULL) {
			return false;
		}
		TIFFReadScanline(ptiff, buff, row);
		pc.Convert(buff, bps, width);
		if (!pManager->ProcessLineBuffer(row, buff)) {
			return false;
		}
	}
	return true;
}

bool TIFFReader::ReadStrippedContiguous_PhotometricYCbCr_Horiz1_Vert1()
{
	for (size_t row=0; row<length; ++row) {
		char* buff = pManager->GetLineBuffer(row);
		if (buff == NULL) {
			return false;
		}
		TIFFReadScanline(ptiff, buff, row);
		arranger(buff, bps, 3*width);
		if (!pManager->ProcessLineBuffer(row, buff)) {
			return false;
		}
	}
	return true;
}

/*
yCbCr yCbCr

y y Cb Cr
*/
bool TIFFReader::ReadStrippedContiguous_PhotometricYCbCr_Horiz2_Vert1()
{
	const size_t rowSamples = (2 + 2) * (width / 2);
	size_t row = 0;
	std::vector<uint8_t> buff(stripSize);
	for (tstrip_t i=0; i<numOfStrips; ++i) {
		tsize_t result = TIFFReadEncodedStrip(ptiff, i, &buff[0], stripSize);
		arranger((char*)&buff[0], bps, rowsPerStrip * rowSamples);
		uint8_t* pSrc = &buff[0]; 
		for (size_t y=0; y<rowsPerStrip; ++y) {
			size_t y2 = row + y;
			uint8_t* pTarget = (uint8_t*) pManager->GetLineBuffer(y2);
			for (size_t x=0; x<width/2; ++x) {
				const size_t srcPos = 4 * x;
				uint8_t y1 = pSrc[srcPos + 0];
				uint8_t y2 = pSrc[srcPos + 1];
				uint8_t cb = pSrc[srcPos + 2];
				uint8_t cr = pSrc[srcPos + 3];
				
				const size_t targetPos = 6 * x;
				pTarget[targetPos + 0] = y1;
				pTarget[targetPos + 1] = cb;
				pTarget[targetPos + 2] = cr;
				pTarget[targetPos + 3] = y2;
				pTarget[targetPos + 4] = cb;
				pTarget[targetPos + 5] = cr;
			}
			pManager->ProcessLineBuffer(y2, (char*)pTarget);
			pSrc += rowSamples;

		}
		row += rowsPerStrip;
	}
	return true;
}

/*
yCbCr yCbCr
yCbCr yCbCr

y y y y Cb Cr
*/
bool TIFFReader::ReadStrippedContiguous_PhotometricYCbCr_Horiz2_Vert2()
{
	const size_t nSamples = (4 + 2) * (width / 2);
	size_t row = 0;
	std::vector<uint8_t> buff(stripSize);
	const int lineOffset = pManager->GetLineOffset();
	for (tstrip_t i=0; i<numOfStrips; ++i) {
		tsize_t result = TIFFReadEncodedStrip(ptiff, i, &buff[0], -1);
		arranger((char*)&buff[0], bps, rowsPerStrip / 2 * nSamples);
		uint8_t* pSrc = &buff[0];
		for (size_t y=0; y<rowsPerStrip; y+=2) {
			size_t y2 = row + y;
			if (y2+2 >= length) {
				return true;
			}
			uint8_t* pTarget = (uint8_t*) pManager->GetLinesBuffer(y2, y2+2);
			if (!pTarget) {
				return false;
			}
			for (size_t x=0; x<width/2; ++x) {
				size_t pos = 6 * x;
				uint8_t y1 = pSrc[pos + 0];
				uint8_t y2 = pSrc[pos + 1];
				uint8_t y3 = pSrc[pos + 2];
				uint8_t y4 = pSrc[pos + 3];
				uint8_t cb = pSrc[pos + 4];
				uint8_t cr = pSrc[pos + 5];
				
				pTarget[pos + 0] = y1;
				pTarget[pos + 1] = cb;
				pTarget[pos + 2] = cr;
				pTarget[pos + 3] = y2;
				pTarget[pos + 4] = cb;
				pTarget[pos + 5] = cr;
				pos += lineOffset;
				pTarget[pos + 0] = y3;
				pTarget[pos + 1] = cb;
				pTarget[pos + 2] = cr;
				pTarget[pos + 3] = y4;
				pTarget[pos + 4] = cb;
				pTarget[pos + 5] = cr;
			}
			if (!pManager->ProcessLinesBuffer(y2, y2+2, (char*)pTarget, lineOffset)) {
				return false;
			}
			pSrc += nSamples;
		}
		row += rowsPerStrip;
	}
	return true;
}

/*
yCbCr yCbCr yCbCr yCbCr

y y y y Cb Cr
*/
bool TIFFReader::ReadStrippedContiguous_PhotometricYCbCr_Horiz4_Vert1()
{
	const size_t nSamples = (4 + 2) * (width / 4);
	for (size_t row=0; row<length; ++row) {
		char* buff = pManager->GetLineBuffer(row);
		if (buff == NULL) {
			return false;
		}
		TIFFReadScanline(ptiff, buff, row);
		arranger(buff, bps, nSamples);
		if (!pManager->ProcessLineBuffer(row, buff)) {
			return false;
		}
	}
	return true;
}

/*
yCbCr yCbCr yCbCr yCbCr
yCbCr yCbCr yCbCr yCbCr

y y y y y y y y Cb Cr
*/
bool TIFFReader::ReadStrippedContiguous_PhotometricYCbCr_Horiz4_Vert2()
{
	const size_t nSamples = (8 + 2) * (width / 4);
	const int lineOffset = pManager->GetLineOffset();
	for (size_t row=0; row<length; row+=2) {
		char* buff = pManager->GetLinesBuffer(row, row+2);
		if (buff == NULL) {
			return false;
		}
		TIFFReadScanline(ptiff, buff, row);
		TIFFReadScanline(ptiff, buff+scanLineSize, row+1);
		arranger(buff, bps, nSamples);
		if (!pManager->ProcessLinesBuffer(row, row+2, buff, lineOffset)) {
			return false;
		}
	}
	return true;
}

/*
yCbCr yCbCr yCbCr yCbCr
yCbCr yCbCr yCbCr yCbCr
yCbCr yCbCr yCbCr yCbCr
yCbCr yCbCr yCbCr yCbCr

y y y y y y y y y y y y y y y y Cb Cr
*/
bool TIFFReader::ReadStrippedContiguous_PhotometricYCbCr_Horiz4_Vert4()
{
	const size_t nSampels = (16 + 2) * (width / 4);
	const int lineOffset = pManager->GetLineOffset();
	for (size_t row=0; row<length; row+=4) {
		char* buff = pManager->GetLinesBuffer(row, row+4);
		if (buff == NULL) {
			return false;
		}
		TIFFReadScanline(ptiff, buff, row);
		arranger(buff, bps, 2*width);
		if (!pManager->ProcessLinesBuffer(row, row+4, buff, lineOffset)) {
			return false;
		}
	}
	return true;
}

bool TIFFReader::ReadStrippedContiguous_PhotometricYCbCr()
{
	uint16 factorHoriz;
	uint16 factorVert;
	if (TIFFGetField(ptiff, TIFFTAG_YCBCRSUBSAMPLING, &factorHoriz, &factorVert) == 0) {
		factorHoriz = 1;	// horiz
		factorVert = 1;		// vert
	}
	if (factorHoriz != 1 && factorHoriz != 2 && factorHoriz != 4) {
		pManager->lastErrorMessage_ = "invalid YCbCrSubsampleHoriz.";
		return false;
	}
	if (factorVert != 1 && factorVert != 2 && factorVert != 4) {
		pManager->lastErrorMessage_ = "invalid YCbCrSubsampleVert.";
		return false;
	}
	if (factorVert > factorHoriz) {
		pManager->lastErrorMessage_ = "invalid YCbCrSubsampleVert. YCbCrSubsampleVert shall always be less than or equal to YCbCrSubsampleHoriz.";
		return false;
	}
	
	// how to use?
	/*
	uint16 yCbCrPositioning;
	if (TIFFGetField(ptiff, TIFFTAG_YCBCRPOSITIONING, &yCbCrPositioning) == 0) {
		yCbCrPositioning = YCBCRPOSITION_CENTERED;
	}
	*/
	switch (factorHoriz) {
	case 1:
		return ReadStrippedContiguous_PhotometricYCbCr_Horiz1_Vert1();
	case 2:
		switch (factorVert) {
		case 1: return ReadStrippedContiguous_PhotometricYCbCr_Horiz2_Vert1();
		case 2: return ReadStrippedContiguous_PhotometricYCbCr_Horiz2_Vert2();
		}
	case 4:
		switch (factorVert) {
		case 1: return ReadStrippedContiguous_PhotometricYCbCr_Horiz4_Vert1();
		case 2: return ReadStrippedContiguous_PhotometricYCbCr_Horiz4_Vert2();
		case 4: return ReadStrippedContiguous_PhotometricYCbCr_Horiz4_Vert4();
		}
	}
	return true;
}

union Multi
{
	unsigned char c[4];
	unsigned short int s[2];
	unsigned int i[1];
	float f[1];
};

bool TIFFReader::ReadStrippedContiguous()
{
	// TODO: 記録されているendianと実行環境のendianが異なった場合に切り分ける。
	
	if (bps == 8 || bps == 16 || bps == 32) arranger.bind(&DoNothing);
	else if (bps < 8)						arranger.bind(&BitsToUInt8);
	else if (bps < 16)						arranger.bind(&BitsToUInt16);
	else if (bps == 24)						arranger.bind(&TripleBytesToUInt32);
	else if (bps < 32)						arranger.bind(&BitsToUInt32);
	
	switch (photometric) {
	case PHOTOMETRIC_PALETTE:
		ReadStrippedContiguous_PhotometricPalette();
		break;
	case PHOTOMETRIC_YCBCR:
		ReadStrippedContiguous_PhotometricYCbCr();
		break;
	default:
		for (size_t row=0; row<length; ++row) {
			char* buff = pManager->GetLineBuffer(row);
			if (buff == NULL) {
				return false;
			}
			TIFFReadScanline(ptiff, buff, row);
			arranger(buff, bps, spp*width);
			if (photometric == PHOTOMETRIC_MINISWHITE) {
				InvertSamples(buff, bps, spp, width);
			}
			Multi* pMultio = (Multi*) buff;
			if (!pManager->ProcessLineBuffer(row, buff)) {
				return false;
			}
		}
		break;
	}
	return true;
}

bool TIFFReader::ReadStrippedSeparate()
{
	fastdelegate::FastDelegate6<
		char* /*targetBuff*/,
		char* /*sourceBuff*/,
		size_t /*spp*/,
		size_t /*bps*/,
		size_t /*scanLineSize*/,
		size_t /*width*/
	> sampleCollector;
	
	if (bps < 8)			sampleCollector.bind(&CollectSamples_Under8);
	else if (bps == 8)		sampleCollector.bind(&CollectSamples_8);
	else if (bps < 16)		sampleCollector.bind(&CollectSamples_Under16);
	else if (bps == 16)		sampleCollector.bind(&CollectSamples_16);
	else if (bps == 24)		sampleCollector.bind(&CollectSamples_24);
	else if (bps == 32)		sampleCollector.bind(&CollectSamples_32);
	else if (bps == 64)		sampleCollector.bind(&CollectSamples_64);
	else {
		pManager->lastErrorMessage_ = "ReadStrippedSeparate not supported bps";
		return false;
	}
	
	if (compress != COMPRESSION_NONE && rowsPerStrip != 1) {
#if 0
		std::vector<char> tmpBuff(stripSize);
		uint32 y = 0;
		for (size_t s=0; s<numOfStrips; ++s) {
			tsize_t ret = TIFFReadEncodedStrip(ptiff, s, &tmpBuff[0], -1);
			if (ret == -1) {
				return false;
			}
			const size_t endY = std::min<uint32>(y + rowsPerStrip, length);
			char* srcLinePos = &tmpBuff[0];
//			for (; y<endY; ++y) {
				char* targetBuff = pManager->GetLinesBuffer(y, endY);
				if (targetBuff == NULL) {
					return false;
				}
				sampleCollector(targetBuff, srcLinePos, spp, bps, scanLineSize, width*rowsPerStrip);
				if (!pManager->ProcessLinesBuffer(y, endY, targetBuff)) {
					return false;
				}
//			}
			y = endY;
		}
#endif
	}else {
		std::vector<char> tmpBuff(scanLineSize * spp);
		for (size_t row=0; row<length; ++row) {
			char* tmp = &tmpBuff[0];
			for (size_t s=0; s<spp; ++s) {
				TIFFReadScanline(ptiff, tmp, row, s);
				tmp += scanLineSize;
			}
			char* buff = pManager->GetLineBuffer(row);
			if (buff == NULL) {
				return false;
			}
			sampleCollector(buff, &tmpBuff[0], spp, bps, scanLineSize, width);
			if (!pManager->ProcessLineBuffer(row, buff)) {
				return false;
			}
		}
	}
	return true;
}

bool TIFFReader::Read(ImageReader* pImageReader, TIFF* ptiff)
{
	pManager = pImageReader;
	this->ptiff = ptiff;
	
	TIFFGetField(ptiff, TIFFTAG_PLANARCONFIG, &planarConfig);
	if (TIFFGetField(ptiff, TIFFTAG_COMPRESSION, &compress) == 0) {
		compress = COMPRESSION_NONE;
	}
	TIFFGetField(ptiff, TIFFTAG_BITSPERSAMPLE, &bps);
	TIFFGetField(ptiff, TIFFTAG_SAMPLESPERPIXEL, &spp);
	TIFFGetField(ptiff, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(ptiff, TIFFTAG_IMAGELENGTH, &length);
	TIFFGetField(ptiff, TIFFTAG_PHOTOMETRIC, &photometric);
	isBigEndian = (TIFFIsBigEndian(ptiff) != 0);

	if (photometric == PHOTOMETRIC_LOGL || photometric == PHOTOMETRIC_LOGLUV) {
		if (TIFFGetField(ptiff, TIFFTAG_STONITS, &stonits) == 0) {
			stonits = 1.0;
		}
	}
	
	if (TIFFIsTiled(ptiff)) {
		return ReadTiled();
	}else {
		if (compress == COMPRESSION_OJPEG || compress == COMPRESSION_JPEG) {
			std::vector<uint8_t> buff(width*length*4);
			int ret = TIFFReadRGBAImageOriented(ptiff, width, length, (uint32*)&buff[0], ORIENTATION_LEFTTOP);
			if (ret <= 0) {
 				return false;
			}
			return pManager->ProcessLinesBuffer(0, length, (char*)&buff[0], width*4);
		}else {
			stripSize = TIFFStripSize(ptiff);
			numOfStrips = TIFFNumberOfStrips(ptiff);
			
			TIFFGetField(ptiff, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);
			rasterScaLineSize = TIFFRasterScanlineSize(ptiff);
			scanLineSize = TIFFScanlineSize(ptiff);

			if (planarConfig == PLANARCONFIG_CONTIG) {
				return ReadStrippedContiguous();
			}else if (planarConfig == PLANARCONFIG_SEPARATE) {
				return ReadStrippedSeparate();
			}else {
				assert(false);
				return false;
			}
		}
	}
}
	
}	// namespace ImageIO
