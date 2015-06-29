#include <windows.h>
#include <stdio.h>
#include <locale.h> 
#include <commctrl.h>
#include "ProcessThread.h"
#include "load_driver.h"
#include "iodriver.h"
#include "resource1.h"
#include "mylib.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment (lib, "comctl32.lib")

HINSTANCE hExe;
HWND MainWindow;
extern const PTCHAR NameDriver = TEXT("\\\\.\\QuantumDriver");
extern const PTCHAR NameDrvInReg = TEXT("SetQuant");
WCHAR DriverPath[500] = {0};
HWND EditQunatTable[3] = {0};
HWND EditStartQunatTable[3] = {0};
HWND ProcessList = NULL;
HWND ThreadList = NULL;
HWND EditTable;
UCHAR QuantTable[3], StartQuantTable[3];
LPVOID AddressTable;
bool isDriverLoad;
HANDLE OpenedDriver = NULL;
DWORD CurSelectedProcess = 0;
WNDPROC OriginalEditProc;
HWND hMinTimerInterval, hMaxTimerInterval, hCurTimerInterval;
ULONG MinimumResolution,MaximumResolution,CurrentResolution;
typedef NTSTATUS (__stdcall * NTQUERYTIMERRESOL)(PULONG MinimumResolution,PULONG MaximumResolution,PULONG CurrentResolution);
NTQUERYTIMERRESOL NtQueryTimerResolution = NULL;
HWND CheckSetLock = NULL;

LV_COLUMN ColumnProcessList[] = 
{
	{LVCF_FMT|LVCF_TEXT|LVCF_SUBITEM|LVCF_WIDTH,LVCFMT_CENTER,150,TEXT("Id"),0,0},
	{LVCF_FMT|LVCF_TEXT|LVCF_SUBITEM|LVCF_WIDTH,LVCFMT_CENTER,150,TEXT("Image name"),0,1},
	{LVCF_FMT|LVCF_TEXT|LVCF_SUBITEM|LVCF_WIDTH,LVCFMT_CENTER,147,TEXT("Quantum"),0,2}
};

LV_COLUMN ColumnThreadList[] = 
{
	{LVCF_FMT|LVCF_TEXT|LVCF_SUBITEM|LVCF_WIDTH,LVCFMT_CENTER,150,TEXT("Id"),0,0},
	{LVCF_FMT|LVCF_TEXT|LVCF_SUBITEM|LVCF_WIDTH,LVCFMT_CENTER,147,TEXT("Quantum"),0,1}
};

#define COUNT_PROCESS_COLUMN (sizeof(ColumnProcessList)/sizeof(ColumnProcessList[0]))
#define COUNT_THREAD_COLUMN (sizeof(ColumnThreadList)/sizeof(ColumnThreadList[0]))
INT_PTR WINAPI MainDlgProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
bool LoadFileFromResource(HMODULE hModule,PTCHAR lpName, LPWSTR lpType, LPWSTR lpOutFileName);
int WINAPI EditProcessQuantProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
int WINAPI EditThreadQuantProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
void OutputCurTimerResol();
void SendQuantFromTextBox(HWND hTextBox, BYTE IndexText);

INT_PTR WINAPI WinMain(HINSTANCE hExe,HINSTANCE, LPSTR lpCmdShow, int nShowCmd)
{
	hExe = hExe;
	setlocale(LC_CTYPE,"russian");
	InitCommonControls();
	//Creating dialog
	MainWindow = CreateDialog(hExe,MAKEINTRESOURCE(IDD_DIALOG1),NULL,MainDlgProc);
	ShowWindow(MainWindow,nShowCmd);
	UpdateWindow(MainWindow);
	MSG msg;
	//Message loop
	while (GetMessage(&msg, NULL, NULL, NULL)) 
	{  
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
		if(msg.message == WM_KEYDOWN)
			MainDlgProc(msg.hwnd,msg.message, msg.wParam,msg.lParam);
	}
	return 0;
}

