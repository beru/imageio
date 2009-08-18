#pragma once
#include <windows.h>

/*!
	@file   Timer.h
	@brief  ŽžŠÔŒv‘ª—p‚ÌClass
*/

class Timer
{
private:
	LARGE_INTEGER start_;

public:
	Timer()
	{
		Start();
	}

	void Start()
	{
		QueryPerformanceCounter(&start_);
	}
	
	__int64 Elapsed()
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return now.QuadPart - start_.QuadPart;
	}
	
};

