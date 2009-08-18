#include "stdafx.h"

#include "LineColorConverter.h"

#include "gl/color.h"
#include "gl/fixed.h"

template <typename ComponentT>
struct ColorYCbCr
{
	typedef typename ComponentT value_type;
	ComponentT y,cb,cr;
};

template <typename ComponentT>
struct ColorCMYK
{
	typedef typename ComponentT value_type;
	ComponentT c,m,y,k;
};

template <typename ComponentT>
struct ColorYCCK
{
	typedef typename ComponentT value_type;
	ComponentT y,c1,c2,k;
};

template <typename ComponentT>
struct ColorXYZ
{
	typedef typename ComponentT value_type;
	ComponentT x,y,z;
};

typedef gl::fixed<8, unsigned char>				Fixed8;
typedef gl::fixed<16, unsigned short int>		Fixed16;
typedef gl::fixed<32, unsigned int>				Fixed32;

typedef gl::Color1< Fixed8 >					ColorMono8i;
typedef gl::Color3< gl::ColorRGB<Fixed8> >		ColorRGB8i;
typedef gl::Color3< gl::ColorBGR<Fixed8> >		ColorBGR8i;
typedef gl::Color4< gl::ColorRGBA<Fixed8> >		ColorRGBA8i;
typedef gl::Color4< gl::ColorARGB<Fixed8> >		ColorARGB8i;
typedef gl::Color4< gl::ColorBGRA<Fixed8> >		ColorBGRA8i;
typedef gl::Color4< gl::ColorABGR<Fixed8> >		ColorABGR8i;
typedef gl::Color3< ColorYCbCr<Fixed8> >		ColorYCbCr8i;
typedef gl::Color4< ColorCMYK<Fixed8> >			ColorCMYK8i;
typedef gl::Color4< ColorYCCK<Fixed8> >			ColorYCCK8i;

typedef gl::Color1< Fixed16 >					ColorMono16i;
typedef gl::Color3< gl::ColorRGB<Fixed16> >		ColorRGB16i;
typedef gl::Color3< gl::ColorBGR<Fixed16> >		ColorBGR16i;
typedef gl::Color4< gl::ColorRGBA<Fixed16> >	ColorRGBA16i;
typedef gl::Color4< gl::ColorARGB<Fixed16> >	ColorARGB16i;
typedef gl::Color4< gl::ColorBGRA<Fixed16> >	ColorBGRA16i;
typedef gl::Color4< gl::ColorABGR<Fixed16> >	ColorABGR16i;
typedef gl::Color3< ColorYCbCr<Fixed16> >		ColorYCbCr16i;
typedef gl::Color4< ColorCMYK<Fixed16> >		ColorCMYK16i;
typedef gl::Color4< ColorYCCK<Fixed16> >		ColorYCCK16i;

typedef gl::Color1< Fixed32 >					ColorMono32i;
typedef gl::Color3< gl::ColorRGB<Fixed32> >		ColorRGB32i;
typedef gl::Color3< gl::ColorBGR<Fixed32> >		ColorBGR32i;
typedef gl::Color4< gl::ColorRGBA<Fixed32> >	ColorRGBA32i;
typedef gl::Color4< gl::ColorARGB<Fixed32> >	ColorARGB32i;
typedef gl::Color4< gl::ColorBGRA<Fixed32> >	ColorBGRA32i;
typedef gl::Color4< gl::ColorABGR<Fixed32> >	ColorABGR32i;
typedef gl::Color3< ColorYCbCr<Fixed32> >		ColorYCbCr32i;
typedef gl::Color4< ColorCMYK<Fixed32> >		ColorCMYK32i;
typedef gl::Color4< ColorYCCK<Fixed32> >		ColorYCCK32i;

typedef gl::Color1< double >					ColorMono64f;
typedef gl::Color3< gl::ColorRGB<double> >		ColorRGB64f;
typedef gl::Color3< gl::ColorBGR<double> >		ColorBGR64f;
typedef gl::Color4< gl::ColorRGBA<double> >		ColorRGBA64f;
typedef gl::Color4< gl::ColorARGB<double> >		ColorARGB64f;
typedef gl::Color4< gl::ColorBGRA<double> >		ColorBGRA64f;
typedef gl::Color4< gl::ColorABGR<double> >		ColorABGR64f;
typedef gl::Color3< ColorYCbCr<double> >		ColorYCbCr64f;
typedef gl::Color4< ColorCMYK<double> >			ColorCMYK64f;
typedef gl::Color4< ColorYCCK<double>	>		ColorYCCK64f;

// typedef gl::Color3< ColorXYZ<float> >			ColorXYZ32f;

namespace ImageIO {

template <typename SrcColorT, typename TargetColorT>
class LineColorConverter : public ILineColorConverter
{
public:
	LineColorConverter(size_t bitsPerSample, size_t lineWidth)
		:
		bitsPerSample_(bitsPerSample),
		lineWidth_(lineWidth)
	{
	}
	