INT_PTR WINAPI MainDlgProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
		{
			MainWindow = hWnd;
			ProcessList = GetDlgItem(MainWindow, IDC_PROCESS);
			ThreadList = GetDlgItem(MainWindow, IDC_THREAD);
			EditQunatTable[0] = GetDlgItem(MainWindow, IDC_EDIT1);
			EditQunatTable[1] = GetDlgItem(MainWindow, IDC_EDIT2);
			EditQunatTable[2] = GetDlgItem(MainWindow, IDC_EDIT3);

			EditStartQunatTable[0] = GetDlgItem(MainWindow, IDC_EDIT6);
			EditStartQunatTable[1] = GetDlgItem(MainWindow, IDC_EDIT5);
			EditStartQunatTable[2] = GetDlgItem(MainWindow, IDC_EDIT4);

			hMinTimerInterval = GetDlgItem(MainWindow, IDC_EDIT9);
			hMaxTimerInterval = GetDlgItem(MainWindow, IDC_EDIT8);
			hCurTimerInterval = GetDlgItem(MainWindow, IDC_EDIT7);
			CheckSetLock = GetDlgItem(MainWindow, IDC_CHECK2);
			for(int i = 0;i < 3;i++)
			{
				SetWindowText(EditQunatTable[i],TEXT("-"));
				SetWindowText(EditStartQunatTable[i],TEXT("-"));
			}
			if(OpenDriver())
				isDriverLoad = true;
			else
			{
				fprintf(stderr, "SETQUANT_GUI: Loading driver\n");
				if(!SetPrivilege())
					fprintf(stderr, "QUANT_GUI: Dont set privelege !\n");
				GetSystemDirectoryW(DriverPath,500);
				wcscat(DriverPath,L"\\drivers\\setquant.sys");
				if(!LoadFileFromResource(hExe,MAKEINTRESOURCE(IDR_BIN1), TEXT("bin"), DriverPath))
					fprintf(stderr, "QUANT_GUI: Driver not copied in sys path\n");

				isDriverLoad = LoadDriver(DriverPath,NameDrvInReg);
				if(isDriverLoad)
					fprintf(stderr, "QUANT_GUI: Driver load in system\n");
				else
					fprintf(stderr, "QUANT_GUI: Driver not load in system\n");
			}
			if(isDriverLoad)
			{
				if(OpenedDriver == (HANDLE)-1)
					if(!OpenDriver())
					{
						fprintf(stderr, "QUANT_GUI: Cannot open driver!\n");
						UnloadDriver(NameDrvInReg);
						isDriverLoad = false;
						goto lblNotLoadDriver;
					}

					//Если получили таблицу квантов
					if(GetPspForegroundQuantumAddress(&AddressTable))
						fprintf(stderr, "QUANT_GUI: Address quantum table in kernel: %x\n", AddressTable);
					else
						fprintf(stderr, "QUANT_GUI: Not get address quantum table!\n");
					//Получаем начальную таблицу квантов
					if(GetStartQuantumTable(StartQuantTable))
					{
						fprintf(
							stderr, 
							"QUANT_GUI: Start value quantums %d %d %d \n", 
							(DWORD)StartQuantTable[0],
							(DWORD)StartQuantTable[1],
							(DWORD)StartQuantTable[2]
						);
						char Buf[10] = {0};
						sprintf(Buf,"%d",(DWORD)StartQuantTable[0]);
						SetWindowTextA(EditStartQunatTable[0],Buf);

						sprintf(Buf,"%d",(DWORD)StartQuantTable[1]);
						SetWindowTextA(EditStartQunatTable[1],Buf);

						sprintf(Buf,"%d",(DWORD)StartQuantTable[2]);
						SetWindowTextA(EditStartQunatTable[2],Buf);
					}
					//Получаем текущую таблицу квантов
					if(GetQuantumTable(QuantTable))
					{
						fprintf(
							stderr, 
							"QUANT_GUI: Current quantums %d %d %d \n", 
							(DWORD)QuantTable[0],
							(DWORD)QuantTable[1],
							(DWORD)QuantTable[2]
						);
						//Активируем редактирование таблицы
						char Buf[10] = {0};
						sprintf(Buf,"%d",(DWORD)QuantTable[0]);
						SetWindowTextA(EditQunatTable[0],Buf);
						SendMessage(EditQunatTable[0], EM_SETREADONLY, FALSE,NULL);

						sprintf(Buf,"%d",(DWORD)QuantTable[1]);
						SetWindowTextA(EditQunatTable[1],Buf);
						SendMessage(EditQunatTable[1], EM_SETREADONLY, FALSE,NULL);

						sprintf(Buf,"%d",(DWORD)QuantTable[2]);
						SetWindowTextA(EditQunatTable[2],Buf);
						SendMessage(EditQunatTable[2], EM_SETREADONLY, FALSE,NULL);
					}
					else
						fprintf(stderr, "QUANT_GUI: Could not get a table quantums.\n");

					SendMessage(hMinTimerInterval, EM_SETREADONLY, FALSE,NULL);
					SendMessage(hMaxTimerInterval, EM_SETREADONLY, FALSE,NULL);
					SendMessage(hCurTimerInterval, EM_SETREADONLY, FALSE,NULL);
					bool StateLockInterval;
					if(GetLockIntervalState(&StateLockInterval))
						SendMessage(CheckSetLock, BM_SETCHECK,StateLockInterval?BST_CHECKED:BST_UNCHECKED,NULL);
			}
lblNotLoadDriver:	
			OutputCurTimerResol();

			ListView_SetExtendedListViewStyle(ProcessList,LVS_EX_COLUMNSNAPPOINTS| LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES| ListView_GetExtendedListViewStyle(ProcessList));
			ListView_SetExtendedListViewStyle(ThreadList,LVS_EX_COLUMNSNAPPOINTS| LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES| ListView_GetExtendedListViewStyle(ThreadList));
			RECT ListProcessRect;
			GetClientRect(ProcessList,&ListProcessRect);
			for(BYTE i = 0; i  < COUNT_PROCESS_COLUMN;i++)
			{
				ColumnProcessList[i].cx = (ListProcessRect.right - ListProcessRect.left - ((i== 0)?70:4)) / COUNT_PROCESS_COLUMN;
				ListView_InsertColumn(ProcessList,i,&(ColumnProcessList[i]));
			}
			GetClientRect(ThreadList,&ListProcessRect);
			for(BYTE i = 0; i  < COUNT_THREAD_COLUMN;i++)
			{
				ColumnThreadList[i].cx = (ListProcessRect.right - ListProcessRect.left - 2) / COUNT_THREAD_COLUMN;
				ListView_InsertColumn(ThreadList,i,&(ColumnThreadList[i]));
			}
			EnumerateProcs(ProcessList);
			LV_ITEM lvi = {LVIF_PARAM,CurSelectedProcess,0};
			ListView_GetItem(ProcessList,&lvi);
			EnumerateThreads(lvi.lParam, ThreadList);
		}
		break;
	case WM_CLOSE:
		//При закрытии окна
		{
			if(isDriverLoad)
			{ 
				if(MessageBox(
					MainWindow,
					TEXT("You want unload driver?\n If yes, then your settings will be deleted."),
					TEXT("Warning"),
					MB_ICONEXCLAMATION|MB_OKCANCEL) == 1)
					UnloadDriver(NameDrvInReg);
			}
			//Удаляем драйвер из системной папки
			//DeleteFile(DriverPath);
			//Удаляем из реестра
			DeleteDriverFromReg(NameDrvInReg);
			PostQuitMessage(0);
		}
		break;
	case WM_KEYDOWN:
		if(wParam == 13)
		switch(GetDlgCtrlID(hWnd))
		{
			//Если необходимо установить новый квант
			case IDC_EDIT1:
				SendQuantFromTextBox(hWnd, 0);
				break;
			case IDC_EDIT2:
				SendQuantFromTextBox(hWnd, 1);
				break;
			case IDC_EDIT3:
				SendQuantFromTextBox(hWnd, 2);
				break;
			case IDC_EDIT9:
				{
					int NewMinResol =  GetDlgItemInt(MainWindow, IDC_EDIT9,NULL,TRUE);
					SetTimerResolution(NewMinResol,0,0);
					OutputCurTimerResol();
				}
				break;
			case IDC_EDIT8:
				{
					int NewMaxResol =  GetDlgItemInt(MainWindow, IDC_EDIT8,NULL,TRUE);
					if(SetTimerResolution(0,NewMaxResol,0))
						OutputCurTimerResol();
				}
				break;
			case IDC_EDIT7:
				{
					//typedef NTSTATUS (__stdcall * SETTIMERRESOLUTION) (ULONG RequestedResolution,BOOLEAN Set,PULONG ActualResolution);
					//SETTIMERRESOLUTION NtSetTimerResolution = (SETTIMERRESOLUTION)GetProcAddress( LoadLibrary(TEXT("NTDLL.DLL")),"NtSetTimerResolution");
					int NewCurResol =  GetDlgItemInt(MainWindow, IDC_EDIT7,NULL,TRUE);
					//ULONG CurResol;
					//NTSTATUS j = NtSetTimerResolution(NewCurResol,TRUE,&CurResol);
					SetTimerResolution(0,0,NewCurResol);
					OutputCurTimerResol();
				}
				break;
		}

		break;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDC_BUTTON3:
			{
				EnumerateProcs(ProcessList);
				if(ListView_GetItemCount(ProcessList) >= CurSelectedProcess)
					CurSelectedProcess = ListView_GetItemCount(ProcessList) - 1;
				LV_ITEM lvi = {LVIF_PARAM,CurSelectedProcess,0};
				ListView_GetItem(ProcessList,&lvi);
				EnumerateThreads(lvi.lParam, ThreadList);
			}
			break;
		case IDC_CHECK2:
			{
				LRESULT Checked = SendMessage (CheckSetLock, BM_GETCHECK, 0, 0);
				if(Checked == BST_CHECKED)
				{
					int NewCurResol =  GetDlgItemInt(MainWindow, IDC_EDIT7,NULL,TRUE);
					SetLockInterval(true, NewCurResol);
					bool SettedState = false;
					GetLockIntervalState(&SettedState);
					if(!SettedState)
						SendMessage (CheckSetLock, BM_SETCHECK, BST_UNCHECKED, 0);

				}else if(Checked == BST_UNCHECKED)
				{
					SetLockInterval(false);
					bool SettedState = false;
					GetLockIntervalState(&SettedState);
					if(SettedState)
						SendMessage (CheckSetLock, BM_SETCHECK, BST_CHECKED, 0);
				}
				OutputCurTimerResol();
			}
			break;
		}
		break;
	case WM_NOTIFY:
		{
			//Если пришло сообщение с List View
			if(((int)wParam == IDC_PROCESS) && (((LPNMHDR)lParam)->code == NM_DBLCLK))
			{
				//При двойном клике по элементу начинаем редактировать его
				LPNMITEMACTIVATE j = (LPNMITEMACTIVATE)lParam;
				if(j->iItem < 0)
					break;
				CurSelectedProcess = j->iItem;
				LV_ITEM lvi = {LVIF_PARAM,CurSelectedProcess,0};
				ListView_GetItem(ProcessList,&lvi);
				if(!EnumerateThreads(lvi.lParam, ThreadList))
					MessageBeep(MB_OK);
				if(j->iSubItem == 2)
				{
					EditTable = ListView_BeginRedactItem(ProcessList,j->iItem,j->iSubItem);
					SetWindowLong(EditTable,GWL_STYLE,GetWindowLong(EditTable,GWL_STYLE)| ES_NUMBER);
					ListView_SetRedactItemData(EditTable,SetWindowLongPtr(EditTable,GWLP_WNDPROC,(LONG)EditProcessQuantProc) );
				}
			}else if(((int)wParam == IDC_THREAD) && (((LPNMHDR)lParam)->code == NM_DBLCLK))
			{

				//При двойном клике по элементу начинаем редактировать его
				LPNMITEMACTIVATE j = (LPNMITEMACTIVATE)lParam;
				if(j->iItem < 0)
					break;
				if(j->iSubItem == 1)
				{
					EditTable = ListView_BeginRedactItem(ThreadList,j->iItem,j->iSubItem);
					SetWindowLong(EditTable,GWL_STYLE,GetWindowLong(EditTable,GWL_STYLE)| ES_NUMBER);
					ListView_SetRedactItemData(EditTable,SetWindowLongPtr(EditTable,GWLP_WNDPROC,(LONG)EditThreadQuantProc) );
				}
			}
		}
		break;
	}
	return 0;
}

