// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

// Function to get the process name by PID
void GetProcessNameByPid(DWORD pid, TCHAR* processName) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL) {
        processName[0] = '\0'; // Empty process name if not found
        return;
    }

    HMODULE hMod;
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
        GetModuleBaseName(hProcess, hMod, processName, MAX_PATH);
    }
    CloseHandle(hProcess);
}

// Callback function for EnumWindows
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    // Get the process name for the given PID
    TCHAR processName[MAX_PATH] = TEXT("<unknown>");
    GetProcessNameByPid(pid, processName);

    // Compare the process name to the target name
    if (_tcsicmp(processName, (TCHAR*)lParam) == 0) {
        // Set the new window title
        SetWindowText(hwnd, (LPCWSTR)((TCHAR*)lParam + MAX_PATH));  // Title passed after process name
    }

    return TRUE;
}

// Exported function to be called from Python
extern "C" DLL_EXPORT void RenameWindowTitleByProcessName(const TCHAR * targetProcessName, const TCHAR * newWindowTitle) {
    // Create a buffer that combines the process name and new title
    TCHAR combinedData[MAX_PATH * 2] = { 0 };
    _tcscpy_s(combinedData, MAX_PATH, targetProcessName); // Copy the process name
    _tcscpy_s(combinedData + MAX_PATH, MAX_PATH, newWindowTitle); // Copy the new title after the process name

    // Enumerate through all open windows and check the process name
    EnumWindows(EnumWindowsProc, (LPARAM)combinedData);
}

// -----------------------------------------------------------------------

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

