//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
//
// Windows include files

#ifndef FGWINDOWS_HPP
#define FGWINDOWS_HPP

// This code does not support versions of Windows prior to 7 in order to make use of features of that
// and later OSes (eg. InetNtop requires Vista and gestures require 7):
#define WINVER 0x0601

// Include new winsock headers before windows.h since they supersede previous ones
// referenced in windows.h:
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <windowsx.h>
//#include <direct.h>                 // Needed for _getdcwd()
//#include <tchar.h>
//#include <conio.h>
#include <commctrl.h>
#  pragma warning(push)
#  pragma warning(disable:4458)		// Declaration hides class member
#include <Gdiplus.h>
#  pragma warning(pop)
#include <Shlobj.h>
#include <sys/stat.h>
#include <Aclapi.h>
//#pragma comment(lib, "gdiplus.lib") // Tells the linker to look for this library

#undef max                          // These are defined in windows.h and interfere with
#undef min                          // the standard library since macros don't respect namespaces.

#include "resource.h"
#include <string>

#ifndef UNICODE  
typedef std::string String;
#else
typedef std::wstring String;
#endif

String _LoadString(int id, LPCTSTR type) {
  auto src = FindResource(NULL, MAKEINTRESOURCE(id), type);
  if (!src)
    return NULL;

  auto handle = LoadResource(NULL, src);
  if (!handle)
    return NULL;

  auto data = (TCHAR*)LockResource(handle);
  auto size = SizeofResource(NULL, src);
  return String(data, size);
}

inline String LoadHLSL(int id) {
  return _LoadString(id, L"HLSL")
}
#endif

// */
