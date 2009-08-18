#include "PathUtility.h"

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#ifdef __AFX_

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#endif

namespace PathUtility  
{

CString FindExtension(const TCHAR* filePath)
{
	return PathFindExtension(filePath);
}

CString ChangeExtension(const TCHAR *filename, const TCHAR *newExtension)
{
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	_tsplitpath(filename, drive, dir, fname, ext);
	return MakePath(drive, dir, fname, newExtension);
}

CString AppendPath(const CString& leftPath, const CString& rightPath, bool dirCheck)
{
	CString tmp = PathUtility::AddBackslash(leftPath);
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	_tsplitpath(tmp, drive, dir, fname, ext);
	TCHAR path_buffer[_MAX_PATH+1];
	_tmakepath(path_buffer, drive, dir, NULL, NULL);
	PathAppend(path_buffer, rightPath);
	if (dirCheck && PathIsDirectory(path_buffer))
	{
		PathAddBackslash(path_buffer);
	}
	return path_buffer;
}

CString GetPathFromFile(const CString& fullPath)
{
	CString path;
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];
	_tsplitpath(fullPath, drive, dir, fname, ext);
	path = drive;
	path += dir;
	return path;
}

bool IsReadOnly(const CString& strPath)
{
	DWORD fa = GetFileAttributes( strPath );
	return (
		fa != 0xffffffff
		&& (fa & FILE_ATTRIBUTE_READONLY)
	);
}

#ifdef __AFX__

//! 読み取り専用ファイルの属性のReset
void ResetFileReadOnly(const CString& strPath)
{
	CFileStatus fileStatus, &s = fileStatus;
	CFile::GetStatus(strPath, fileStatus);				
	fileStatus.m_attribute = fileStatus.m_attribute & ~0x01;
	CFile::SetStatus(strPath,s);
}

#endif

//! Directoryの有効性を判断する
bool IsDirectory(const CString& strPath)
{
	DWORD fa = GetFileAttributes( strPath );
	return (
		fa != 0xffffffff 
		&& (fa & FILE_ATTRIBUTE_DIRECTORY)
	);
}

bool IsReadOnlyDirectory(const CString& path)
{
	DWORD fa = GetFileAttributes( path );
	return (
		fa != 0xffffffff 
		&& (fa & FILE_ATTRIBUTE_DIRECTORY)
		&& (fa & FILE_ATTRIBUTE_READONLY)
	);
}

bool IsWritableDirectory(const CString& path)
{
	if (GetDriveType(path) == DRIVE_CDROM)
		return FALSE;
	DWORD fa = GetFileAttributes( path );
	return (
		fa != 0xffffffff 
		&& (fa & FILE_ATTRIBUTE_DIRECTORY)
		&& !(fa & FILE_ATTRIBUTE_READONLY)
	);
}

CString GetValidPath(const CString& strPath)
{
	CString tmpPath = "";
	if (strPath.GetLength() > 0)
	{
		if (strPath[strPath.GetLength()-1] == '\\')
			tmpPath = strPath.Left(strPath.GetLength()-1);
	}
	return tmpPath;
}

CString MakePath(const TCHAR* drive, const TCHAR* dir, const TCHAR* fname, const TCHAR* ext)
{
	size_t len = 0;
	if (drive)	len += _tcslen(drive);
	if (dir)	len += _tcslen(dir);
	if (fname)	len += _tcslen(fname);
	if (ext)	len += _tcslen(ext);
	len *= 2;
	TCHAR* buff = new TCHAR[len];
	_tmakepath(buff, drive, dir, fname, ext);
	CString ret = buff;
	delete buff;
	return ret;
}

CString AddBackslash(const CString& path)
{
	TCHAR* buff = new TCHAR[path.GetLength()+8];
	const TCHAR* ptr = path;
	_tcscpy(buff, ptr);
	PathAddBackslash(buff);
	CString ret = buff;
	delete buff;
	return ret;
}

CString RemoveBackslash(const CString& path)
{
	TCHAR* buff = new TCHAR[path.GetLength()+8];
	const TCHAR* ptr = path;
	_tcscpy(buff, ptr);
	PathRemoveBackslash(buff);
	CString ret = buff;
	delete buff;
	return ret;
}

