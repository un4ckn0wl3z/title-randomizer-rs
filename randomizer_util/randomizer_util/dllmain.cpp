// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

DWORD FindProcessIdByName(const std::wstring& processName) {
    DWORD pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (processName == pe.szExeFile) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
    return pid;
}

extern "C" __declspec(dllexport) DWORD FindProcessIdByNameRaw(LPCWSTR processName) {
    return FindProcessIdByName(std::wstring(processName));
}

extern "C" __declspec(dllexport) HWND FindMainWindowByPID(DWORD pid) {
    struct HandleData {
        DWORD pid;
        HWND hwnd;
    } data = { pid, nullptr };

    auto enumCallback = [](HWND hwnd, LPARAM lParam) -> BOOL {
        HandleData& data = *(HandleData*)lParam;
        DWORD wndPid = 0;
        GetWindowThreadProcessId(hwnd, &wndPid);
        if (wndPid == data.pid && GetWindow(hwnd, GW_OWNER) == nullptr && IsWindowVisible(hwnd)) {
            data.hwnd = hwnd;
            return FALSE; // stop enumeration
        }
        return TRUE; // continue
        };

    EnumWindows(enumCallback, (LPARAM)&data);
    return data.hwnd;
}


extern "C" __declspec(dllexport) VOID SetRandomTitle(
    _In_ HWND hWnd,
    _In_opt_ LPCWSTR lpString) {
    SetWindowTextW(hWnd, lpString);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

