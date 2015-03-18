#ifndef __MY_LIB_H__
#define __MY_LIB_H__
#include <windows.h>

HWND ListView_BeginRedactItem(HWND hWndOwner, int iItem, int iSubItem,bool isScroll = false,bool noSetText = false, HINSTANCE hInstProgramm = GetModuleHandle(NULL));
bool ListView_EndRedactItem(HWND hEdit);
LONG ListView_SetRedactItemData(HWND hEdit, LONG NewData);
LONG ListView_GetRedactItemData(HWND hEdit);
bool ListView_GetRedactItem(HWND hEdit,int * iItem, int * iSubItem);

#endif