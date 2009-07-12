#pragma once

/*
	�F�������킹����
*/

namespace gl
{

//! �����܂���
template <typename ColorT, typename NumericT>
struct ColorUnblender
{
	ColorT operator() (ColorT foreColor, ColorT backColor)
	{
		return foreColor;
	}

	ColorT operator() (ColorT foreColor, NumericT foreColorAlpha, ColorT backColor)
	{
		foreColor.a = ColorT::value_type(foreColorAlpha);
		return foreColor;
	}
};

//! �h��d�˂�
template <typename ColorT, typename NumericT>
struct PaintColorBlender
{
	ColorT operator() (ColorT foreColor, ColorT backColor)
	{
		// ���ꂼ��̐F�̕s�����x���l�����č�����B
		NumericT bcFactor = OneMinusEpsilon(NumericT(1)) - foreColor.a;
		NumericT foreAlpha = NumericT(foreColor.a);
		ColorT ret(
			(foreColor.r * foreAlpha + backColor.r * bcFactor),
			(foreColor.g * foreAlpha + backColor.g * bcFactor),
			(foreColor.b * foreAlpha + backColor.b * bcFactor),
			gl::min(foreAlpha + backColor.a, OneMinusEpsilon(NumericT(1)))
		);
		return ret;
	}

	
	ColorT operator() (ColorT foreColor, NumericT foreColorAlpha, ColorT backColor)
	{
		// ���ꂼ��̐F�̕s�����x���l�����č�����B
		NumericT bcFactor = OneMinusEpsilon(NumericT(1)) - foreColorAlpha;
		ColorT ret(
			NumericT(foreColor.r) * foreColorAlpha + NumericT(backColor.r) * bcFactor,
			NumericT(foreColor.g) * foreColorAlpha + NumericT(backColor.g) * bcFactor,
			NumericT(foreColor.b) * foreColorAlpha + NumericT(backColor.b) * bcFactor,
			gl::min((foreColorAlpha + NumericT(backColor.a)), OneMinusEpsilon(NumericT(1)))
		);
		return ret;
	}

};

template <typename Color1T, typename NumericT>
struct PaintColor1Blender
{
	Color1T operator() (Color1T foreColor, Color1T backColor)
	{
		// ���ꂼ��̐F�̕s�����x���l�����č�����B
		Color1T::value_type bcFactor = OneMinusEpsilon(Color1T::value_type(1)) - foreColor.a;
		Color1T ret(
			(foreColor.a + backColor.a * bcFactor)
		);
		return ret;
	}

	Color1T operator() (Color1T foreColor, NumericT foreColorAlpha, Color1T backColor)
	{
		// ���ꂼ��̐F�̕s�����x���l�����č�����B
		NumericT bcFactor = OneMinusEpsilon(NumericT(1)) - foreColorAlpha;
		Color1T ret(
			gl::min(foreColorAlpha + backColor.a * bcFactor, OneMinusEpsilon(NumericT(1)))
		);
		return ret;
	}
};

//! additive blend
template <typename ColorT, typename NumericT>
struct AdditiveColorBlender
{
	ColorT operator() (ColorT foreColor, NumericT foreColorAlpha, ColorT backColor)
	{
		// ���ꂼ��̐F�̕s�����x���l�����č�����B
		ColorT ret(
			gl::min<NumericT>(NumericT(foreColor.r) * foreColorAlpha + NumericT(backColor.r), OneMinusEpsilon(NumericT(1))),
			gl::min<NumericT>(NumericT(foreColor.g) * foreColorAlpha + NumericT(backColor.g), OneMinusEpsilon(NumericT(1))),
			gl::min<NumericT>(NumericT(foreColor.b) * foreColorAlpha + NumericT(backColor.b), OneMinusEpsilon(NumericT(1))),
			gl::min<NumericT>(foreColorAlpha + NumericT(backColor.a), OneMinusEpsilon(NumericT(1)))
		);
		return ret;
	}
};

//! additive blend
template <typename Color1T, typename NumericT>
struct AdditiveColor1Blender
{
	Color1T operator() (Color1T foreColor, NumericT foreColorAlpha, Color1T backColor)
	{
		return Color1T(
			gl::min(foreColorAlpha + NumericT(backColor.a), OneMinusEpsilon(NumericT(0)))
		);
		
	}
};

// ���ʂɐF��������
template <typename ColorT>
struct NormalColorBlender
{
	ColorT operator() (ColorT a, ColorT b)
	{
		// ���ꂼ��̐F�̕s�����x���l�����č�����B
		T sum = a.a + b.a;
		T percentage1 = a.a / sum;
		T percentage2 = b.a / sum;
		
		Color ret(
			( a.r * percentage1 + b.r * percentage2 ),
			( a.g * percentage1 + b.g * percentage2 ),
			( a.b * percentage1 + b.b * percentage2 ),
			sum
		);
		return ret;
	}
};

}	// namespace gl
