#include "stdafx.h"
#include "converter.h"

// inlineすると名前解決に問題が出る。。
std::string ConvertToMString(const char* pStr)
{
	return ConvertToMString(std::string(pStr));
}

std::string ConvertToMString(const wchar_t* pStr)
{
	return ConvertToMString(std::wstring(pStr));
}

std::string ConvertToMString(const std::string& val)
{
	return std::string(val);
}

std::string ConvertToMString(const CString& val)
{
	return ConvertToMString((LPCTSTR)val);
}

std::string ConvertToMString(const std::wstring& val)
{
    int length( ::WideCharToMultiByte( CP_ACP, 0, &val[ 0 ], -1, NULL, 0, NULL, NULL ) );
    std::string result( length - 1, 0 );
    ::WideCharToMultiByte( CP_ACP, 0, &val[ 0 ], -1, &result[ 0 ], length, NULL, NULL );
    return result;
}

std::wstring ConvertToWString(const std::string& val)
{
    int length( ::MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, &val[ 0 ], -1, NULL, 0 ) );
    std::wstring result( length - 1, 0 );
    ::MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, &val[ 0 ], -1, &result[ 0 ], length );
    return result;
}

std::wstring ConvertToWString(const char* pStr)
{
	return ConvertToWString(std::string(pStr));
}

std::wstring ConvertToWString(const wchar_t* pStr)
{
	return ConvertToWString(std::wstring(pStr));
}

std::wstring ConvertToWString(const std::wstring& val)
{
	return val;
}

std::wstring ConvertToWString(const CString& val)
{
	return ConvertToWString((LPCTSTR)val);
}

std::basic_string<TCHAR> ConvertToTString(const char* pStr)
{
	return ConvertToTString(std::string(pStr));
}

std::basic_string<TCHAR> ConvertToTString(const wchar_t* pStr)
{
	return ConvertToTString(std::wstring(pStr));
}

std::basic_string<TCHAR> ConvertToTString(const std::string& val)
{
#ifdef _UNICODE
	return ConvertToWString(val);
#else
	return val;
#endif
}

std::basic_string<TCHAR> ConvertToTString(const std::wstring& val)
{
#ifdef _UNICODE
	return val;
#else
	return ConvertToMString(val);
#endif
}

std::basic_string<TCHAR> ConvertToTString(const CString& val)
{
#ifdef _UNICODE
	return ConvertToWString(val);
#else
	return ConvertToMString(val);
#endif
}
