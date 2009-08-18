#pragma once

#include "ImageIO.h"
#include <string>

namespace ImageIO {

class ILineColorConverter
{
public:
	virtual bool Convert(const char* src, char* target) = 0;
};

ILineColorConverter* CreateLineColorConverter(const ImageInfo& srcImageInfo, const ImageInfo& targetImageInfo, std::string& errorMessage);

}	// namespace ImageIO
