#include "ImageIO.h"

namespace ImageIO {

ImageFormatType EstimateImageFormatTypeFromPath(LPCTSTR path)
{
	CString strExt = PathUtility::FindExtension(path);
	strExt.Delete(0,1);
	strExt.MakeLower();
	ImageFormatType formatType = ImageFormatType_Unknown;
	if (0) {
		;
	}else if (strExt == _T("bmp")) {
		formatType = ImageFormatType_BMP;
	}else if (strExt == _T("jpg") || strExt == _T("jpeg")) {
		formatType = ImageFormatType_JPEG;
	}else if (strExt == _T("tif") || strExt == _T("tiff")) {
		formatType = ImageFormatType_TIFF;
	}else if (strExt == _T("png") || strExt == _T("ping")) {
		formatType = ImageFormatType_PNG;
	}
	return formatType;
}

} // namespace ImageIO

