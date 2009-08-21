#pragma once

HBITMAP CreateDIB(int width, int height, size_t bitCount, int& lineOffset, BITMAPINFO& bmi, void*& pBits);

