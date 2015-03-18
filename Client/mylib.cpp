#include "mylib.h"
#include <commctrl.h>

//Данные привязанные к редактируемой ячейке
typedef struct 
{
	WNDPROC	OriginalProc;
	WNDPROC	OriginalEditProc;
	HWND	CurEdit;
	LONG	UserData;
	bool	isEditHide;
	int		ColumnHeadHeight;
	int		iItem;
	int		iSubItem;
} LISTVIEW_EDIT_DATA;

#define LISTVIEW_EDIT_ID 0x200

LRESULT WINAPI ListViewEditProcScroll(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
LRESULT WINAPI ListViewEditProcNoScroll(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
LRESULT WINAPI EditProcNoScroll(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);

//Начало редактирования элемента
HWND ListView_BeginRedactItem(HWND hWndOwner, int iItem, int iSubItem, bool isScroll,bool noSetText, HINSTANCE hInstProgramm)
{
	HWND RetHWND = GetDlgItem(hWndOwner,LISTVIEW_EDIT_ID);
	if(RetHWND != NULL)
		return RetHWND;

	RECT RectCurElement;
	if(!ListView_GetSubItemRect(hWndOwner,iItem,iSubItem,LVIR_BOUNDS,&RectCurElement))
		return NULL;
	//Получаем координаты заголовка
	RECT RectCurHeader;
	HD_ITEM hdItem = {HDI_FORMAT};
	DWORD AlignEdit;
	HWND CurHeader = ListView_GetHeader(hWndOwner);
	Header_GetItemRect(CurHeader,iSubItem,&RectCurHeader);
	Header_GetItem(CurHeader,iSubItem,&hdItem);
	
	switch(hdItem.fmt & 0x7)
	{
	case HDF_CENTER:
		AlignEdit = ES_CENTER;
		break;
	case HDF_LEFT:
		AlignEdit = ES_LEFT;
		break;
	case HDF_RIGHT:
		AlignEdit = ES_RIGHT;
		break;
	default:
		AlignEdit = 0;
	}

	TCHAR CurText[1024];
	if(!noSetText)
		ListView_GetItemText(hWndOwner,iItem,iSubItem,CurText,sizeof(CurText)/sizeof(TCHAR));

	RetHWND = CreateWindow(
		TEXT("EDIT"),
		(noSetText)?NULL:CurText,
		WS_CHILD |ES_AUTOHSCROLL | WS_VISIBLE | AlignEdit,
		RectCurElement.left + 1,
		RectCurElement.top,
		ListView_GetColumnWidth(hWndOwner,iSubItem) - 1,
		RectCurElement.bottom - RectCurElement.top - 1,
		hWndOwner,
		(HMENU)LISTVIEW_EDIT_ID,
		hInstProgramm,
		NULL
		);

	//Задаём нормальный шрифт
	SendMessage(RetHWND, WM_SETFONT, (WPARAM)SendMessage(hWndOwner, WM_GETFONT,0,0), TRUE);
	
	LISTVIEW_EDIT_DATA * EditData = new LISTVIEW_EDIT_DATA;
	EditData->CurEdit = RetHWND;
	//Если без скроллинга то забиндиваем наш обработчик на оканчание печатанья
	if(!isScroll)
		EditData->OriginalEditProc = (WNDPROC)SetWindowLongPtr(RetHWND, GWL_WNDPROC,(LONG)EditProcNoScroll);
	EditData->ColumnHeadHeight = -RectCurHeader.bottom;
	EditData->iItem = iItem;
	EditData->iSubItem = iSubItem;
	EditData->OriginalProc = (WNDPROC)SetWindowLongPtr(hWndOwner,GWLP_WNDPROC,(LONG)((isScroll)?ListViewEditProcScroll:ListViewEditProcNoScroll));
	SetWindowLongPtr(RetHWND,GWLP_USERDATA, (LONG)EditData);
	SetFocus(RetHWND);
	return RetHWND;
}



LRESULT WINAPI ListViewEditProcScroll(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	HWND EditWindow = GetDlgItem(hWnd,LISTVIEW_EDIT_ID);
	LISTVIEW_EDIT_DATA * EditData = (LISTVIEW_EDIT_DATA *)GetWindowLongPtr(EditWindow,GWLP_USERDATA);

	LRESULT result =  CallWindowProc(EditData->OriginalProc,hWnd,Msg,wParam,lParam);
	bool isIvalidate = false;
	switch(Msg)
	{
	case WM_NOTIFY:
		//Сдвиг столбца
		if(((LPNMHDR)lParam)->code != 0xfffffebf)
		  break;
	case WM_HSCROLL:
	case WM_MOUSEWHEEL:
	case WM_VSCROLL:
		//При сроллинге немного косячит поэтому перересовываем
		isIvalidate = true;	
	case WM_KEYFIRST:
lblSlideVisible:
		{
			RECT RectCurElement;
			//Получаем координаты элемента
			ListView_GetSubItemRect(hWnd,EditData->iItem,EditData->iSubItem,LVIR_BOUNDS,&RectCurElement);
			if(((RectCurElement.top + EditData->ColumnHeadHeight) < 0) || !ListView_IsItemVisible(hWnd,EditData->iItem))
			{
				if(!EditData->isEditHide)
				{
					//Если окно видимо, скрываем
					ShowWindow(EditWindow,SW_HIDE);
					EditData->isEditHide = true;
				}
				break;
			}
			//Меняем позицию Edit контрола
			SetWindowPos
			(
				EditWindow,
				NULL,
				RectCurElement.left + 1,
				RectCurElement.top,
				ListView_GetColumnWidth(hWnd,EditData->iSubItem) - 1,
				RectCurElement.bottom - RectCurElement.top - 1,
				SWP_NOZORDER
			);

			if(isIvalidate)
			{
				if(EditData->isEditHide)
				{
					//Если окно скрыто, отображаем
					ShowWindow(EditWindow,SW_SHOW);
					SetFocus(EditWindow);
					EditData->isEditHide = false;
				}
				InvalidateRect(EditWindow,NULL,TRUE);
			}
		}
		break;
	} 
	return result;
}

LRESULT WINAPI ListViewEditProcNoScroll(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	HWND EditWindow = GetDlgItem(hWnd,LISTVIEW_EDIT_ID);
	LISTVIEW_EDIT_DATA * EditData = (LISTVIEW_EDIT_DATA *)GetWindowLongPtr(EditWindow,GWLP_USERDATA);
	switch(Msg)
	{
	case WM_NOTIFY:
		//При сдвиге столбца
		if(((LPNMHDR)lParam)->code != 0xfffffebf)
			break;
	case WM_HSCROLL:
	case WM_MOUSEWHEEL:
	case WM_VSCROLL:
	case WM_KEYFIRST:
		ListView_EndRedactItem(EditWindow);
		return 0;
	} 
	return CallWindowProc(EditData->OriginalProc,hWnd,Msg,wParam,lParam);
}

LRESULT WINAPI EditProcNoScroll(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	LISTVIEW_EDIT_DATA * EditData = (LISTVIEW_EDIT_DATA *)GetWindowLongPtr(hWnd,GWLP_USERDATA);
	switch(Msg)
	{
	case WM_KILLFOCUS:
		goto lblEndRedact;
	case WM_KEYDOWN:
			switch(wParam)
			{
			case 27://ESC
			case 13://ENTER
lblEndRedact:
				ListView_EndRedactItem(hWnd);
				return 0;
			}
		
		break;
	}
	return CallWindowProc(EditData->OriginalEditProc,hWnd,Msg,wParam,lParam);
}

//Завершение редактирования элемента
bool ListView_EndRedactItem(HWND hEdit)
{
	LISTVIEW_EDIT_DATA * EditData = (LISTVIEW_EDIT_DATA *)GetWindowLongPtr(hEdit,GWLP_USERDATA);
	//Если в окне нет наших данных
	if(EditData == NULL)
		return false;

	//Запрашиваем сам List View
    HWND OwnerEdit = GetParent(hEdit);
	//Предотвращаем рекурсию
	if(EditData->CurEdit != NULL)
		EditData->CurEdit = NULL;
	else
		return false;
	//Отсылаем запрос на убиение окна
	SendMessage(hEdit,WM_CLOSE,0,0);
	//Если окно убилось
	if(!IsWindow(hEdit))
	{
		//Восстанавливаем оригинальную процедуру
		SetWindowLongPtr(OwnerEdit,GWLP_WNDPROC,(LONG)EditData->OriginalProc);
		delete[] EditData;
		return true;
	}
	return false;
}

LONG ListView_SetRedactItemData(HWND hEdit, LONG NewData)
{
   LISTVIEW_EDIT_DATA * EditData = (LISTVIEW_EDIT_DATA *)GetWindowLongPtr(hEdit,GWLP_USERDATA);
   LONG OldData = EditData->UserData;
   EditData->UserData = NewData;
   return OldData;
}

LONG ListView_GetRedactItemData(HWND hEdit)
{
   return ((LISTVIEW_EDIT_DATA *)GetWindowLongPtr(hEdit,GWLP_USERDATA))->UserData;
}

bool ListView_GetRedactItem(HWND hEdit,int * iItem, int * iSubItem)
{
   LISTVIEW_EDIT_DATA * EditData = (LISTVIEW_EDIT_DATA *)GetWindowLongPtr(hEdit,GWLP_USERDATA);
   if(EditData == NULL)
	   return false;
   *iItem = EditData->iItem;
   *iSubItem = EditData->iSubItem;
   return true;
}