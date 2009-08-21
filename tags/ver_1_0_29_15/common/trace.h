#pragma once

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#if defined(_DEBUG) || defined(DEBUG)

class DbgStr
{
public:
	void operator() ( LPCTSTR pszFormat, ...)
	{
		va_list	argp;
		static TCHAR pszBuf[10240];
		va_start(argp, pszFormat);
		_vstprintf( pszBuf, pszFormat, argp);
		va_end(argp);
		OutputDebugString( pszBuf);
	}

};

#define TRACE     DbgStr()

#else
#define TRACE

#endif

