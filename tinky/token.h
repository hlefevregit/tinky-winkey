#ifndef TOKEN_H
#define TOKEN_H

#include <windows.h>
BOOL GetSystemImpersonationToken(HANDLE *outToken);
BOOL EnablePrivilege(LPCWSTR name);

#endif