#pragma once

#include "IFile.h"

class MemoryFile : public IFile
{
public:
	MemoryFile(void* pBuff, size_t buffSize);

	BOOL Read(void* pBuffer, DWORD nNumberOfBytesToRead, DWORD& nNumberOfBytesRead);
	
	BOOL Write(const void* pBuffer, DWORD nNumberOfBytesToWrite, DWORD& nNumberOfBytesWritten);
	
	DWORD Seek(LONG lDistanceToMove, DWORD dwMoveMethod);
	
	DWORD Tell() const;
	
	DWORD Size() const;
	
	BOOL Flush();

	bool HasBuffer() const { return true; }
	virtual const void* GetBuffer() const { return pBuff_; }
	
private:
	void* pBuff_;
	DWORD buffSize_;
	
	DWORD curPos_;
};