bool IsRoot(const CString& path)
{
	return PathIsRoot(path) != FALSE;
}

CString MakeFolderChangedPath(const TCHAR* oldPath, const TCHAR* folderPath)
{
	TCHAR fileBuf[_MAX_FNAME];
	TCHAR extBuf[_MAX_FNAME];
	_tsplitpath(oldPath, NULL, NULL, fileBuf, extBuf);
	return MakePath(NULL, folderPath, fileBuf, extBuf);
}

CString GetModuleFileFolderPath()
{
	CString ret;
	TCHAR szPath[_MAX_PATH+1] = {0};
	TCHAR szDrive[_MAX_DRIVE] = {0};
	TCHAR szDir[_MAX_DIR] = {0};

	// 実行モジュールのフルパスの取得
	DWORD dwRet = ::GetModuleFileName(NULL, szPath, sizeof(szPath));
	if (dwRet)
	{
		// フルパスを分解
		_tsplitpath(szPath, szDrive, szDir, NULL, NULL);
		// szPathに実行モジュールのパスを作成
		ret += szDrive;
		ret += szDir;
	}
	return ret;
}

bool IsSamePath(LPCTSTR lhs, LPCTSTR rhs)
{
	CString leftStr = RemoveBackslash(lhs);
	CString rightStr = RemoveBackslash(rhs);
	return Canonicalize(leftStr).CompareNoCase(Canonicalize(rightStr)) == 0;
}

CString Canonicalize(LPCTSTR path)
{
	TCHAR buff[_MAX_PATH+1];
	PathCanonicalize(buff, path);
	return buff;
}

CString GetFileNameWithExtension(const TCHAR* filePath)
{
	TCHAR fileBuf[_MAX_FNAME];
	TCHAR extBuf[_MAX_FNAME];
	_tsplitpath(filePath, NULL, NULL, fileBuf, extBuf);
	return MakePath(NULL, NULL, fileBuf, extBuf);
}

bool IsFileExist(const TCHAR* path)
{
	WIN32_FIND_DATA findData;
	HANDLE hResult = ::FindFirstFile(path, &findData);
	bool ret = hResult != INVALID_HANDLE_VALUE;
	::FindClose(hResult);
	return ret;
}

CString GetTempPath()
{
	TCHAR buff[_MAX_PATH+1];
	::GetTempPath(_MAX_PATH, buff);
	return buff;
}

CString FindFileName(const CString& path)
{
	return PathFindFileName(path);
}

CString RemoveFileSpec(const CString& path)
{
	CString work = path;
	TCHAR* ptr = work.GetBuffer(0);
	PathRemoveFileSpec(ptr);
	CString ret = ptr;
	work.ReleaseBuffer();
	return ret;
}

#if 0
static const int CSIDL_FOLDERS[] = { 
	CSIDL_FLAG_CREATE,
	CSIDL_ADMINTOOLS,
	CSIDL_ALTSTARTUP,
	CSIDL_APPDATA,
	CSIDL_BITBUCKET,
	CSIDL_CDBURN_AREA,
	CSIDL_COMMON_ADMINTOOLS,
	CSIDL_COMMON_ALTSTARTUP,
	CSIDL_COMMON_APPDATA,
	CSIDL_COMMON_DESKTOPDIRECTORY,
	CSIDL_COMMON_DOCUMENTS,
	CSIDL_COMMON_FAVORITES,
	CSIDL_COMMON_MUSIC,
	CSIDL_COMMON_PICTURES,
	CSIDL_COMMON_PROGRAMS,
	CSIDL_COMMON_STARTMENU,
	CSIDL_COMMON_STARTUP,
	CSIDL_COMMON_TEMPLATES,
	CSIDL_COMMON_VIDEO,
	CSIDL_CONTROLS,
	CSIDL_COOKIES,
	CSIDL_DESKTOP,
	CSIDL_DESKTOPDIRECTORY,
	CSIDL_DRIVES,
	CSIDL_FAVORITES,
	CSIDL_FONTS,
	CSIDL_HISTORY,
	CSIDL_INTERNET,
	CSIDL_INTERNET_CACHE,
	CSIDL_LOCAL_APPDATA,
	CSIDL_MYDOCUMENTS,
	CSIDL_MYMUSIC,
	CSIDL_MYPICTURES,
	CSIDL_MYVIDEO,
	CSIDL_NETHOOD,
	CSIDL_NETWORK,
	CSIDL_PERSONAL,
	CSIDL_PRINTERS,
	CSIDL_PRINTHOOD,
	CSIDL_PROFILE,
//	CSIDL_PROFILES,
	CSIDL_PROGRAM_FILES,
	CSIDL_PROGRAM_FILES_COMMON,
	CSIDL_PROGRAMS,
	CSIDL_RECENT,
	CSIDL_SENDTO,
	CSIDL_STARTMENU,
	CSIDL_STARTUP,
	CSIDL_SYSTEM,
	CSIDL_TEMPLATES,
	CSIDL_WINDOWS,
};
/*
 * @ret 特別なフォルダかどうか
 */
