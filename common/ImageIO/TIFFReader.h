#pragma once

extern "C" {
#include "LibTIFF/tiffio.h"
#include "LibTIFF/tiffiop.h"
}

#include "fastdelegate.h"

namespace ImageIO {

class ImageReader;

// TODO: どのページを読むか指定出来るようにする。

struct TIFFReader
{
public:
	bool Read(ImageReader* pReader, TIFF* ptiff);
	
private:
	bool ReadTiled();
	bool ReadStrippedContiguous();
	bool ReadStrippedSeparate();
	
	bool ReadStrippedContiguous_PhotometricPalette();

	bool ReadStrippedContiguous_PhotometricYCbCr();
	bool ReadStrippedContiguous_PhotometricYCbCr_Horiz1_Vert1();
	bool ReadStrippedContiguous_PhotometricYCbCr_Horiz2_Vert1();
	bool ReadStrippedContiguous_PhotometricYCbCr_Horiz2_Vert2();
	bool ReadStrippedContiguous_PhotometricYCbCr_Horiz4_Vert1();
	bool ReadStrippedContiguous_PhotometricYCbCr_Horiz4_Vert2();
	bool ReadStrippedContiguous_PhotometricYCbCr_Horiz4_Vert4();
	
	fastdelegate::FastDelegate3<char* /*buff*/, size_t /*bps*/, size_t /*totalSamples*/> arranger;

	ImageReader* pManager;
	TIFF* ptiff;
	
	uint16 planarConfig;
	uint16 compress;
	uint32 rowsPerStrip;
	uint16 bps;
	uint16 spp;
	uint32 width;
	uint32 length;
	uint16 photometric;
	bool isBigEndian;
	tsize_t rasterScaLineSize;
	tsize_t scanLineSize;
	double stonits;

	tsize_t stripSize;
	tstrip_t numOfStrips;

};

}	// namespace ImageIO
