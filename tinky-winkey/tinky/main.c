#define UNICODE
#define _UNICODE
#include <windows.h>
#include <winsvc.h>

#define SERVICE_NAME L"tinky"

static SERVICE_STATUS_HANDLE gStatusHandle = NULL;
static SERVICE_STATUS gStatus = {0};
static DWORD gCheckpoint = 0;
static HANDLE gStopEvent = NULL;

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
    (void)lpParam;
    while (WaitForSingleObject(gStopEvent, 2000) == WAIT_TIMEOUT) {
        // travail périodique (non bloquant)
        // … (ne PAS faire de Sleep très long ici)
    }
    return 0;
}

static VOID WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    (void)argc; (void)argv;

    gStatusHandle = RegisterServiceCtrlHandlerW(SERVICE_NAME, CtrlHandler);
    if (!gStatusHandle) return;

    SetState(SERVICE_START_PENDING, 3000);

    gStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!gStopEvent) {
        gStatus.dwWin32ExitCode = GetLastError();
        SetState(SERVICE_STOPPED, 0);
        return;
    }

    // ⚠️ Annoncer RUNNING avant d'entrer dans un wait long
    SetState(SERVICE_RUNNING, 0);

    HANDLE th = CreateThread(NULL, 0, Worker, NULL, 0, NULL);
    if (th) {
        WaitForSingleObject(th, INFINITE);
        CloseHandle(th);
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
    StartServiceCtrlDispatcherW(table);
    return 0;
}