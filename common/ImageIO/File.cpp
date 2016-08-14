#include "stdafx.h"
#include "File.h"

#include <io.h>
#include <assert.h>
#include <memory.h>
#include <stdio.h>

#define NOMINMAX
#include <windows.h>

File::File(HANDLE hFile)
	:
	hFile_(hFile)
{
}

File::File(FILE* pFile)
{
	hFile_ = (HANDLE) _get_osfhandle(_fileno(pFile));
}

BOOL File::Read(void* pBuffer, DWORD nNumberOfBytesToRead, DWORD& nNumberOfBytesRead)
{
	return ReadFile(hFile_, pBuffer, nNumberOfBytesToRead, &nNumberOfBytesRead, NULL);
}

BOOL File::Write(const void* pBuffer, DWORD nNumberOfBytesToWrite, DWORD& nNumberOfBytesWritten)
{
	return WriteFile(hFile_, pBuffer, nNumberOfBytesToWrite, &nNumberOfBytesWritten, NULL);
}

DWORD File::Seek(LONG lDistanceToMove, DWORD dwMoveMethod)
{
	return SetFilePointer(hFile_, lDistanceToMove, NULL, dwMoveMethod);
}

DWORD File::Tell() const
{
	/*
		-- Reference --
		http://nukz.net/reference/fileio/hh/winbase/filesio_3vhu.htm
	*/
	return SetFilePointer(
		hFile_, // must have GENERIC_READ and/or GENERIC_WRITE 
		0,     // do not move pointer 
		NULL,  // hFile is not large enough to need this pointer 
		FILE_CURRENT
	);  // provides offset from current position 
}

DWORD File::Size() const
{
	return GetFileSize(hFile_, NULL);
}

BOOL File::Flush()
{
	return FlushFileBuffers(hFile_);
}

