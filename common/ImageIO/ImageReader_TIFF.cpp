#include "ImageReader.h"

extern "C" {
#include "LibTIFF/tiffio.h"
#include "LibTIFF/tiffiop.h"
}

#include "misc/TIFFinterface.h"
#include <math.h>

#include "TIFFReader.h"

namespace ImageIO {

bool ImageReader::ReadSourceInformation(TIFF* ptiff)
{
	uint16 config;
	if (TIFFGetField(ptiff, TIFFTAG_PLANARCONFIG, &config) == 0) {
		lastErrorMessage_ = "cannot get TIFFTAG_PLANARCONFIG field.";
		return false;
	}
	
	uint16 bps;
	if (TIFFGetField(ptiff, TIFFTAG_BITSPERSAMPLE, &bps) == 0) {
		lastErrorMessage_ = "bits per sample undefined";
		return false;
	}
	uint16 spp;
	if (TIFFGetField(ptiff, TIFFTAG_SAMPLESPERPIXEL, &spp) == 0) {
		lastErrorMessage_ = "samples per pixels undefined";
		return false;
	}
	if (spp != 1 && spp != 3 && spp != 4) {
		lastErrorMessage_ = "unsupported samples per pixel";
		return false;
	}
	uint32 width;
	if (TIFFGetField(ptiff, TIFFTAG_IMAGEWIDTH, &width) == 0) {
		lastErrorMessage_ = "image width undefined";
		return false;
	}
	uint32 length;
	if (TIFFGetField(ptiff, TIFFTAG_IMAGELENGTH, &length) == 0) {
		lastErrorMessage_ = "image length undefined";
		return false;
	}
	uint16 photometric;
	if (TIFFGetField(ptiff, TIFFTAG_PHOTOMETRIC, &photometric) == 0) {
		lastErrorMessage_ = "photometric interpretation undefined";
		return false;
	}
	uint16 compress;
	if (TIFFGetField(ptiff, TIFFTAG_COMPRESSION, &compress) == 0) {
		compress = COMPRESSION_NONE;
	}

	// TODO: to follow
#if 0
	uint16 sampleFormats[32];
	if (TIFFGetField(ptiff, TIFFTAG_SAMPLEFORMAT, sampleFormats) == 0) {
//		sampleFormat = SAMPLEFORMAT_UINT;
	}
#endif
	
	imageInfo_.colorFormat = ColorFormat_Unknown;
	switch (photometric) {
	case PHOTOMETRIC_MINISWHITE:
	case PHOTOMETRIC_MINISBLACK:
		imageInfo_.colorFormat = ColorFormat_Mono;
		break;
	case PHOTOMETRIC_PALETTE:
		imageInfo_.colorFormat = ColorFormat_RGB;
		break;
	case PHOTOMETRIC_RGB:
		if (spp == 3) {
			imageInfo_.colorFormat = ColorFormat_RGB;
		}else if (spp = 4) {
			imageInfo_.colorFormat = ColorFormat_RGBA;
		}
		break;
	case PHOTOMETRIC_MASK:
		break;
	case PHOTOMETRIC_SEPARATED:
		imageInfo_.colorFormat = ColorFormat_CMYK;
		break;
	case PHOTOMETRIC_YCBCR:
		imageInfo_.colorFormat = ColorFormat_YCbCr;
		break;
	case PHOTOMETRIC_CIELAB:
//	case PHOTOMETRIC_ICCLAB:
	case PHOTOMETRIC_ITULAB:
		break;
	case PHOTOMETRIC_LOGLUV:
	case PHOTOMETRIC_LOGL:
#if 0
		if (compress != COMPRESSION_SGILOG && compress != COMPRESSION_SGILOG24) {
			lastErrorMessage_ = "Only support SGILOG compressed LogLuv data";
			return false;
		}
		TIFFSetField(ptiff, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
		imageInfo_.colorFormat = ColorFormat_XYZ;
#else
		TIFFSetField(ptiff, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
		if (spp == 3) {
			imageInfo_.colorFormat = ColorFormat_RGB;
		}else {
			imageInfo_.colorFormat = ColorFormat_Mono;
		}
#endif
		break;
	default:
		lastErrorMessage_ = "TIFF has unsupported photometric type";
		return false;
	}

	if (compress == COMPRESSION_OJPEG || compress == COMPRESSION_JPEG) {
		imageInfo_.colorFormat = ColorFormat_BGRA;
		imageInfo_.bitsPerSample = 8;
		imageInfo_.samplesPerPixel = 4;
	}else {
		if (photometric == PHOTOMETRIC_PALETTE) {
			imageInfo_.bitsPerSample = 16;
			imageInfo_.samplesPerPixel = 3;
		}else if (photometric == PHOTOMETRIC_LOGLUV || photometric == PHOTOMETRIC_LOGL) {
#if 0
			imageInfo_.bitsPerSample = 32;
			imageInfo_.samplesPerPixel = 3;
#else
			imageInfo_.bitsPerSample = 8;
			imageInfo_.samplesPerPixel = spp;
#endif
		}else {
			if (bps <= 8) {
				imageInfo_.bitsPerSample = 8;
			}else if (bps <= 16) {
				imageInfo_.bitsPerSample = 16;
			}else if (bps <= 32) {
				imageInfo_.bitsPerSample = 32;
			}else {
				imageInfo_.bitsPerSample = 64;
			}
			imageInfo_.samplesPerPixel = spp;
		}
	}
	
	imageInfo_.width = width;
	imageInfo_.height = length;
	imageInfo_.bitsPerPixel = imageInfo_.bitsPerSample * imageInfo_.samplesPerPixel;
	imageInfo_.lineOffset = TIFFRasterScanlineSize(ptiff);
	imageInfo_.lineOffset = imageInfo_.bitsPerSample/8 * imageInfo_.samplesPerPixel * imageInfo_.width;
	
	return true;
}

/*---------------------------------------------------------------------------------
reference sites
http://www.awaresystems.be/imaging/tiff.html
http://www.awaresystems.be/imaging/tiff/tifftags.html
http://www-06.ibm.com/jp/developerworks/linux/020517/j_l-libtiff.html
http://www-06.ibm.com/jp/developerworks/linux/020802/j_l-libtiff2.html

TODO: -- HDR images --
http://www.anyhere.com/gward/pixformat/tiffluv.html
http://www.anyhere.com/gward/hdrenc/hdr_encodings.html
http://www.anyhere.com/gward/papers/jgtpap1.pdf
*/
bool ImageReader::ReadTIFF(IFile& file)
{
	imageFormatType_ = ImageFormatType_TIFF;

	TIFFSetErrorHandler(NULL);
	TIFFSetWarningHandler(NULL);

	TIFF* ptiff = OpenTiff(&file);
	if (!ptiff) {
		lastErrorMessage_ = "open error";
		return false;
	}
	bool ret = false;
	if (ReadSourceInformation(ptiff)) {
		if (Setup()) {
			TIFFReader reader;
			ret = reader.Read(this, ptiff);
		}
	}
	TIFFClose(ptiff);
	return ret;
}

}	// namespace ImageIO
