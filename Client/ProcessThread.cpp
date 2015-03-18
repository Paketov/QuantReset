#include "ProcessThread.h"
#include <tlhelp32.h>
#include <commctrl.h>

#include "iodriver.h"

bool EnumerateProcs(HWND ProcessList)
{

	ListView_DeleteAllItems(ProcessList);

	HANDLE pSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	bool bIsok;
	//Структура, в которую будут записаны данные процесса
	PROCESSENTRY32 ProcEntry;
	ProcEntry.dwSize = sizeof(PROCESSENTRY32);

	bIsok = Process32First(pSnap,&ProcEntry);
	while(bIsok)
	{
		{
			WCHAR Buf[50];
			UCHAR ProcessQuant;

			wsprintf(Buf,L"%d", ProcEntry.th32ProcessID);
			LV_ITEM li = {LVIF_TEXT|LVIF_PARAM,0,0,0,0,Buf,0,0,ProcEntry.th32ProcessID};
			//li.iItem = ListView_GetItemCount(ProcessList);
			ListView_InsertItem(ProcessList,&li);

			li.mask = LVIF_TEXT;
			li.iSubItem = 1;
			li.pszText = ProcEntry.szExeFile;
			ListView_SetItem(ProcessList,&li);

			li.mask = LVIF_TEXT;
			li.iSubItem = 2;
			if(GetProcessQuantum(ProcEntry.th32ProcessID, &ProcessQuant))
			{
				wsprintf(Buf,L"%d", (DWORD)ProcessQuant);
				li.pszText = Buf;
			}else
			{
				li.pszText = L"-";
			}
			ListView_SetItem(ProcessList,&li);
		}
		ProcEntry.dwSize = sizeof(PROCESSENTRY32);
		bIsok = Process32Next(pSnap,&ProcEntry);
	}
	CloseHandle(pSnap);
	return true;
}

bool EnumerateThreads(DWORD ProcessId, HWND ThreadList)
{
	ListView_DeleteAllItems(ThreadList);
	{
		//Проверка на запущенность процесса
		HANDLE H = OpenProcess(PROCESS_QUERY_INFORMATION,1,ProcessId);
		if(H == 0)
			return false;
		CloseHandle(H);
	}
	
	//Начнем с создания снимка
	HANDLE pThreadSnap =  CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, ProcessId);

	bool bIsok;
	THREADENTRY32 ThrdEntry;
	ThrdEntry.dwSize = sizeof(THREADENTRY32);
	bIsok = Thread32First(pThreadSnap, &ThrdEntry);
	//и бегаем по всем потокам...
	while(bIsok)
	{	
		//проверяем, тому ли процессу принадлежит поток
		if (ThrdEntry.th32OwnerProcessID == ProcessId)
		{
			WCHAR Buf[50];
			UCHAR ThreadQuant;

			wsprintf(Buf,L"%d", ThrdEntry.th32ThreadID);
			LV_ITEM li = {LVIF_TEXT|LVIF_PARAM,0,0,0,0,Buf,0,0,ThrdEntry.th32ThreadID};
			//li.iItem = ListView_GetItemCount(ProcessList);
			ListView_InsertItem(ThreadList,&li);

			li.mask = LVIF_TEXT;
			li.iSubItem = 1;
			if(GetThreadQuantum(ThrdEntry.th32ThreadID, &ThreadQuant))
			{
				wsprintf(Buf,L"%d", (DWORD)ThreadQuant);
				li.pszText = Buf;
			}else
			{
				li.pszText = L"-";
			}
			ListView_SetItem(ThreadList,&li);
		}
		bIsok = Thread32Next(pThreadSnap, &ThrdEntry);
	}
	CloseHandle(pThreadSnap);
}