void OutputCurTimerResol()
{
	if(NtQueryTimerResolution == NULL)
		NtQueryTimerResolution = (NTQUERYTIMERRESOL)GetProcAddress(LoadLibrary(TEXT("NTDLL.DLL")),"NtQueryTimerResolution");

	NtQueryTimerResolution(&MaximumResolution,&MinimumResolution,&CurrentResolution);
	SetDlgItemInt(MainWindow,GetDlgCtrlID(hMinTimerInterval),MinimumResolution,FALSE);
	SetDlgItemInt(MainWindow,GetDlgCtrlID(hMaxTimerInterval),MaximumResolution,FALSE);
	SetDlgItemInt(MainWindow,GetDlgCtrlID(hCurTimerInterval),CurrentResolution,FALSE);
}

int WINAPI EditProcessQuantProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	if((Msg == WM_KEYDOWN) && (wParam == 13))
	{
		BOOL Tst;
		UINT Result = GetDlgItemInt(ProcessList,GetDlgCtrlID(hWnd),&Tst,FALSE);
		if(!Tst || (Result > 255))
			goto lblOut;
		LV_ITEM lvi = {LVIF_PARAM,CurSelectedProcess,0};
		ListView_GetItem(ProcessList,&lvi);
		if(SetProcessQuantum(lvi.lParam, (UCHAR)Result))
		{
			WCHAR Buf[20];
			wsprintf(Buf,L"%d", Result);
			ListView_SetItemText(ProcessList,CurSelectedProcess,2,Buf);
		}
	}
