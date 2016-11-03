// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#define STRICT
#define WINVER			0x0500
#define _WIN32_WINNT	0x0500
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

// Windows headers
#include <windows.h>
#include <ole2.h>
#include <winsock2.h>
#include <initguid.h>
#define SMTPINITGUID
#include <smtpguid.h>
#include <iadmw.h>
#include <iiscnfg.h>

// ATL headers and definitions
#define _ATL_FREE_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _ATL_ALL_WARNINGS

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <comdef.h>


// C++ standard library headers
#include <cctype>
#include <cwctype>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <algorithm>


using namespace ATL;

