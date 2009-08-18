#pragma once

#include "IFile.h"

#include <stdio.h>

class File : public IFile
{
public:
	File(HANDLE hFile);

	File(FILE* pFile);

	BOOL Read(void* pBuffer, DWORD nNumberOfBytesToRead, DWORD& nNumberOfBytesRead);
	
	BOOL Write(const void* pBuffer, DWORD nNumberOfBytesToWrite, DWORD& nNumberOfBytesWritten);
	
	DWORD Seek(LONG lDistanceToMove, DWORD dwMoveMethod);
	
	DWORD Tell() const;
	
	DWORD Size() const;
	
	BOOL Flush();
	
	bool HasBuffer() const { return false; }
	virtual const void* GetBuffer() const { return 0; }
	
private:
	HANDLE hFile_;
};