	template <typename ValueT>
	void Convert(const gl::Color1<ValueT>& from, TargetColorT& to)
	{
		to.r = TargetColorT::value_type(from.a);
		to.g = TargetColorT::value_type(from.a);
		to.b = TargetColorT::value_type(from.a);
	}
	
	template <typename ValueT>
	void Convert(const gl::Color3<gl::ColorRGB<ValueT> >& from, TargetColorT& to)
	{
		to = from;
	}
	
	template <typename ValueT>
	void Convert(const gl::Color3<gl::ColorBGR<ValueT> >& from, TargetColorT& to)
	{
		to = from;
	}
	
	template <typename ValueT>
	void Convert(const gl::Color4<gl::ColorRGBA<ValueT> >& from, TargetColorT& to)
	{
		to = from;
	}
	
	template <typename ValueT>
	void Convert(const gl::Color4<gl::ColorARGB<ValueT> >& from, TargetColorT& to)
	{
		to = from;
	}
	
	template <typename ValueT>
	void Convert(const gl::Color4<gl::ColorBGRA<ValueT> >& from, TargetColorT& to)
	{
		to = from;
	}
	
	template <typename ValueT>
	void Convert(const gl::Color4<gl::ColorABGR<ValueT> >& from, TargetColorT& to)
	{
		to = from;
	}
	
	// http://anipeg.yks.ne.jp/color.html
	template <typename ValueT>
	void Convert(const gl::Color3<ColorYCbCr<ValueT> >& from, TargetColorT& to)
	{
		double y = from.y;
		double cr = double(from.cr) - 0.5;
		double cb = double(from.cb) - 0.5;
		double r = std::max(0.0, y + 1.402 * cr);
		double g = std::max(0.0, y - 0.34414 * cb - 0.71414 * cr);
		double b = std::max(0.0, y + 1.772 * cb);
		to.r = r;
		to.g = g;
		to.b = b;
	}
	
	template <typename ValueT>
	void Convert(const gl::Color4<ColorCMYK<ValueT> >& from, TargetColorT& to)
	{
		static const SrcColorT::value_type one = gl::OneMinusEpsilon(SrcColorT::value_type(0));
		to.r = TargetColorT::value_type(one - std::min(one, from.c * (one - from.k) + from.k));
		to.g = TargetColorT::value_type(one - std::min(one, from.m * (one - from.k) + from.k));
		to.b = TargetColorT::value_type(one - std::min(one, from.y * (one - from.k) + from.k));
	}

	template <typename ValueT>
	void Convert(const gl::Color3<ColorXYZ<ValueT> >& from, TargetColorT& to)
	{
		/*
		static const SrcColorT::value_type one = gl::OneMinusEpsilon(SrcColorT::value_type(0));
		to.r = TargetColorT::value_type(one - min(one, from.c * (one - from.k) + from.k));
		to.g = TargetColorT::value_type(one - min(one, from.m * (one - from.k) + from.k));
		to.b = TargetColorT::value_type(one - min(one, from.y * (one - from.k) + from.k));
		*/
	}

	template <typename ValueT>
	void Convert(const gl::Color4<ColorYCCK<ValueT> >& from, TargetColorT& to)
	{
		
	}
	
