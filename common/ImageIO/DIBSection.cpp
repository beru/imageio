#include "DIBSection.h"

#include "CreateDIB.h"
#include "ArrayUtil.h"

#undef min
#undef max

#include "LineColorConverter.h"

namespace ImageIO
{

DIBSection::DIBSection()
	:
	hBMP_(NULL),
	pBits_(NULL)
{
}

void DIBSection::Delete()
{
	if (hBMP_) {
		DeleteObject(hBMP_);
		pBits_ = 0;
	}
}

char* DIBSection::GetLineBuffer(size_t line)
{
	return (char*)GetFirstLinePtr() + lineOffset_ * line;
}

void* DIBSection::GetFirstLinePtr() const
{
	if (lineOffset_ > 0) {
		return pBits_;
	}else {
		size_t height = abs(bmih_.biHeight);
		return (char*)pBits_ + abs(lineOffset_) * (height - 1);
	}
}

DIBSectionReader::DIBSectionReader(size_t bpp, size_t maxWidth, size_t maxHeight)
	:
	pImage_(NULL),
	bpp_(bpp),
	maxWidth_(maxWidth),
	maxHeight_(maxHeight)
{
}

bool DIBSectionReader::Setup()
{
	bUseImageBuffer_ = false;
	if (imageInfo_.bitsPerPixel <= bpp_) {
		bUseImageBuffer_ = true;
	}
	
	size_t setupWidth = imageInfo_.width;
	// 128bit単位にしたいので、横を16の倍数で確保する。
	if (setupWidth % 16) {
		setupWidth += 16 - setupWidth % 16;
	}
	// 画像処理の関係上、横を余分に16ドット多く確保する。（確保領域を前後に16byte余分に作る方法に比べると勿体無いけど、CreateDIBSectionで指定出来ず…）
	setupWidth += 16;
	
	size_t setupHeight = imageInfo_.height;
	
	if (setupWidth == 0 || setupWidth > maxWidth_) {
		lastErrorMessage_ = "invalid width";
		return false;
	}
	if (setupHeight == 0 || setupHeight > maxHeight_) {
		lastErrorMessage_ = "invalid height";
		return false;
	}
	
	pImage_->Delete();
	pImage_->hBMP_ = CreateDIB(setupWidth, setupHeight, bpp_, pImage_->lineOffset_, (BITMAPINFO&)pImage_->bmih_, pImage_->pBits_);
	
	ImageInfo targetImageInfo;
	targetImageInfo.width = pImage_->bmih_.biWidth;
	targetImageInfo.height = pImage_->bmih_.biHeight;
	targetImageInfo.bitsPerSample = 8;
	targetImageInfo.samplesPerPixel = bpp_/8;
	targetImageInfo.lineOffset = pImage_->lineOffset_;
	targetImageInfo.bitsPerPixel = pImage_->bmih_.biBitCount;
	targetImageInfo.colorFormat = (bpp_ == 24) ? ColorFormat_BGR : ColorFormat_BGRA;
	
	bNeedToProcess_ = (0
		|| !bUseImageBuffer_
		|| imageInfo_.colorFormat != targetImageInfo.colorFormat
		|| imageInfo_.samplesPerPixel != targetImageInfo.samplesPerPixel
		|| imageInfo_.bitsPerSample != targetImageInfo.bitsPerSample
		|| imageInfo_.bitsPerPixel != targetImageInfo.bitsPerPixel
	);
	
	pLineColorConverter_ = boost::shared_ptr<ILineColorConverter>(CreateLineColorConverter(imageInfo_, targetImageInfo, lastErrorMessage_));
	if (!pLineColorConverter_) {
		lastErrorMessage_ = "cannot create line color converter. " + lastErrorMessage_;
		return false;
	}
	
	return true;
}

char* DIBSectionReader::GetLineBuffer(size_t line)
{
	if (bUseImageBuffer_) {
		return pImage_->GetLineBuffer(line);
	}else {
		const size_t needLen = abs(imageInfo_.lineOffset);
		if (buff_.size() < needLen) {
			buff_.resize(needLen);
		}
		return &buff_[0];
	}
}

char* DIBSectionReader::GetLinesBuffer(size_t beginLine, size_t endLine)
{
	if (bUseImageBuffer_) {
		return pImage_->GetLineBuffer(beginLine);
	}else {
		const size_t nLines = abs((int)endLine - (int)beginLine);
		const size_t needLen = nLines * abs(imageInfo_.lineOffset);
		if (buff_.size() < needLen) {
			buff_.resize(needLen);
		}
		return &buff_[0];
	}
}

bool DIBSectionReader::ProcessLineBuffer(size_t lineIdx, char* buff)
{
	return bNeedToProcess_ ? pLineColorConverter_->Convert(buff, pImage_->GetLineBuffer(lineIdx)) : true;
}

int DIBSectionReader::GetLineOffset()
{
	return bUseImageBuffer_ ? pImage_->lineOffset_ : imageInfo_.lineOffset;
}

} // namespace ImageIO

