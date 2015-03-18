#ifndef __PROCESSTHREAD_H__
#define __PROCESSTHREAD_H__
#include <windows.h>
bool EnumerateProcs(HWND ProcessList);
bool EnumerateThreads(DWORD ProcessId, HWND ThreadList);
#endif