	bool Convert(const char* src, char* target)
	{
		const SrcColorT* srcBuff = (const SrcColorT*) src;
		TargetColorT* targetBuff = (TargetColorT*) target;
		for (int i=lineWidth_-1; i>=0; --i) {
			Convert(srcBuff[i], targetBuff[i]);
		}
		return true;
	}

private:
	size_t bitsPerSample_;
	size_t lineWidth_;
};

template <typename TargetColorT>
ILineColorConverter* CreateLineColorConverter(ColorFormat srcColorFormat, size_t bitsPerSample, size_t width)
{
	if (bitsPerSample == 8) {
		switch (srcColorFormat) {
		case ColorFormat_Mono:	return new LineColorConverter<ColorMono8i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_RGB:	return new LineColorConverter<ColorRGB8i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_BGR:	return new LineColorConverter<ColorBGR8i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_RGBA:	return new LineColorConverter<ColorRGBA8i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_ARGB:	return new LineColorConverter<ColorARGB8i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_BGRA:	return new LineColorConverter<ColorBGRA8i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_ABGR:	return new LineColorConverter<ColorABGR8i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_YCbCr:	return new LineColorConverter<ColorYCbCr8i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_CMYK:	return new LineColorConverter<ColorCMYK8i,	TargetColorT>(bitsPerSample, width);
		}
	}else if (bitsPerSample == 16) {
		switch (srcColorFormat) {
		case ColorFormat_Mono:	return new LineColorConverter<ColorMono16i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_RGB:	return new LineColorConverter<ColorRGB16i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_BGR:	return new LineColorConverter<ColorBGR16i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_RGBA:	return new LineColorConverter<ColorRGBA16i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_ARGB:	return new LineColorConverter<ColorARGB16i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_BGRA:	return new LineColorConverter<ColorBGRA16i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_ABGR:	return new LineColorConverter<ColorABGR16i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_YCbCr:	return new LineColorConverter<ColorYCbCr16i,TargetColorT>(bitsPerSample, width);
		case ColorFormat_CMYK:	return new LineColorConverter<ColorCMYK16i,	TargetColorT>(bitsPerSample, width);
		}
	}else if (bitsPerSample == 32) {
		switch (srcColorFormat) {
		case ColorFormat_Mono:	return new LineColorConverter<ColorMono32i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_RGB:	return new LineColorConverter<ColorRGB32i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_BGR:	return new LineColorConverter<ColorBGR32i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_RGBA:	return new LineColorConverter<ColorRGBA32i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_ARGB:	return new LineColorConverter<ColorARGB32i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_BGRA:	return new LineColorConverter<ColorBGRA32i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_ABGR:	return new LineColorConverter<ColorABGR32i,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_YCbCr:	return new LineColorConverter<ColorYCbCr32i,TargetColorT>(bitsPerSample, width);
		case ColorFormat_CMYK:	return new LineColorConverter<ColorCMYK32i,	TargetColorT>(bitsPerSample, width);
		}
	}else if (bitsPerSample == 64) {
		switch (srcColorFormat) {
		case ColorFormat_Mono:	return new LineColorConverter<ColorMono64f,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_RGB:	return new LineColorConverter<ColorRGB64f,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_BGR:	return new LineColorConverter<ColorBGR64f,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_RGBA:	return new LineColorConverter<ColorRGBA64f,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_ARGB:	return new LineColorConverter<ColorARGB64f,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_BGRA:	return new LineColorConverter<ColorBGRA64f,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_ABGR:	return new LineColorConverter<ColorABGR64f,	TargetColorT>(bitsPerSample, width);
		case ColorFormat_YCbCr:	return new LineColorConverter<ColorYCbCr64f,TargetColorT>(bitsPerSample, width);
		case ColorFormat_CMYK:	return new LineColorConverter<ColorCMYK64f,	TargetColorT>(bitsPerSample, width);
		}
	}
	return NULL;
}

ILineColorConverter* CreateLineColorConverter(const ImageInfo& srcImageInfo, const ImageInfo& targetImageInfo, std::string& errorMessage)
{
	// source format check
	const size_t srcBitsPerSample = srcImageInfo.bitsPerSample;
	const ColorFormat srcColorFormat = srcImageInfo.colorFormat;
	const size_t srcWidth = srcImageInfo.width;
	if (srcBitsPerSample != 8 && srcBitsPerSample != 16 && srcBitsPerSample != 32 && srcBitsPerSample != 64) {
		errorMessage = "source image's bits per sample must be 8 or 16 or 32 or 64.";
		return NULL;
	}
	if (srcBitsPerSample > 64) {
		errorMessage = "source image's bits per sample exceeds 64.";
		return NULL;
	}
	
	// target format check
	if (targetImageInfo.samplesPerPixel != 3 && targetImageInfo.samplesPerPixel != 4) {
		errorMessage = "target image's samples per pixel must be 3 or 4";
		return NULL;
	}
	if (targetImageInfo.bitsPerSample != 8) {
		errorMessage = "target image's bits per sample must be 8.";
		return NULL;
	}
	if (targetImageInfo.colorFormat != ColorFormat_BGR && targetImageInfo.colorFormat != ColorFormat_BGRA) {
		errorMessage = "target image's color format must be BGR.";
		return NULL;
	}
	switch (targetImageInfo.colorFormat) {
//	case ColorFormat_Mono:	return CreateLineColorConverter<ColorMono8i>(srcColorFormat, srcBitsPerSample, srcWidth);
//	case ColorFormat_RGB:	return CreateLineColorConverter<ColorRGB8i>(srcColorFormat, srcBitsPerSample, srcWidth);
	case ColorFormat_BGR:	return CreateLineColorConverter<ColorBGR8i>(srcColorFormat, srcBitsPerSample, srcWidth);
//	case ColorFormat_RGBA:	return CreateLineColorConverter<ColorRGBA8i>(srcColorFormat, srcBitsPerSample, srcWidth);
//	case ColorFormat_ARGB:	return CreateLineColorConverter<ColorARGB8i>(srcColorFormat, srcBitsPerSample, srcWidth);
	case ColorFormat_BGRA:	return CreateLineColorConverter<ColorBGRA8i>(srcColorFormat, srcBitsPerSample, srcWidth);
//	case ColorFormat_ABGR:	return CreateLineColorConverter<ColorABGR8i>(srcColorFormat, srcBitsPerSample, srcWidth);
	}
	return NULL;
}

} // namespace ImageIO

	