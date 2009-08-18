#include "stdafx.h"
#include "MemoryFile.h"

MemoryFile::MemoryFile(void* pBuff, size_t buffSize)
	:
	pBuff_(pBuff),
	buffSize_(buffSize)
{
}

BOOL MemoryFile::Read(void* pBuffer, DWORD nNumberOfBytesToRead, DWORD& nNumberOfBytesRead)
{
	DWORD fileSize = Size();
	nNumberOfBytesRead = (fileSize < curPos_+nNumberOfBytesToRead) ? (fileSize - curPos_) : nNumberOfBytesToRead;
	memcpy(pBuffer, (BYTE*)pBuff_+curPos_, nNumberOfBytesRead);
	Seek(nNumberOfBytesRead, FILE_CURRENT);
	return TRUE;
}

BOOL MemoryFile::Write(const void* pBuffer, DWORD nNumberOfBytesToWrite, DWORD& nNumberOfBytesWritten)
{
	memcpy((BYTE*)pBuff_+curPos_, pBuffer, nNumberOfBytesToWrite);
	nNumberOfBytesWritten = nNumberOfBytesToWrite;
	Seek(nNumberOfBytesToWrite, FILE_CURRENT);
	return TRUE;
}

DWORD MemoryFile::Seek(LONG lDistanceToMove, DWORD dwMoveMethod)
{
	switch (dwMoveMethod) {
	case FILE_BEGIN:
		curPos_ = lDistanceToMove;
		break;
	case FILE_CURRENT:
		curPos_ += lDistanceToMove;
		break;
	case FILE_END:
		curPos_ += buffSize_ + lDistanceToMove;
		break;
	}
	assert(0 <= curPos_ && curPos_ <= buffSize_);
	return curPos_;
}

DWORD MemoryFile::Tell() const
{
	return curPos_;
}

DWORD MemoryFile::Size() const
{
	return buffSize_;
}

BOOL MemoryFile::Flush()
{
	return TRUE;
}

