#pragma once

#include <vector>

#include "ImageReader.h"
#include "fastdelegate.h"

namespace ImageIO
{

struct DIBSection
{
	DIBSection();
	
	HBITMAP hBMP_;
	BITMAPINFOHEADER bmih_;
	RGBQUAD bmiColors_;
	
	void* pBits_;
	int lineOffset_;
	
	char* GetLineBuffer(size_t line);
	void Delete();
	void* GetFirstLinePtr() const;
};

class DIBSectionReader : public ImageReader
{
public:
	DIBSectionReader(size_t bpp=24, size_t maxWidth = 8192+16, size_t maxHeight = 8192);
	virtual char* GetLineBuffer(size_t line);
	virtual char* GetLinesBuffer(size_t beginLine, size_t endLine);
	virtual int GetLineOffset();
	virtual bool ProcessLineBuffer(size_t lineIdx, char* buff);
	DIBSection* pImage_;
	
protected:
	virtual bool Setup();
	const size_t bpp_;
	const size_t maxWidth_;
	const size_t maxHeight_;
	bool bUseImageBuffer_;
	bool bNeedToProcess_;
	std::vector<char> buff_;
	
	std::shared_ptr<class ILineColorConverter> pLineColorConverter_;
};

} // namespace ImageIO

