#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <iostream>
#include <psapi.h>
#include <string.h>
#include <tchar.h>

#define DLL_EXPORT __declspec(dllexport)