#pragma once

#include "IFile.h"
#include "CriticalSection.h"

#include <vector>

class AsyncFile : public IFile
{
public:
	AsyncFile(HANDLE hFile);
	
	BOOL Read(void* pBuffer, DWORD nNumberOfBytesToRead, DWORD& nNumberOfBytesRead);
	
	BOOL Write(const void* pBuffer, DWORD nNumberOfBytesToWrite, DWORD& nNumberOfBytesWritten);
	
	DWORD Seek(LONG lDistanceToMove, DWORD dwMoveMethod);
	
	DWORD Tell() const;
	
	DWORD Size() const;
	
	BOOL Flush();
	
	BOOL AsyncRead(DWORD nNumberOfBytesToRead);

	bool HasBuffer() const { return true; }
	virtual const void* GetBuffer() const { return buff_; }

	unsigned char* buff_;
private:
	HANDLE hFile_;
	OVERLAPPED overlapped_;
	CriticalSection cs_;

	DWORD internalOffset_;
};

