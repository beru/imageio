#include "ImageReader.h"

#define XMD_H
#undef FAR
extern "C" {
#define JPEG_INTERNAL_OPTIONS
#include "libjpeg/jpeglib.h"
#include "libjpeg/jconfig.h"
#include "libjpeg/jmorecfg.h"
#include "libjpeg/jerror.h"
}
#include "misc/JPEGManager.h"

#include <boost/static_assert.hpp>

namespace ImageIO {

void JpegErrorHandler(j_common_ptr ptr)
{
    jpeg_decompress_struct* jdJpeg = (jpeg_decompress_struct*)ptr;
	
	char buff[1024] = {0};
	jdJpeg->err->format_message((j_common_ptr)jdJpeg, buff);
	
	ImageReader* pReader = (ImageReader*) jdJpeg->client_data;
	pReader->SetLastErrorMessage(buff);
}

bool ImageReader::ReadSourceInformation(jpeg_decompress_struct& jd)
{
	imageInfo_.width = jd.image_width;
	imageInfo_.height = jd.image_height;
	imageInfo_.bitsPerSample = 8;
	imageInfo_.samplesPerPixel = jd.num_components;
	imageInfo_.bitsPerPixel = imageInfo_.samplesPerPixel * imageInfo_.bitsPerSample;
	imageInfo_.lineOffset = imageInfo_.width * imageInfo_.samplesPerPixel;

	switch (jd.out_color_space) {
	case JCS_GRAYSCALE:	imageInfo_.colorFormat = ColorFormat_Mono; break;
	case JCS_RGB:		imageInfo_.colorFormat = ColorFormat_BGR; break;
	case JCS_YCbCr:		imageInfo_.colorFormat = ColorFormat_YCbCr; break;
	case JCS_CMYK:		imageInfo_.colorFormat = ColorFormat_CMYK; break;
	case JCS_YCCK:		imageInfo_.colorFormat = ColorFormat_YCCK; break;
	case JCS_UNKNOWN:
	default:
		lastErrorMessage_ = "unknown color space.";
		return false;
	}
	
	return true;
}

/*!

-- Reference Site --
http://tanack.hp.infoseek.co.jp/program.html

*/

bool ImageReader::ReadJPEG(IFile& file)
{
	imageFormatType_ = ImageFormatType_JPEG;

	jpeg_decompress_struct jdJpeg;	  // デコード用構造体
	jdJpeg.do_fancy_upsampling = 0;
	
	JSAMPROW jsrWork;				  // 作業用JSAMPROW
	
	jpeg_error_mgr jemError;		  // JPEGエラーマネージャ
	jdJpeg.err = jpeg_std_error(&jemError);
	jemError.error_exit = JpegErrorHandler;
	jdJpeg.client_data = this;
	
	
	// デコード準備
	jpeg_create_decompress(&jdJpeg);
	jdJpeg.dct_method = JDCT_IFAST;

	// ソースを設定
	JPEGManager<8192> jpegManager(&file);
	jdJpeg.src = &jpegManager;
	
	// ヘッダ読み込み
	jpeg_read_header(&jdJpeg, TRUE);
	
	if (!ReadSourceInformation(jdJpeg)) {
		return false;
	}
	if (!Setup()) {
		return false;
	}
	
	// デコード開始
	jpeg_start_decompress(&jdJpeg);
	
	bool bRet = true;
	while (jdJpeg.output_scanline < jdJpeg.output_height) {
		size_t readLine = jdJpeg.output_scanline;
		char* buff = GetLineBuffer(readLine);
		if (buff == NULL) {
			jpeg_abort((j_common_ptr)&jdJpeg);
			bRet = false;
			goto End;
		}
		jsrWork = (JSAMPROW)((BYTE*)buff);
		jpeg_read_scanlines(&jdJpeg, (JSAMPARRAY)&jsrWork , 1);
		if (!ProcessLineBuffer(readLine, buff)) {
			jpeg_abort((j_common_ptr)&jdJpeg);
			bRet = false;
			goto End;
		}
	}
	jpeg_finish_decompress(&jdJpeg);
	
End:
	jpeg_destroy_decompress(&jdJpeg);
	return bRet;
}

}	// namespace ImageIO
