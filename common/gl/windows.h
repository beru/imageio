#pragma once

/*
	Windows“Á—L‚Ìˆ—”z’u
*/

#include "color.h"
#include "fixed.h"

namespace gl {

class IBuffer2D;
gl::IBuffer2D* BuildBuffer2DFromBMP(const BITMAPINFOHEADER& header, void* pBits);

typedef Color4< ColorBGRA< fixed<8, unsigned char> > > WinColor4;
typedef Color3< ColorBGR< fixed<8, unsigned char> > > WinColor3;

}


