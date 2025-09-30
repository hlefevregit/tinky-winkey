#include <windows.h>
#include <stdio.h>
#include <winsvc.h>
#include "cli.h"
#define SERVICE_NAME L"tinky"
#define _UNICODE

void PrintLastError(const wchar_t* where) 
{ 
    DWORD e = GetLastError(); wchar_t buf[512]; 
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, e, \
    0, buf, (DWORD)(sizeof buf / sizeof *buf), NULL); 
    fwprintf(stderr, L"%s failed (0x%08X): %s\n", where, e, buf); 
}

int svc_install(void)
{

    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) { PrintLastError(L"GetModuleFileNameW"); return 1; }
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm) { PrintLastError(L"OpenSCManagerW"); return 1; }
    SC_HANDLE svc = CreateServiceW(
        scm,
        SERVICE_NAME,            // service name
        SERVICE_NAME,            // display name
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,    // start = demand
        SERVICE_ERROR_NORMAL,
        path,
        NULL, NULL, NULL, NULL, NULL);

    if (!svc) {
        PrintLastError(L"CreateServiceW");
        CloseServiceHandle(scm);
        return 1;
    }

    SERVICE_DESCRIPTIONW desc = { L"Demo service tinky" };
    ChangeServiceConfig2W(svc, SERVICE_CONFIG_DESCRIPTION, &desc);

    wprintf(L"Service (%s) installed successfully.\n", SERVICE_NAME);
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return 0;
}



int svc_start(void)
{
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) { PrintLastError(L"OpenSCManagerW"); return 1; }

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_START | SERVICE_QUERY_STATUS);
    if (!svc) { PrintLastError(L"OpenServiceW"); CloseServiceHandle(scm); return 1; }

    if (!StartServiceW(svc, 0, NULL)) {
        DWORD e = GetLastError();
        if (e == ERROR_SERVICE_ALREADY_RUNNING) {
            wprintf(L"Service (%s) already running.\n", SERVICE_NAME);
        } else {
            PrintLastError(L"StartServiceW");
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return 1;
        }
    } else {
        wprintf(L"Service (%s) started successfully.\n", SERVICE_NAME);
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return 0;
}

int svc_stop(void)
{
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) { PrintLastError(L"OpenSCManagerW"); return 1; }

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!svc) { PrintLastError(L"OpenServiceW"); CloseServiceHandle(scm); return 1; }

    SERVICE_STATUS status;
    if (!ControlService(svc, SERVICE_CONTROL_STOP, &status)) {
        PrintLastError(L"ControlService(STOP)");
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return 1;
    }
    wprintf(L"Service (%s) stopped successfully.\n", SERVICE_NAME);

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return 0;
}

int svc_delete(void)
{
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) { PrintLastError(L"OpenSCManagerW"); return 1; }

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, DELETE);
    if (!svc) { PrintLastError(L"OpenServiceW"); CloseServiceHandle(scm); return 1; }

    if (!DeleteService(svc)) {
        PrintLastError(L"DeleteService");
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return 1;
    }
    wprintf(L"Service (%s) deleted successfully.\n", SERVICE_NAME);

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return 0;
}

int svc_status(void)
{
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) { PrintLastError(L"OpenSCManagerW"); return 1; }

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_QUERY_STATUS);
    if (!svc) { PrintLastError(L"OpenServiceW"); CloseServiceHandle(scm); return 1; }

    SERVICE_STATUS_PROCESS ssp;
    DWORD bytes = 0;
    if (!QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &bytes)) {
        PrintLastError(L"QueryServiceStatusEx");
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return 1;
    }

    const wchar_t* st = L"?";
    switch (ssp.dwCurrentState) {
        case SERVICE_STOPPED: st = L"STOPPED"; break;
        case SERVICE_START_PENDING: st = L"START_PENDING"; break;
        case SERVICE_STOP_PENDING: st = L"STOP_PENDING"; break;
        case SERVICE_RUNNING: st = L"RUNNING"; break;
        case SERVICE_CONTINUE_PENDING: st = L"CONTINUE_PENDING"; break;
        case SERVICE_PAUSE_PENDING: st = L"PAUSE_PENDING"; break;
        case SERVICE_PAUSED: st = L"PAUSED"; break;
    }
    wprintf(L"SERVICE_NAME: %s\nSTATE       : %s (PID=%lu)\n",
            SERVICE_NAME, st, (unsigned long)ssp.dwProcessId);

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return 0;

}
