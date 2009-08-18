#include "stdafx.h"
#include "MathUtil.h"

double CalcFitScale(double width1, double height1, double width2, double height2)
{
	double scale = width2 / width1;
	if (height1 * scale > height2)
		scale = height2 / height1;
	return scale;
}

double gaussian(double x, double invDispersion)
{
	return exp(-0.5 * (x*x) * invDispersion);
}

void gaussianArray(double* pArray, size_t count, double n, double halfBandWidth)
{
	double invN = 1 / n;
	double invDispersion = 1 / (halfBandWidth * halfBandWidth);
	int halfCount = count / 2;
	for (int i=0; i<(int)count; ++i)
	{
		pArray[i] = invN * gaussian(i-halfCount, invDispersion);
	}
}

double fround(double val, size_t decimal)
{
	if (val == 0)
		return 0.0;
	double factor = pow(0.1, (double)decimal);
	double num = factor * 5;
	double tmp = val + ((val > 0) ? +num : -num);
	return factor * ((int)tmp / factor);
}

double LimitDecimalPlace(double val, size_t decimalPlace)
{
	double factor = pow(10.0, (double)decimalPlace);
	LONGLONG integralPart = LONGLONG(LONGLONG(val) * factor);
	LONGLONG allPart = LONGLONG(val * factor);
	LONGLONG fractionalPart = allPart - integralPart;

	double integral = double(LONGLONG(val));
	double fractional = (fractionalPart / factor);
	return integral + fractional;
}

// http://jeanne.wankuma.com/tips/math/rounddown.html
double roundDown(double val, size_t digits)
{
	double factor = pow(10.0, (double)digits);
	double ret = (val > 0) ? (floor(val*factor) / factor) : (ceil(val*factor) / factor);
	return ret;
}
