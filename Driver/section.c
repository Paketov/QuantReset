#include "section.h"


LPIMAGE_NT_HEADER_WITH_SECTION  GetNtHeaders(IN PVOID pBaseAddress)
{
   return (LPIMAGE_NT_HEADER_WITH_SECTION)((ULONG)pBaseAddress + ((PIMAGE_DOS_HEADER)pBaseAddress)->e_lfanew); 
}

BOOL CheckPESignature(IN LPIMAGE_NT_HEADER_WITH_SECTION lpNtHeaders)
{
	static const char * PeSignature = "PE\0\0";
	return *(LPDWORD)PeSignature == lpNtHeaders->Signature;
}

WORD SearchSectionHeaderByName(IN LPIMAGE_NT_HEADER_WITH_SECTION lpNtHeaders, IN PCHAR lpszName)
{
	WORD i = 0;
	WORD CountSections = lpNtHeaders->FileHeader.NumberOfSections;
	for(;i < CountSections;i++)
		if(strcmp((const char*)lpNtHeaders->SectionHeaders[i].Name, lpszName) == 0)
			return i;
	return -1;
}

WORD SearchSectionByInsideAddress(IN LPIMAGE_NT_HEADER_WITH_SECTION lpNtHeaders, IN PVOID pBase, IN PVOID pvInsideAddress)
{
	WORD i = 0;
	WORD CountSections = lpNtHeaders->FileHeader.NumberOfSections;
	DWORD VirtualAddress = (DWORD)pvInsideAddress - (DWORD)pBase;
	for(;i < CountSections;i++)
		if((lpNtHeaders->SectionHeaders[i].VirtualAddress <= VirtualAddress) && 
			((lpNtHeaders->SectionHeaders[i].VirtualAddress + lpNtHeaders->SectionHeaders[i].Misc.VirtualSize) > VirtualAddress))
			return i;
	return -1;
}


PIMAGE_SECTION_HEADER GetSection(IN PVOID pBaseModule, IN PVOID pAddressInside)
{
	LPIMAGE_NT_HEADER_WITH_SECTION lpNtHeaders;
	WORD SectionNumber;
	lpNtHeaders = GetNtHeaders(pBaseModule);
	if(!CheckPESignature (lpNtHeaders))
		return NULL;
	SectionNumber = SearchSectionByInsideAddress(lpNtHeaders,pBaseModule, pAddressInside);
	if(SectionNumber == (WORD)-1)
		return NULL;
	return &lpNtHeaders->SectionHeaders[SectionNumber];
}

PIMAGE_SECTION_HEADER GetSectionByName(IN PVOID pBaseModule, IN PCHAR lpszName)
{
	LPIMAGE_NT_HEADER_WITH_SECTION lpNtHeaders;
	WORD SectionNumber;

	lpNtHeaders = GetNtHeaders(pBaseModule);
	if(!CheckPESignature (lpNtHeaders))
		return NULL;

	SectionNumber = SearchSectionHeaderByName(lpNtHeaders,lpszName);
	if(SectionNumber == (WORD)-1)
		return NULL;
	return &lpNtHeaders->SectionHeaders[SectionNumber];
}