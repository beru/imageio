#include "ImageReader.h"

#include "File.h"

namespace ImageIO {

bool ImageReader::IsReadablePath(LPCTSTR filePath)
{
	ImageFormatType type = EstimateImageFormatTypeFromPath(filePath);
	return (type != ImageFormatType_Unknown);
}

bool ImageReader::Read(LPCTSTR filePath)
{
	ImageFormatType type  = EstimateImageFormatTypeFromPath(filePath);
	if (type == ImageFormatType_Unknown) {
		// TODO: check fileformat from file header
		lastErrorMessage_ = "unknown format";
		return false;
	}
	HANDLE hFile = CreateFile(
		filePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL
	);
	File file(hFile);
	bool ret = Read(file, type);
	CloseHandle(hFile);
	return ret;
}

bool ImageReader::Read(IFile& file, ImageFormatType type)
{
	switch (type) {
	case ImageFormatType_BMP:	return ReadBMP(file);
	case ImageFormatType_JPEG:	return ReadJPEG(file);
	case ImageFormatType_TIFF:	return ReadTIFF(file);
	case ImageFormatType_PNG:	return ReadPNG(file);
	default:
		lastErrorMessage_ = "unsupported image format.";
		return false;
	}
}

bool ImageReader::ProcessLinesBuffer(size_t beginLine, size_t endLine, char* buff, int lineOffset)
{
	bool ret = true;
	for (size_t i=beginLine; i<endLine; ++i) {
		if (!ProcessLineBuffer(i, buff)) {
			return false;
		}
		buff += lineOffset;
	}
	return true;
}

}	// namespace ImageIO
