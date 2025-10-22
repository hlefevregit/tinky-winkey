#define _UNICODE
#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include <wchar.h>
#include "cli.h"
#include "token.h" 

#define SERVICE_NAME L"tinky"

static SERVICE_STATUS_HANDLE gStatusHandle = NULL;
static SERVICE_STATUS gStatus = {0};
static DWORD gCheckpoint = 0;
static HANDLE gStopEvent = NULL;
static HANDLE gSystemToken = NULL;
static HANDLE gChildProc = NULL;

static void LogWinErr(const wchar_t* where)
{
    (void)where;
    DWORD e = GetLastError();
    wchar_t msg[512];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, e, 0, msg, _countof(msg), NULL);
}

static void LogLastErr(const wchar_t* where) {
    DWORD e = GetLastError();
    wchar_t buf[512];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, e, 0, buf, (DWORD)(sizeof buf / sizeof *buf), NULL);
    fwprintf(stderr, L"[svc] %s failed (%lu): %s\n", where, e, buf);
}

static BOOL RunHiddenConsole(LPCWSTR exePath, LPCWSTR args, LPCWSTR logPath, HANDLE* outProc)
{
    DWORD attr = GetFileAttributesW(exePath);
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return FALSE;
    }

    wchar_t cmd[1024];
    if (args && *args) _snwprintf_s(cmd, _countof(cmd), _TRUNCATE, L"\"%s\" %s", exePath, args);
    else               _snwprintf_s(cmd, _countof(cmd), _TRUNCATE, L"\"%s\"",   exePath);

    wchar_t workdir[MAX_PATH];
    wcsncpy_s(workdir, _countof(workdir), exePath, _TRUNCATE);
    wchar_t* last = wcsrchr(workdir, L'\\');
    if (last) *last = L'\0';

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE }; 
    CreateDirectoryW(L"C:\\Temp", NULL);
    LPCWSTR logFile = (logPath && *logPath) ? logPath : L"C:\\Temp\\child_output.log";
    HANDLE hLog = CreateFileW(logFile, GENERIC_WRITE, FILE_SHARE_READ,
                              &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hLog == INVALID_HANDLE_VALUE) { LogWinErr(L"CreateFile(log)"); return FALSE; }

    HANDLE hNullIn = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                 &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hNullIn == INVALID_HANDLE_VALUE) { CloseHandle(hLog); LogWinErr(L"CreateFile(NUL)"); return FALSE; }

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = hNullIn;
    si.hStdOutput = hLog;
    si.hStdError  = hLog;

    PROCESS_INFORMATION pi = {0};

    DWORD flags = CREATE_NO_WINDOW;
    BOOL ok = CreateProcessW(
        exePath,
        cmd,
        NULL, NULL,
        TRUE,
        flags,
        NULL,
        workdir,
        &si, &pi
    );

    CloseHandle(hNullIn);
    CloseHandle(hLog);

    if (!ok) {
        LogWinErr(L"CreateProcessW");
        return FALSE;
    }

    if (outProc) *outProc = pi.hProcess; else CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return TRUE;
}

static void SetState(DWORD state, DWORD waitHintMs)
{
    gStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    gStatus.dwCurrentState            = state;
    gStatus.dwWin32ExitCode           = NO_ERROR;
    gStatus.dwServiceSpecificExitCode = 0;

    switch (state) {
    case SERVICE_START_PENDING:
    case SERVICE_STOP_PENDING:
        gStatus.dwControlsAccepted = 0;
        gStatus.dwCheckPoint = ++gCheckpoint;
        gStatus.dwWaitHint  = waitHintMs;
        break;
    case SERVICE_RUNNING:
        gStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
        gStatus.dwCheckPoint = 0;
        gStatus.dwWaitHint  = 0;
        break;
    case SERVICE_STOPPED:
    default:
        gStatus.dwControlsAccepted = 0;
        gStatus.dwCheckPoint = 0;
        gStatus.dwWaitHint  = 0;
        break;
    }

    SetServiceStatus(gStatusHandle, &gStatus);
}

static VOID WINAPI CtrlHandler(DWORD ctrl)
{
    if (ctrl == SERVICE_CONTROL_STOP && gStopEvent) {
        SetState(SERVICE_STOP_PENDING, 3000);
        SetEvent(gStopEvent);
    }
}