bool IsSpecialFolder(const CString& folderPath)
{
	TCHAR buff[_MAX_PATH+1];
	CString path = RemoveBackslash(folderPath);
	CString str;
	const int arrSize = (sizeof(CSIDL_FOLDERS)/sizeof(CSIDL_FOLDERS[0]));
	for (size_t i=0; i<arrSize; ++i)
	{
		HRESULT ret = SHGetFolderPath(NULL, CSIDL_FOLDERS[i], NULL, SHGFP_TYPE_CURRENT, buff);
		if (ret == S_OK)
		{
			str = buff;
			// 含まれていたら、親のpathも変更できないようにする
			if (str.Find(path, 0) != -1)
				return true;
		}
	}
	return false;
}
#endif

CString FindLastComponent(const CString& path)
{
	CString str = RemoveBackslash(path);
	const TCHAR* ptr = str;
	for (;;)//~~~
	{
		const TCHAR* tmpPtr = PathFindNextComponent(ptr);
		if (CString(tmpPtr).IsEmpty())
			return ptr;
		else
			ptr = tmpPtr;
	}
	return "";
}


bool HasSubFolder(LPCTSTR folderPath)
{
	WIN32_FIND_DATA wfd;
	HANDLE hFind;
	TCHAR pathBuff[_MAX_PATH] = {0};
	_tcscpy(pathBuff, folderPath);
	PathAddBackslash(pathBuff);
	_tcscat(pathBuff, _T("*.*"));
	hFind = FindFirstFile(pathBuff, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;
	bool folderFound = false;
	do {
		if (
			_tcscmp(wfd.cFileName, _T(".")) == 0
			|| _tcscmp(wfd.cFileName, _T("..")) == 0
		)
			continue;
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			folderFound = true;
			break;
		}
	} while (FindNextFile(hFind, &wfd));
	FindClose(hFind);
	return folderFound;
}

double GetDiskFreeSpace(LPCTSTR szDrive)
{
	double dVolume;
	ULARGE_INTEGER i64FreeBytesToCaller; // bytes available to caller
	ULARGE_INTEGER i64TotalBytes; // bytes on disk
	ULARGE_INTEGER i64FreeBytes;
	if (!GetDiskFreeSpaceEx(szDrive, &i64FreeBytesToCaller, &i64TotalBytes, &i64FreeBytes))
		return -1.0;
	dVolume = (double)(signed __int64)i64FreeBytes.QuadPart;
	return dVolume;
}

bool IsUNCServerShare(LPCTSTR path)
{
	bool ret = PathIsUNCServerShare(RemoveBackslash(path)) != FALSE;
	return ret;
}

bool IsModifiableFolder(LPCTSTR path)
{
	return !(
		0
		|| IsRoot(path)
//		|| IsSpecialFolder(path)
		|| !IsWritableDirectory(path)
		|| IsUNCServerShare(path)
	);

}

bool IsRelative(LPCTSTR path)
{
	return PathIsRelative(path) == TRUE;
}

};

