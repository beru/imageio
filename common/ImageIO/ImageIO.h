#pragma once

#include "File.h"

#include <type_traits>
#include <memory>

#include "PathUtility.h"

namespace ImageIO
{

enum ColorFormat {
	ColorFormat_Unknown,
	ColorFormat_Mono,
	ColorFormat_RGB,
	ColorFormat_RGBA,
	ColorFormat_ARGB,
	ColorFormat_BGR,
	ColorFormat_BGRA,
	ColorFormat_ABGR,
	ColorFormat_CMYK,
	ColorFormat_YCCK,
	ColorFormat_YCbCr,
//	ColorFormat_XYZ,
};

struct ImageInfo
{
	size_t width;
	size_t height;
	int lineOffset;
	int bitsPerPixel;
	int bitsPerSample;
	int samplesPerPixel;
	ColorFormat colorFormat;
};
	
enum ImageFormatType {
	ImageFormatType_Unknown,
	ImageFormatType_BMP,
	ImageFormatType_JPEG,
	ImageFormatType_TIFF,
	ImageFormatType_PNG,
};

ImageFormatType EstimateImageFormatTypeFromPath(LPCTSTR path);

} // namespace ImageIO