static BOOL RunWinkeyAsSystem(HANDLE hSystemToken, HANDLE* outProc)
{
    
    if (!hSystemToken) {
        return FALSE;
    }

    wchar_t cmd[1024];
    _snwprintf_s(cmd, _countof(cmd), _TRUNCATE, 
                 L"\"%s\"", L"C:\\Users\\meter\\Code\\tinky-winkey\\winkey\\main.exe");

    wchar_t workdir[] = L"C:\\Users\\meter\\Code\\tinky-winkey\\winkey";

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    CreateDirectoryW(L"C:\\Temp", NULL);
    HANDLE hLog = CreateFileW(L"C:\\Temp\\winkey.log", GENERIC_WRITE, FILE_SHARE_READ,
                              &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hLog == INVALID_HANDLE_VALUE) {
        LogWinErr(L"CreateFile(log)");
        return FALSE;
    }

    HANDLE hNullIn = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                 &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hNullIn == INVALID_HANDLE_VALUE) {
        CloseHandle(hLog);
        LogWinErr(L"CreateFile(NUL)");
        return FALSE;
    }

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = hNullIn;
    si.hStdOutput = hLog;
    si.hStdError  = hLog;
    si.lpDesktop = L"winsta0\\default";

    PROCESS_INFORMATION pi = {0};
    
    BOOL ok = CreateProcessAsUserW(
        hSystemToken,  
        NULL,
        cmd,
        NULL, NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        workdir,
        &si, &pi
    );

    CloseHandle(hNullIn);
    CloseHandle(hLog);

    if (!ok) {
        LogWinErr(L"CreateProcessAsUserW");
        return FALSE;
    }

    if (outProc) *outProc = pi.hProcess; else CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return TRUE;
}

static DWORD WINAPI Worker(LPVOID lpParam)
{
    HANDLE hTok = (HANDLE)lpParam;
    BOOL didImpersonate = FALSE;

    if (hTok) {
        didImpersonate = ImpersonateLoggedOnUser(hTok);
    }

    RunWinkeyAsSystem(hTok, &gChildProc);

    while (WaitForSingleObject(gStopEvent, 2000) == WAIT_TIMEOUT) {
        if (gChildProc) {
            DWORD code = 0;
            if (GetExitCodeProcess(gChildProc, &code) && code != STILL_ACTIVE) {
                // relancer winkey avec le token système :
                RunWinkeyAsSystem(hTok, &gChildProc);
            }
        }
    }

    if (gChildProc) {
        TerminateProcess(gChildProc, 0);
        WaitForSingleObject(gChildProc, 3000);
        CloseHandle(gChildProc);
        gChildProc = NULL;
    }

    if (didImpersonate) RevertToSelf();
    if (hTok) CloseHandle(hTok);

    return 0;
}

static VOID WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    (void)argc; (void)argv;

    gStatusHandle = RegisterServiceCtrlHandlerW(SERVICE_NAME, CtrlHandler);
    if (!gStatusHandle) return;

    SetState(SERVICE_START_PENDING, 5000);
    
    gStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!gStopEvent) {
        gStatus.dwWin32ExitCode = GetLastError();
        SetState(SERVICE_STOPPED, 0);
        return;
    }
    gSystemToken = NULL;
    if (!GetSystemImpersonationToken(&gSystemToken)) {
        printf("Error with the impersonation Token ");
    }

    
    SetState(SERVICE_RUNNING, 0);

    HANDLE th = CreateThread(NULL, 0, Worker, gSystemToken, 0, NULL);
    if (th) {
        gSystemToken = NULL;
        WaitForSingleObject(th, INFINITE);
        CloseHandle(th);
    }
    
    if (gSystemToken) { 
        CloseHandle(gSystemToken); 
        gSystemToken = NULL; 
    }
    CloseHandle(gStopEvent);
    gStopEvent = NULL;

    gStatus.dwWin32ExitCode = NO_ERROR;
    SetState(SERVICE_STOPPED, 0);
}

int wmain(int argc, wchar_t** argv)
{
    if (argc > 1) {
        if (_wcsicmp(argv[1], L"install") == 0) return svc_install();
        if (_wcsicmp(argv[1], L"start")   == 0) return svc_start();
        if (_wcsicmp(argv[1], L"stop")    == 0) return svc_stop();
        if (_wcsicmp(argv[1], L"delete")  == 0) return svc_delete();
        if (_wcsicmp(argv[1], L"status")  == 0) return svc_status();

        fwprintf(stderr, L"Usage: %s [install|start|stop|delete|status]\n", argv[0]);
        return 2;
    }
         SERVICE_TABLE_ENTRYW table[] = {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };
    if (!StartServiceCtrlDispatcherW(table)) {
        PrintLastError(L"StartServiceCtrlDispatcherW");
        fwprintf(stderr, L"Tip: run with one of: install | start | stop | delete | status\n");
        return 1;
    }
    return 0;
}
