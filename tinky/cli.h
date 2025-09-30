#ifndef CLI_H
#define CLI_H

#include <windows.h>

int svc_install(void);
int svc_stop(void);
int svc_start(void);
int svc_delete(void);
int svc_status(void);
void PrintLastError(const wchar_t* where);

#endif