lblOut:
	return CallWindowProc((WNDPROC)ListView_GetRedactItemData(hWnd),hWnd,Msg,wParam,lParam);
}

//Обработчик нажатия ENTER, когда редактируется квант потока
int WINAPI EditThreadQuantProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	if((Msg == WM_KEYDOWN) && (wParam == 13))
	{
		BOOL Tst;
		UINT Result = GetDlgItemInt(ThreadList,GetDlgCtrlID(hWnd),&Tst,FALSE);
		if(!Tst || (Result > 255))
			goto lblOut;
		int Item, SubItem;
		ListView_GetRedactItem(EditTable,&Item, &SubItem);
		LV_ITEM lvi = {LVIF_PARAM,Item,0};
		ListView_GetItem(ThreadList,&lvi);
		if(SetThreadQuantum(lvi.lParam, (UCHAR)Result))
		{
			WCHAR Buf[20];
			wsprintf(Buf,L"%d", Result);
			ListView_SetItemText(ThreadList,Item,SubItem,Buf);
		}
	}
lblOut:
	return CallWindowProc((WNDPROC)ListView_GetRedactItemData(hWnd),hWnd,Msg,wParam,lParam);
}

//Отправление драйверу нового значения квантов
void SendQuantFromTextBox(HWND hTextBox, BYTE IndexText)
{
	BOOL Test;
	int IdTextBox = GetDlgCtrlID(hTextBox);
	DWORD NewValue = GetDlgItemInt(MainWindow,IdTextBox,&Test,FALSE);
	if(!Test)
	{
		SetDlgItemInt(MainWindow,IdTextBox,(DWORD)QuantTable[IndexText],FALSE);
		MessageBox(MainWindow,TEXT("Too short value!"), NULL, MB_ICONERROR);
		return;
	}
	if(NewValue > 255)
	{
		SetDlgItemInt(MainWindow,IdTextBox,(DWORD)QuantTable[IndexText],FALSE);
		MessageBox(MainWindow,TEXT("Invalid value.\n The value must be from 0 to 255."), NULL, MB_ICONERROR);
		return;
	}
	UCHAR OldValue = QuantTable[IndexText];
	QuantTable[IndexText] = (UCHAR)NewValue;
	if(!SetQuantumTable(QuantTable))
	{
		MessageBox(MainWindow,TEXT("Failed to set value. \n Please try again later."), NULL, MB_ICONERROR);
		QuantTable[IndexText] = OldValue;
		SetDlgItemInt(MainWindow,IdTextBox,(DWORD)OldValue,FALSE);
	}
}

bool LoadFileFromResource(HMODULE hModule,PTCHAR lpName, LPWSTR lpType, LPWSTR lpOutFileName)
{
	HRSRC FindedRes = FindResourceW(hModule,lpName,lpType);
	if(FindedRes == NULL)
		return false;
	HGLOBAL hLodResource = LoadResource(hModule, FindedRes);
	if(hLodResource == NULL)
		return false;
	LPVOID lpDataRes = LockResource(hLodResource);
	DWORD Size = SizeofResource(hModule, FindedRes);
	DWORD Written = 0;
	HANDLE OutFile = CreateFileW(
		lpOutFileName,
		FILE_ALL_ACCESS,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if(OutFile == NULL)
		goto lblOutErr;
	WriteFile(OutFile,lpDataRes,Size,&Written,NULL);
	CloseHandle(OutFile);
lblOutErr:
	UnlockResource(hLodResource);
	FreeResource(hLodResource);
	return Written == Size;
}
