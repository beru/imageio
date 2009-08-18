#pragma once

namespace gl {

// http://sunak2.cs.shinshu-u.ac.jp/~maruyama/course/graphics/ogl-glut/blending.html

enum ColorBlendingMode
{
	ColorBlendingMode_First,
	ColorBlendingMode_Replace = ColorBlendingMode_First,
	ColorBlendingMode_AlphaBlend,
	ColorBlendingMode_Additive,
//	ColorBlendingMode_Subtract,
//	ColorBlendingMode_Multiply,
	ColorBlendingMode_Last = ColorBlendingMode_Additive
};

template <typename ColorT>
class ColorBlender
{
public:
	ColorBlendingMode mode;
	
	ColorT blend(ColorT src, ColorT dst)
	{
		switch (mode) {
		case ColorBlendingMode_Replace:
			return blend_Replace(src, dst);
		case ColorBlendingMode_AlphaBlend:
			return blend_AlphaBlend(src, dst);
		case ColorBlendingMode_Additive:
			return blend_Additive(src, dst);
//		case ColorBlendingMode_Subtract:
//			return blend_Subtract(dst, src);
//		case ColorBlendingMode_Multiply:
//			return blend_Multiply(dst, src);
		}
	}

	ColorT operator() (ColorT src, ColorT dst)
	{
		return blend(src, dst);
	}

	template <typename NumericT>
	ColorT operator() (ColorT src, NumericT srcAlpha, ColorT dst)
	{
		src.a = srcAlpha;
		return blend(src, dst);
	}

	
	static ColorT blend_Replace(ColorT src, ColorT dst)
	{
		return src;
	}

	static ColorT blend_AlphaBlend(ColorT src, ColorT dst)
	{
		float a1 = src.a;
		float a2 = 1.0 - a1;
		ColorT ret(
			(float) src.r * a1 + (float) dst.r * a2,
			(float) src.g * a1 + (float) dst.g * a2,
			(float) src.b * a1 + (float) dst.b * a2,
			1.0
		);
		return ret;
	}
	
	static ColorT blend_Additive(ColorT src, ColorT dst)
	{
		float a1 = src.a;
		float a2 = dst.a;
		ColorT ret(
			(float) src.r * a1 + (float) dst.r,
			(float) src.g * a1 + (float) dst.g,
			(float) src.b * a1 + (float) dst.b,
			1.0
		);
		return ret;
	}
	
/*
	static ColorT blend_Subtract(ColorT src, ColorT dst)
	{
		return dst - src;
	}
	
	static ColorT blend_Multiply(ColorT src, ColorT dst)
	{
		return dst * src;
	}
*/
	
};

} // namespace gl

