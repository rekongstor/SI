#pragma once
#include <wtypes.h>

#ifdef _WIN64
typedef long long WinProc;
#else
typedef long WinProc;
#endif

extern WinProc winProc(HWND window, unsigned msg, WPARAM wp, LPARAM lp);