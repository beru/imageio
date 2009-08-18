#if !defined(AFX_PATHUTILITY_H__4AFF9AFD_F671_4ADB_A1AF_0D80F180E6C2__INCLUDED_)
#define AFX_PATHUTILITY_H__4AFF9AFD_F671_4ADB_A1AF_0D80F180E6C2__INCLUDED_

#include <TCHAR.h>
#include <atlstr.h>

namespace PathUtility  
{

bool IsSamePath(LPCTSTR lhs, LPCTSTR rhs);
CString Canonicalize(LPCTSTR path);
CString ChangeExtension(const TCHAR* filename, const TCHAR* newExtension);
CString FindExtension(const TCHAR* filePath);
CString GetFileNameWithExtension(const TCHAR* filePath);
bool IsReadOnly(const CString& strPath);
void ResetFileReadOnly(const CString& strPath);
bool IsDirectory(const CString & strPath);
bool IsReadOnlyDirectory(const CString& path);
bool IsWritableDirectory(const CString& path);
CString AppendPath(const CString& leftPath, const CString& rightPath, bool dirCheck = true);
CString AddBackslash(const CString& path);
CString RemoveBackslash(const CString& path);
bool IsRoot(const CString& path);
CString GetValidPath(const CString & strPath);
CString MakePath(const TCHAR *drive = NULL, const TCHAR *dir = NULL, const TCHAR *fname = NULL, const TCHAR *ext = NULL);
CString MakeFolderChangedPath(const TCHAR* oldPath, const TCHAR* folderPath);
CString GetPathFromFile(const CString& fullPath);
CString GetModuleFileFolderPath();
bool IsFileExist(const TCHAR* path);
CString GetTempPath();
CString FindFileName(const CString& path);
CString RemoveFileSpec(const CString& path);
bool IsSpecialFolder(const CString& folderPath);
CString FindLastComponent(const CString& path);
bool HasSubFolder(LPCTSTR folderPath);
double GetDiskFreeSpace(LPCTSTR szDrive);
bool IsUNCServerShare(LPCTSTR path);
bool IsModifiableFolder(LPCTSTR path);
bool IsRelative(LPCTSTR path);

};

#endif // !defined(AFX_PATHUTILITY_H__4AFF9AFD_F671_4ADB_A1AF_0D80F180E6C2__INCLUDED_)
