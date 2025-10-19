#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

BOOL EnablePrivilege(LPCWSTR name) {
    HANDLE hProcTok = NULL;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hProcTok))
        return FALSE;

    TOKEN_PRIVILEGES tp = {0};
    if (!LookupPrivilegeValueW(NULL, name, &tp.Privileges[0].Luid)) {
        CloseHandle(hProcTok);
        return FALSE;
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL ok = AdjustTokenPrivileges(hProcTok, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(hProcTok);
    return ok && GetLastError() == ERROR_SUCCESS;
}

BOOL GetSystemImpersonationToken(HANDLE *outToken) {
    if (!outToken) return FALSE;
    *outToken = NULL;

    // Aide pour ouvrir un process SYSTEM sur certaines configs
    EnablePrivilege(L"SeDebugPrivilege");

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return FALSE;

    PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
    BOOL found = Process32FirstW(snap, &pe);
    DWORD pid = 0;

    while (found) {
        if (_wcsicmp(pe.szExeFile, L"winlogon.exe") == 0 ||
            _wcsicmp(pe.szExeFile, L"services.exe") == 0) {
            pid = pe.th32ProcessID;
            break;
        }
        found = Process32NextW(snap, &pe);
    }
    CloseHandle(snap);

    if (!pid) return FALSE;

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return FALSE;

    HANDLE hTok = NULL;
    if (!OpenProcessToken(hProc, TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_IMPERSONATE, &hTok)) {
        CloseHandle(hProc);
        return FALSE;
    }

    HANDLE hDup = NULL;
    if (!DuplicateTokenEx(hTok, MAXIMUM_ALLOWED, NULL,
                          SecurityImpersonation, TokenImpersonation, &hDup)) {
        CloseHandle(hTok);
        CloseHandle(hProc);
        return FALSE;
    }

    CloseHandle(hTok);
    CloseHandle(hProc);
    *outToken = hDup; // à fermer par l’appelant
    return TRUE;
}
