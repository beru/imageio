#pragma once

#include <windows.h>

/*!

�T�v
	�摜�̓��o�͗p�ɉ��z�I��File��������悤�ɂ���
	
����
	�񓯊�����ɂ͑Ή����Ȃ�
	2GB�܂�
	
���l
	CxImage by Davide Pizzolato (http://www.xdp.it/cximage.htm)	���Q�l�ɂ��܂����B

	Interface��WindowsAPI��File�֐��Ɏ����Ă��܂�

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

