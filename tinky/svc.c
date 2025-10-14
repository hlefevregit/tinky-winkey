#define _UNICODE
#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include "cli.h"
#include "token.h"  // <-- déclare GetSystemImpersonationToken()

#define SERVICE_NAME L"tinky"

static SERVICE_STATUS_HANDLE gStatusHandle = NULL;
static SERVICE_STATUS gStatus = {0};
static DWORD gCheckpoint = 0;
static HANDLE gStopEvent = NULL;
static HANDLE gSystemToken = NULL;


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

static DWORD WINAPI Worker(LPVOID lpParam)
{
    HANDLE hTok = (HANDLE)lpParam;
    BOOL didImpersonate = FALSE;

    if (hTok) {
        didImpersonate = ImpersonateLoggedOnUser(hTok);
        // (optionnel) vérifier didImpersonate pour log
    }

    while (WaitForSingleObject(gStopEvent, 2000) == WAIT_TIMEOUT) {
        // ... travail périodique ...
    }

    if (didImpersonate) RevertToSelf();
    if (hTok) CloseHandle(hTok); // le thread possède maintenant le handle

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
        // Si lancé *hors* SCM sans argument, on peut afficher une aide
        PrintLastError(L"StartServiceCtrlDispatcherW");
        fwprintf(stderr, L"Tip: run with one of: install | start | stop | delete | status\n");
        return 1;
    }
    return 0;
}
//cl /w4 /EHSC /DUNICODE /D_unicode main.c cli.c /llink advapi32.lib /FE:svc.exe