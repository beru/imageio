#include "ImageReader.h"

#include "../libpng/png.h"
#include <vector>

namespace ImageIO {

struct Reader
{
	Reader(IFile& file)
		:
		file_(file)
	{
	}
	
	void Read(size_t*& data, size_t readSize)
	{
		DWORD nBytesRead = 0;
		file_.Read(data, readSize, nBytesRead);
	}
	
	IFile& file_;
};

static void ReadDataFunc(png_structp png_ptr, png_bytep data, png_size_t length)
{
	Reader* pReader = (Reader*) png_get_io_ptr(png_ptr);
	pReader->Read((size_t*&)data, length);
}

bool sys_is_little_endian(){
#if defined(__LITTLE_ENDIAN__)
	return true;
#elif defined(__BIG_ENDIAN__)
	return false;
#else
	int i = 1;
	return (bool)(*(char*)&i);
#endif
}

// http://libpng.org/pub/png/pngsuite.html
// http://dencha.ojaru.jp/programs_07/pg_graphic_10_libpng_txt.html
bool ImageReader::ReadSourceInformation(png_struct_def* pPNG, png_info_struct* pINFO)
{
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_method;
	int compression_method, filter_method;
	png_get_IHDR(
		pPNG, pINFO,
		&width, &height, &bit_depth, &color_type,
		&interlace_method, &compression_method, &filter_method
	);
	
	png_set_bgr(pPNG);
	bool hasAlpha = false;
	bool isGrayscale = false;
	
	switch (color_type) {
	case PNG_COLOR_TYPE_GRAY:
		isGrayscale = true;
		if (bit_depth < 8) {
			png_set_gray_1_2_4_to_8(pPNG);
		}
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		hasAlpha = true;
		break;
	case PNG_COLOR_TYPE_PALETTE:
		png_set_palette_to_rgb(pPNG);
		break;
	}

	if (png_get_valid(pPNG, pINFO, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(pPNG);
		hasAlpha = true;
	}
	if (hasAlpha && isGrayscale) {
		png_set_gray_to_rgb(pPNG);
		isGrayscale = false;
	}

	imageInfo_.width = width;
	imageInfo_.height = height;
	
#if 0
	if (bit_depth == 16) {
		png_set_strip_16(pPNG);
	}
	imageInfo_.bitsPerSample = 8;
#else
	imageInfo_.bitsPerSample = (bit_depth <= 8) ? 8 : 16;
	if (sys_is_little_endian()) {
		png_set_swap(pPNG);		// little endian
	}
#endif
	if (isGrayscale) {
		imageInfo_.samplesPerPixel = 1;
		imageInfo_.colorFormat = ColorFormat_Mono;
	}else {
		if (hasAlpha) {
			imageInfo_.samplesPerPixel = 4;
			imageInfo_.colorFormat = ColorFormat_BGRA;
		}else {
			imageInfo_.samplesPerPixel = 3;
			imageInfo_.colorFormat = ColorFormat_BGR;
		}
	}
	imageInfo_.bitsPerPixel = imageInfo_.bitsPerSample * imageInfo_.samplesPerPixel;
	imageInfo_.lineOffset = imageInfo_.width * imageInfo_.samplesPerPixel * (imageInfo_.bitsPerPixel) / 8;
	
	return true;
}

bool ImageReader::ReadPNG(IFile& file)
{
	imageFormatType_ = ImageFormatType_PNG;

	png_structp pPNG = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pPNG) {
		return false;
	}
	png_infop pINFO = png_create_info_struct(pPNG);
	if (!pINFO) {
		png_destroy_read_struct(&pPNG, NULL, NULL);
		return false;
	}
	
	if (setjmp(png_jmpbuf(pPNG))) {
		png_destroy_read_struct(&pPNG, &pINFO, (png_infopp)NULL);
		// TODO: to destruct linePtrs.
		return false;
	}

	Reader reader(file);
	png_set_read_fn(pPNG, &reader, ReadDataFunc);
	png_read_info(pPNG, pINFO);
	
	if (!ReadSourceInformation(pPNG, pINFO)) {
		return false;
	}
	if (!Setup()) {
		return false;
	}
	
	char* pBuffer = GetLinesBuffer(0, imageInfo_.height);
	if (!pBuffer) {
		return false;
	}
	std::vector<char*> linePtrs(imageInfo_.height);
	const int lineOffset = GetLineOffset();
	for (size_t i=0; i<imageInfo_.height; ++i) {
		linePtrs[i] = pBuffer;
		pBuffer += lineOffset;
	}
	
	png_read_image(pPNG, (png_bytepp)&linePtrs[0]);
	png_read_end(pPNG, pINFO);
	png_destroy_read_struct(&pPNG, &pINFO, (png_infopp)NULL);
	
	if (!ProcessLinesBuffer(0, imageInfo_.height, linePtrs[0], lineOffset)) {
		return false;
	}

	return true;
}

}	// namespace ImageIO
