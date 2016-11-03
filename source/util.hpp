// utility.hpp

// Copyright (C) Bronislaw Kozicki 2004. Use, modification and 
// distribution is subject to the Common Public License Version 1.0 
// See license.txt or http://www.opensource.org/licenses/cpl.php

#pragma once

template <bool> struct static_assert;
template <> struct static_assert<true> {};

namespace util
{

template <typename Type>
class array
{   // non-copyable & non-assignable
	array(const array&);
public:
	// no overhead, stores only const size and const pointer to data
	const size_t			size;

private:
	Type* const				ptr_;

public:
	array(size_t s) : size(s), ptr_(s ? new Type[s] : NULL) {}
	~array() {delete[] ptr_;}

	operator Type* () const {return ptr_;}
};

} // namespace util

namespace str
{

inline bool cast(std::string& dest, const wchar_t* src)
{
	unsigned int len = WideCharToMultiByte(CP_ACP, 0, src, -1, 0, 0, 0, 0);
	if (len == 0)
		return false;

	util::array<char> buf(len);

	if (WideCharToMultiByte(CP_ACP, 0, src, len, buf, static_cast<int> (buf.size), 0, 0) == 0)
		return false;

	dest.assign(buf, len - 1);
	return true;
}

inline bool cast(std::string& dest, const wchar_t* src, size_t len)
{
	util::array<char> buf(len + 1);

	if (WideCharToMultiByte(CP_ACP, 0, src, static_cast<int> (len), buf, static_cast<int> (buf.size), 0, 0) == 0)
		return false;

	dest.assign(buf, len);
	return true;
}

inline bool cast(char *dest, size_t size, const wchar_t* src, size_t len)
{
	if (WideCharToMultiByte(CP_ACP, 0, src, static_cast<int> (len), dest, static_cast<int> (size), 0, 0) == 0)
		return false;
	
	return true;
}

template <size_t Size>
inline bool cast(char (&dest)[Size], const wchar_t* src, size_t len)
{
	return cast(dest, Size, src, len);
}

inline bool cast(std::string& dest, const std::wstring& src)
{
	return cast(dest, src.c_str(), src.length());
}

inline int format(std::string& dest, const char* szformat, ...)
{
	va_list args;
	va_start(args, szformat);

	static const unsigned int local = 300;
	unsigned int len = _vscprintf(szformat, args) + 1;
	if (len > local)
	{
		util::array<char> buf(len);
		int count = vsprintf(buf, szformat, args);
		if (count >= 0)
		{
			dest.assign(buf, count);
			return count;
		}
	}
	else
	{
		char buf[local];
		int count = vsprintf(buf, szformat, args);
		if (count >= 0)
		{
			dest.assign(buf, count);
			return count;
		}
	}

	throw std::runtime_error("Bad format");
}

inline int format(std::wstring& dest, const wchar_t* szformat, ...)
{
	va_list args;
	va_start(args, szformat);

	static const unsigned int local = 300/sizeof(wchar_t);
	unsigned int len = _vscwprintf(szformat, args) + 1;
	if (len > local)
	{
		util::array<wchar_t> buf(len);
		int count = vswprintf(buf, szformat, args);
		if (count >= 0)
		{
			dest.assign(buf, count);
			return count;
		}
	}
	else
	{
		wchar_t buf[local];
		int count = vswprintf(buf, szformat, args);
		if (count >= 0)
		{
			dest.assign(buf, count);
			return count;
		}
	}
		
	throw std::runtime_error("Bad format");
}

inline int format(char *dest, size_t size, const char* szformat, ...)
{
	if (size <= 0)
		throw std::runtime_error("Bad destination size");

	va_list args;
	va_start(args, szformat);

	int count = _vsnprintf(dest, size - 1, szformat, args);
	if (count >= 0)
	{
		dest[count] = '\0';
		return count;
	}
	
	throw std::runtime_error("Bad format");
}

template <size_t Size>
inline int format(char (&dest)[Size], const char* szformat, ...)
{
	va_list args;
	va_start(args, szformat);

	int count = _vsnprintf(dest, Size - 1, szformat, args);
	if (count >= 0)
	{
		dest[count] = '\0';
		return count;
	}
	
	throw std::runtime_error("Bad format");
}

inline int format(wchar_t *dest, size_t size, const wchar_t* szformat, ...)
{
	if (size <= 0)
		throw std::runtime_error("Bad destination size");

	va_list args;
	va_start(args, szformat);

	int count = _vsnwprintf(dest, size - 1, szformat, args);
	if (count >= 0)
	{
		dest[count] = L'\0';
		return count;
	}
	
	throw std::runtime_error("Bad format");
}

template <size_t Size>
inline int format(wchar_t (&dest)[Size], const wchar_t* szformat, ...)
{
	va_list args;
	va_start(args, szformat);

	int count = _vsnwprintf(dest, Size - 1, szformat, args);
	if (count >= 0)
	{
		dest[count] = L'\0';
		return count;
	}
	
	throw std::runtime_error("Bad format");
}

inline int format(const std::nothrow_t&, std::string& dest, const char* szformat, ...)
{
	va_list args;
	va_start(args, szformat);

	static const unsigned int local = 300;
	unsigned int len = _vscprintf(szformat, args) + 1;
	if (len > local)
	{
		try
		{
			util::array<char> buf(len);
			int count = vsprintf(buf, szformat, args);
			if (count >= 0)
			{
				dest.assign(buf, count);
				return count;
			}
		}
		catch (const std::bad_alloc&)
		{
			return 0;
		}
	}
	else
	{
		char buf[local];
		int count = vsprintf(buf, szformat, args);
		if (count >= 0)
		{
			dest.assign(buf, count);
			return count;
		}
	}

	return 0;
}

inline int format(const std::nothrow_t&, std::wstring& dest, const wchar_t* szformat, ...)
{
	va_list args;
	va_start(args, szformat);

	static const unsigned int local = 300/sizeof(wchar_t);
	unsigned int len = _vscwprintf(szformat, args) + 1;
	if (len > local)
	{
		try
		{
			util::array<wchar_t> buf(len);
			int count = vswprintf(buf, szformat, args);
			if (count >= 0)
			{
				dest.assign(buf, count);
				return count;
			}
		}
		catch (const std::bad_alloc&)
		{
			return 0;
		}
	}
	else
	{
		wchar_t buf[local];
		int count = vswprintf(buf, szformat, args);
		if (count >= 0)
		{
			dest.assign(buf, count);
			return count;
		}
	}
		
	return 0;
}

inline int format(const std::nothrow_t&, char *dest, size_t size, const char* szformat, ...)
{
	if (size <= 0)
		return 0;

	va_list args;
	va_start(args, szformat);

	int count = _vsnprintf(dest, size - 1, szformat, args);
	if (count >= 0)
	{
		dest[count] = '\0';
		return count;
	}
	
	return 0;
}

template <size_t Size>
inline int format(const std::nothrow_t&, char (&dest)[Size], const char* szformat, ...)
{
	va_list args;
	va_start(args, szformat);

	int count = _vsnprintf(dest, Size - 1, szformat, args);
	if (count >= 0)
	{
		dest[count] = '\0';
		return count;
	}
	
	return 0;
}

inline int format(const std::nothrow_t&, wchar_t *dest, size_t size, const wchar_t* szformat, ...)
{
	if (size <= 0)
		return 0;

	va_list args;
	va_start(args, szformat);

	int count = _vsnwprintf(dest, size - 1, szformat, args);
	if (count >= 0)
	{
		dest[count] = L'\0';
		return count;
	}
	
	return 0;
}

template <size_t Size>
inline int format(const std::nothrow_t&, wchar_t (&dest)[Size], const wchar_t* szformat, ...)
{
	va_list args;
	va_start(args, szformat);

	int count = _vsnwprintf(dest, Size - 1, szformat, args);
	if (count >= 0)
	{
		dest[count] = L'\0';
		return count;
	}
	
	return 0;
}

template <typename Char> struct chars;

template<> struct
chars<char>
{
	static bool isdigit(const char& ch)
	{
		return ::isdigit(static_cast<int> (static_cast<unsigned char> (ch))) != 0;
	}

	static bool isalpha(const char& ch)
	{
		return ::isalpha(static_cast<int> (static_cast<unsigned char> (ch))) != 0;
	}

	static void toupper(char& ch)
	{
		int i = static_cast<unsigned char> (ch);
		ch = static_cast<char>(::toupper(i));
	}

	static void tolower(char& ch)
	{
		int i = static_cast<unsigned char> (ch);
		ch = static_cast<char>(::tolower(i));
	}

	static const char* const blank()
	{
		return  "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020"
				"\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037\040";
	}
};

template<> struct
chars<wchar_t>
{
	static bool isdigit(const wchar_t& ch)
	{
		return ::iswdigit(static_cast<int> (ch)) != 0;
	}

	static bool isalpha(const wchar_t& ch)
	{
		return ::iswalpha(static_cast<int> (ch)) != 0;
	}

	static void toupper(wchar_t& ch)
	{
		int i = ch;
		ch = static_cast<wchar_t>(::towupper(i));
	}

	static void tolower(wchar_t& ch)
	{
		int i = ch;
		ch = static_cast<wchar_t>(::towlower(i));
	}

	static const wchar_t* const blanks()
	{
		return  L"\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020"
				L"\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037\040";
	}
};

template <typename Char>
inline void trim(std::basic_string<Char>& str)
{
	size_t first = 0;
	size_t pfirst = first;
	do
	{
		pfirst = str.find_first_not_of(static_cast<Char> (0), first);
		first = str.find_first_not_of(chars<Char>::blank(), pfirst);
	}
	while (first != pfirst);

	if (first != std::string::npos)
	{
		size_t last = str.size() - 1;
		size_t plast = last;
		do
		{
			plast = str.find_last_not_of(static_cast<Char> (0), last);
			last = str.find_last_not_of(chars<Char>::blank(), plast);
		}
		while (last != plast);

		if (last != std::string::npos)
		{
			str = str.substr(first, last + 1 - first);
		}
		else
			str.clear();
	}
	else
		str.clear();
}

template <typename Char>
inline void upper(std::basic_string<Char>& str)
{
	std::for_each(str.begin(), str.end(), chars<Char>::toupper);
}

template <typename Char>
inline void lower(std::basic_string<Char>& str)
{
	std::for_each(str.begin(), str.end(), chars<Char>::tolower);
}

} // namespace str

namespace com
{

inline bool cast(std::string& dest, CComVariant& var)
{
	if (var.ChangeType(VT_BSTR) != S_OK || var.bstrVal == NULL)
		return false;
	
	if (!str::cast(dest, var.bstrVal, SysStringLen(var.bstrVal)))
		return false;

	return true;
}

inline bool cast(std::wstring& dest, CComVariant& var)
{
	if (var.ChangeType(VT_BSTR) != S_OK || var.bstrVal == NULL)
		return false;
	
	dest.assign(var.bstrVal, SysStringLen(var.bstrVal));

	return true;
}

inline bool cast(unsigned int& dest, CComVariant& var)
{
	if (var.ChangeType(VT_UI4) != S_OK)
		return false;

	dest = var.ulVal;
	return true;
}

inline bool cast(bool& dest, CComVariant& var)
{
	if (var.ChangeType(VT_BOOL) != S_OK)
		return false;

	dest = (var.boolVal != 0);
	return true;
}

inline HRESULT enforce(HRESULT hr)
{
	if (FAILED(hr))
		AtlThrow(hr);
	return hr;
}

inline void enforce(void* pv)
{
	if (pv == NULL)
		AtlThrow(E_POINTER);
}

} // namespace com

