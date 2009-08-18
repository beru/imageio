#include "stdafx.h"

#include "AsyncFile.h"

#include <algorithm>

#undef min

AsyncFile::AsyncFile(HANDLE hFile)
	:
	hFile_(hFile),
	internalOffset_(0)
{
	memset(&overlapped_, 0, sizeof(OVERLAPPED));
}

BOOL AsyncFile::Read(void* pBuffer, DWORD nNumberOfBytesToRead, DWORD& nNumberOfBytesRead)
{
	DWORD nBytesTransferred = 0;
	bool loop = true;
	do {
		if (GetOverlappedResult(hFile_, &overlapped_, &nBytesTransferred, FALSE)) {
			break;
		}
		switch (DWORD err = GetLastError()) {
		case ERROR_HANDLE_EOF:
			loop = false;
			break;
		case ERROR_IO_PENDING:
			loop = (nBytesTransferred < internalOffset_ + nNumberOfBytesToRead);
			break;
		case ERROR_IO_INCOMPLETE:
			break;
		default:
			return FALSE;
		}
		AsyncRead(Size());
	}while (loop) ;
	
	nNumberOfBytesRead = std::min(nBytesTransferred - internalOffset_, nNumberOfBytesToRead);
	memcpy(pBuffer, &buff_[internalOffset_], nNumberOfBytesRead);

	internalOffset_ += nNumberOfBytesRead;
	
	return TRUE;
}

BOOL AsyncFile::Write(const void* pBuffer, DWORD nNumberOfBytesToWrite, DWORD& nNumberOfBytesWritten)
{
	return 0;
}

DWORD AsyncFile::Seek(LONG lDistanceToMove, DWORD dwMoveMethod)
{
//	CriticalSection::Lock lock(cs_);
	switch (dwMoveMethod) {
	case FILE_BEGIN:
		internalOffset_ = lDistanceToMove;
		break;
	case FILE_CURRENT:
		internalOffset_ += lDistanceToMove;
		internalOffset_ = std::min(internalOffset_, Size());
		break;
	case FILE_END:
		internalOffset_ = Size() + lDistanceToMove;
		break;
	}
//	overlapped_.Offset = internalOffset_;
	return internalOffset_;
}

DWORD AsyncFile::Tell() const
{
	return internalOffset_; //overlapped_.Offset;
}

DWORD AsyncFile::Size() const
{
	return GetFileSize(hFile_, NULL);
}

BOOL AsyncFile::Flush()
{
	return FlushFileBuffers(hFile_);
}


BOOL AsyncFile::AsyncRead(DWORD nNumberOfBytesToRead)
{
//	CriticalSection::Lock lock(cs_);
	DWORD nBytesRead = 0;
	BOOL bRet = ReadFile(hFile_, &buff_[0], nNumberOfBytesToRead, &nBytesRead, &overlapped_);
	DWORD dwError = GetLastError();
	return dwError == ERROR_IO_PENDING;
}

