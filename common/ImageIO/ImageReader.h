#pragma once

#include "ImageIO.h"
#include <string>
#include <vector>

struct jpeg_decompress_struct;
struct tiff;
struct png_struct_def;
struct png_info_struct;
typedef struct png_info_def png_info;

namespace ImageIO {

class ImageReader
{
friend struct TIFFReader;

public:
	static bool IsReadablePath(LPCTSTR filePath);
	bool Read(LPCTSTR filePath);
	bool Read(IFile& file, ImageFormatType type);
	
	bool ReadBMP(IFile& file);
	bool ReadJPEG(IFile& file);
	bool ReadTIFF(IFile& file);
	bool ReadPNG(IFile& file);
	
	std::string GetLastErrorMessage() const { return lastErrorMessage_; }
	void SetLastErrorMessage(LPCSTR buff) { lastErrorMessage_ = buff; }

	const ImageInfo& GetImageInfo() const { return imageInfo_; }
	ImageFormatType GetImageFormatType() const { return imageFormatType_; }
	
protected:
	virtual bool ReadSourceInformation(const BITMAPINFOHEADER& bmih, const RGBQUAD* bmiColors);
	virtual bool ReadSourceInformation(jpeg_decompress_struct& jd);
	virtual bool ReadSourceInformation(tiff* ptiff);
	virtual bool ReadSourceInformation(png_struct_def* pPNG, png_info* pINFO);
	ImageInfo imageInfo_;
	
	virtual bool Setup() = 0;
	virtual char* GetLineBuffer(size_t line) = 0;
	virtual char* GetLinesBuffer(size_t beginLine, size_t endLine) = 0;
	virtual int GetLineOffset() = 0;
	
	virtual bool ProcessLinesBuffer(size_t beginLine, size_t endLine, char* buff, int lineOffset);
	virtual bool ProcessLineBuffer(size_t lineIdx, char* buff) = 0;
	
	std::string lastErrorMessage_;
	ImageFormatType imageFormatType_;
	std::vector<unsigned char> buff_;
};

}	// namespace ImageIO
