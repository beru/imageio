#pragma once

#include <windows.h>

/*!

概要
	画像の入出力用に仮想的にFileを扱えるようにする
	
制限
	非同期操作には対応しない
	2GBまで
	
備考
	CxImage by Davide Pizzolato (http://www.xdp.it/cximage.htm)	を参考にしました。

	InterfaceをWindowsAPIのFile関数に似せています

*/

class IFile
{
public:
	virtual BOOL Read(void* pBuffer, DWORD nNumberOfBytesToRead, DWORD& nNumberOfBytesRead) = 0;
	virtual BOOL Write(const void* pBuffer, DWORD nNumberOfBytesToWrite, DWORD& nNumberOfBytesWritten) = 0;
	virtual DWORD Seek(LONG lDistanceToMove, DWORD dwMoveMethod) = 0;
	virtual DWORD Tell() const = 0;
	virtual DWORD Size() const = 0;
	virtual bool IsEof() const { return Tell() == Size(); }
	virtual BOOL Flush() = 0;

	virtual bool HasBuffer() const = 0;
	virtual const void* GetBuffer() const = 0;
};

