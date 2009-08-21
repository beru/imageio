#pragma once

/*
	色変換処理
*/

#include "color.h"

namespace gl
{

template <typename TargetColorT>
struct ColorConverter
{
	template <typename SrcBaseColorT, typename TargetColorT>
	__forceinline void operator () (gl::Color4<SrcBaseColorT> from, TargetColorT& to)
	{
		to = TargetColorT(
			from.r,
			from.g,
			from.b,
			from.a
		);
	}

	template <typename SrcBaseColorT>
	__forceinline void operator () (gl::Color3<SrcBaseColorT> from, TargetColorT& to)
	{
		to = TargetColorT(
			from.r,
			from.g,
			from.b
		);
	}

	template <typename SrcComponentT>
	__forceinline void operator () (gl::Color1<SrcComponentT> from, TargetColorT& to)
	{
		to = TargetColorT(from.a);
	}

	template <typename T>
	__forceinline TargetColorT operator () (T from)
	{
		return TargetColorT(from);
	}

};

template <typename NumericT, typename ValueT, size_t tableSize>
struct TableLookupHelper
{
	TableLookupHelper(ValueT* table)
		:
		table_(table)
	{
	}

	__forceinline ValueT Lookup(NumericT value)
	{
#if 1
		return table_[ gl::ToInt(mul<tableSize>(value)) ];
#else
		return table_[ gl::ToInt(value * NumericT(tableSize)) ];
#endif
	}
	ValueT* table_;
};

template <typename NumericT, typename TargetColorT>
struct GammaModify_ColorConverter
{
private:
	double gamma_;
	enum {
		GAMMA_TABLE_SIZE = 1024,
	};
	TargetColorT colorTable_[GAMMA_TABLE_SIZE];
	TableLookupHelper<NumericT, TargetColorT, GAMMA_TABLE_SIZE> clut_;

public:
	GammaModify_ColorConverter()
		:
		clut_(colorTable_)
	{
	}
	
	void SetGamma(double gamma)
	{
		gamma_ = gamma;
		double invGamma = 1.0 / gamma_;
		static const double invGammaTableSize = 1.0 / GAMMA_TABLE_SIZE;
		for (size_t i=0; i<GAMMA_TABLE_SIZE; ++i) {
			double val = i * invGammaTableSize;
			double powRet = ::pow(val, invGamma);
			colorTable_[i] = TargetColorT(min(powRet, 1.0-0.005));
		}
	}
	
	template <typename ComponentT>
	__forceinline TargetColorT operator () (gl::Color1<ComponentT> srcColor)
	{
		return clut_.Lookup(srcColor.a);
	}

/*
	template <typename ColorT>
	__forceinline TargetColorT operator () (ColorT srcColor)
	{
		return TargetColorT(
			(TargetColorT::value_type) clut_.Lookup(srcColor.r),
			clut_.Lookup(srcColor.g),
			clut_.Lookup(srcColor.b),
			gl::OneMinusEpsilon(NumericT(1))
		);
	}
*/

};

template <typename NumericT, typename TargetColorT>
struct ColorConverter_PassThrough
{
public:
	ColorConverter_PassThrough()
	{
	}
	
	template <typename ComponentT>
	__forceinline TargetColorT operator () (gl::Color1<ComponentT> srcColor)
	{
		return TargetColorT(srcColor.a);
	}
};


} // namespace gl

