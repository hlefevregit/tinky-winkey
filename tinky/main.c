#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#pragma comment(lib, "advapi32.lib")
static const wchar_t *SERVICE_NAME = L"tinky";
static SERVICE_STATUS_HANDLE gStatusHandle = NULL;
static SERVICE_STATUS gStatus;
static HANDLE gStopEvent = NULL;

static void SetState(DWORD state, DWORD waitHint) 
{
    gStatus.dwCurrentState = state;
    gStatus.dwControlsAccepted = (state == SERVICE_RUNNING) ? SERVICE_ACCEPT_STOP : 0;
    gStatus.dwWaitHint = waitHint;
    SetServiceStatus(gStatusHandle, &gStatus);
}

static VOID WINAPI CtrlHandler(DWORD ctrl)
{
    if (gStopEvent && ctrl == SERVICE_CONTROL_STOP)
    {
        SetState(SERVICE_STOP_PENDING ,3000);
        SetEvent(gStopEvent);
    }
}

static DWORD WINAPI Worker(LPVOID lpParam)
{
    while(WaitForSingleObject(gStopEvent, 2000) == WAIT_TIMEOUT)
    {
            printf("currently running on Worker");
    }
    return 0;
}

static VOID WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    gStatusHandle = RegisterServiceCtrlHandlerW(SERVICE_NAME, CtrlHandler);
    SetState(SERVICE_START_PENDING, 3000);
    gStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    SetState(SERVICE_RUNNING, 0);
    HANDLE hThread = CreateThread(NULL, 0, Worker, NULL, 0, NULL);
    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    CloseHandle(gStopEvent);
    gStopEvent = NULL;

    gStatus.dwWin32ExitCode = NO_ERROR;
    SetState(SERVICE_STOPPED, 0);
}

int wmain(void) 
{
    SERVICE_TABLE_ENTRYW table[] = {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };
    if (!StartServiceCtrlDispatcherW(table)) {
        fprintf(stderr, "Pas lancé par le Service Control Manager (code %lu)\n", GetLastError());
    }
    return 0;
}
