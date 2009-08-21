#pragma once

#include <string>
#include <sstream>

/////////////////////////////////////////////////////////////////////////////
//! •¶Žš—ñŒ^‚Ì‘ŠŒÝ•ÏŠ·
std::string ConvertToMString(const char* pStr);
std::string ConvertToMString(const wchar_t* pStr);
std::string ConvertToMString(const std::string& val);
std::string ConvertToMString(const std::wstring& val);
std::string ConvertToMString(const CString& val);

std::wstring ConvertToWString(const char* pStr);
std::wstring ConvertToWString(const wchar_t* pStr);
std::wstring ConvertToWString(const std::string& val);
std::wstring ConvertToWString(const std::wstring& val);
std::wstring ConvertToWString(const CString& val);

std::basic_string<TCHAR> ConvertToTString(const char* pStr);
std::basic_string<TCHAR> ConvertToTString(const wchar_t* pStr);
std::basic_string<TCHAR> ConvertToTString(const std::string& val);
std::basic_string<TCHAR> ConvertToTString(const std::wstring& val);
std::basic_string<TCHAR> ConvertToTString(const CString& val);

/////////////////////////////////////////////////////////////////////////////
//! •¶Žš—ñ‚Ö‚Ì•ÏŠ·

/*
template <typename FromType>
bool convert(const FromType& from, CString& to)
{
	std::basic_stringstream<TCHAR> stream;
	std::basic_string<TCHAR> str;
	bool ret = (stream << from && stream >> str);
	if (ret)
		to = str.c_str();
	return ret;
}
*/

inline bool convert(const std::string& from, std::string& to)
{
	to = from;
	return true;
}

inline bool convert(const std::string& from, CString& to)
{
	std::basic_string<TCHAR> s = ConvertToTString(from);
	to = s.c_str();
	return true;
}

inline bool convert(const std::wstring& from, CString& to)
{
	std::basic_string<TCHAR> s = ConvertToTString(from);
	to = s.c_str();
	return true;
}

/*
template <typename FromType, typename ToType>
bool convert(const FromType& from, ToType& to)
{
	std::basic_stringstream<ToType::value_type> stream;
	return (stream << from && stream >> to);
}
*/

inline bool convert(const std::wstring& from, std::wstring& to)
{
	to = from;
	return true;
}

inline bool convert(const std::wstring& from, std::string& to)
{
	to = ConvertToMString(from);
	return true;
}

inline bool convert(const std::string& from, std::wstring& to)
{
	to = ConvertToWString(from);
	return true;
}

inline bool convert(const CString& from, std::string& to)
{
	to = ConvertToMString(from);
	return true;
}

inline bool convert(const CString& from, std::wstring& to)
{
	to = ConvertToWString(from);
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//! •¶Žš—ñ‚©‚ç‘¼‚ÌŒ^‚Ö‚Ì•ÏŠ·
/*
template <typename ToType>
bool convert(const CString& from, ToType& to)
{
	std::basic_string<TCHAR> str((LPCTSTR)from);
	std::basic_stringstream<TCHAR> stream;
	return (stream << str && stream >> to);
}
*/

inline bool convert(const CString& from, CString& to)
{
	to = from;
	return true;
}

inline bool convert(const char* from, CString& to)
{
	to = from;
	return true;
}

/*
template <typename FromType, typename ToType>
bool convert(const FromType& from, ToType& to)
{
	std::basic_stringstream<FromType::value_type> stream;
	return (stream << from && stream >> to);
}
*/

inline bool convert(const char* from, double& to)
{
	to = atof(from);
	return true;
}

inline bool convert(double from, char* to)
{
	sprintf(to, "%f", from);
	return true;
}

inline bool convert(double from, std::string& to)
{
	char buff[128] = {0};
	sprintf(buff, "%f", from);
	to = buff;
	return true;
}

inline bool convert(double from, CString& to)
{
	char buff[128] = {0};
	sprintf(buff, "%f", from);
	to = buff;
	return true;
}

inline bool convert(const char* from, size_t& to)
{
	to = atoi(from);
	return true;
}

inline bool convert(size_t from, std::string& to)
{
	char buff[32];
	to = _itoa(from, buff, 10);
	return true;
}

inline bool convert(const wchar_t* from, size_t& to)
{
	to = _wtoi(from);
	return true;
}

inline bool convert(unsigned short from, CString& to)
{
	TCHAR buff[64] = {0};
	_itot(from, buff, 10);
	to = buff;
	return true;
}

inline bool convert(int from, CString& to)
{
	TCHAR buff[64] = {0};
	_itot(from, buff, 10);
	to = buff;
	return true;
}

inline bool convert(long from, CString& to)
{
	TCHAR buff[64] = {0};
	_itot(from, buff, 10);
	to = buff;
	return true;
}

inline bool convert(unsigned long from, CString& to)
{
	TCHAR buff[64] = {0};
	_itot(from, buff, 10);
	to = buff;
	return true;
}

inline bool convert(size_t from, CString& to)
{
	TCHAR buff[64] = {0};
	_itot(from, buff, 10);
	to = buff;
	return true;
}


inline bool convert(const char* from, unsigned long& to)
{
	to = atoi(from);
	return true;
}

inline bool convert(unsigned long from, std::string& to)
{
	char buff[32];
	to = _itoa(from, buff, 10);
	return true;
}

inline bool convert(const char* from, int& to)
{
	to = atoi(from);
	return true;
}

inline bool convert(int from, std::string& to)
{
	char buff[32];
	to = _itoa(from, buff, 10);
	return true;
}

inline bool convert(const char* from, bool& to)
{
	to = (_stricmp(from, "true") == 0);
	return true;
}

inline bool convert(bool from, std::string& to)
{
	to = from ? "true" : "false";
	return true;
}

inline bool convert(const char* from, std::string& to)
{
	to = from;
	return true;
}

inline bool convert(const wchar_t* from, unsigned long& to)
{
	wchar_t* endPtr;
	to = wcstoul(from, &endPtr, 10);
	return true;
}

inline bool convert(const wchar_t* from, int& to)
{
	to = _wtoi(from);
	return true;
}


