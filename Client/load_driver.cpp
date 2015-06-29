#include <windows.h>
#include "load_driver.h"

//From kernel sources ))
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
#ifdef MIDL_PASS
	[size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else 
	PWCH   Buffer;
#endif
} UNICODE_STRING;

enum
{
	STATUS_SUCCESS = 0x00000000, 
	STATUS_IMAGE_ALREADY_LOADED = 0xC000010E
};

typedef VOID  (WINAPI *RTLINITUNICODESTRING)(IN OUT UNICODE_STRING * DestinationString,IN PCWSTR SourceString);
typedef NTSTATUS (__stdcall *ZWUNLOADDRIVER)(IN  UNICODE_STRING * DriverServiceName);

bool SetPrivilege()
{
	HANDLE Token;
	TOKEN_PRIVILEGES pr = {0};

	if( OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &Token) )
	{
		if( LookupPrivilegeValue(NULL, TEXT("SeLoadDriverPrivilege"), &pr.Privileges[0].Luid ))
		{
			pr.PrivilegeCount = 1; 
			pr.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
			AdjustTokenPrivileges(Token, false, &pr, sizeof(pr), NULL,NULL); 
		}
		CloseHandle(Token); 
	} 
	return true;
} 



bool LoadDriver(IN PTCHAR DriverPatch,IN PTCHAR DriverName)
{

#ifdef UNICODE
#define lenstring wcslen
#define catstring wcscat
#else
#define lenstring strlen
#define catstring strcat
#endif

	typedef NTSTATUS (__stdcall *ZWLOADDRIVER)(IN  UNICODE_STRING * DriverServiceName);

	HMODULE NtdllInst = LoadLibrary(TEXT("ntdll.dll"));
	if(NtdllInst == NULL)
		return false;
	ZWLOADDRIVER ZwLoadDriver = (ZWLOADDRIVER)GetProcAddress(NtdllInst,"ZwLoadDriver");
	if(ZwLoadDriver == NULL)
		return false;
	RTLINITUNICODESTRING RtlInitUnicodeString = (RTLINITUNICODESTRING)GetProcAddress(NtdllInst,"RtlInitUnicodeString");
	if(RtlInitUnicodeString == NULL)
		return false;

	if(DriverPatch != NULL)
	{
		TCHAR Image[MAX_PATH + 4] = TEXT("\\??\\");	
		HKEY Key, Key2;
		DWORD dType = 1;
		GetFullPathName(DriverPatch, MAX_PATH , &(Image[4]), NULL);
		if(RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("system\\CurrentControlSet\\Services"), &Key) != ERROR_SUCCESS)
			return false;
		if(RegCreateKey(Key, DriverName, &Key2) != ERROR_SUCCESS)
			return false;
		RegSetValueEx(Key2, TEXT("ImagePath"), 0, REG_SZ, (LPBYTE)Image, lenstring(Image) * sizeof(TCHAR));
		RegSetValueEx(Key2, TEXT("Type"), 0, REG_DWORD, (LPBYTE)&dType, sizeof(dType));
		RegCloseKey(Key2);
		RegCloseKey(Key);
	}

	TCHAR DriverRegistry[500] = TEXT("\\registry\\machine\\system\\CurrentControlSet\\Services\\");
	catstring(DriverRegistry,DriverName);

	UNICODE_STRING ImageFile;
	RtlInitUnicodeString(&ImageFile, DriverRegistry);
	NTSTATUS tst = ZwLoadDriver(&ImageFile);

	return (STATUS_SUCCESS == tst) || (STATUS_IMAGE_ALREADY_LOADED == tst);
}

bool DeleteDriverFromReg(IN PTCHAR DriverName)
{
	HKEY Key;
	if(RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("system\\CurrentControlSet\\Services"), &Key) != ERROR_SUCCESS)
		return false;
	if(RegDeleteKey(Key,DriverName) != ERROR_SUCCESS)
	{
		RegCloseKey(Key);
		return false;
	}
	RegCloseKey(Key);
	return true;
}


bool UnloadDriver(IN PTCHAR DriverName)
{
	HMODULE NtdllInst = LoadLibrary(TEXT("ntdll.dll"));
	if(NtdllInst == NULL)
		return false;
	ZWUNLOADDRIVER ZwUnloadDriver = (ZWUNLOADDRIVER)GetProcAddress(NtdllInst,"ZwUnloadDriver");
	if(ZwUnloadDriver == NULL)
		return false;
	RTLINITUNICODESTRING RtlInitUnicodeString = (RTLINITUNICODESTRING)GetProcAddress(NtdllInst,"RtlInitUnicodeString");
	if(RtlInitUnicodeString == NULL)
		return false;
	TCHAR DriverRegistry[500] = TEXT("\\registry\\machine\\system\\CurrentControlSet\\Services\\");
	catstring(DriverRegistry,DriverName);
	UNICODE_STRING ImageFile;
	RtlInitUnicodeString(&ImageFile, DriverRegistry);
	NTSTATUS tst = ZwUnloadDriver(&ImageFile);
	return (tst == STATUS_SUCCESS);
}
