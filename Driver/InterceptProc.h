#ifndef __INTERCEPT_H__
#define __INTERCEPT_H__
#include <ntddk.h>
#include <windef.h>


#pragma pack(push, 1)
typedef struct 
{
	UCHAR  OpcodeJmp;          //0xE9
	PVOID  RelAddressHandler;  //Relative address
} JMP_COMMAND;
#pragma pack(pop)

//Set handling on function
VOID SetInterceptProc(IN PVOID AddressProc,IN PVOID AddressHandler,OUT JMP_COMMAND * OriginalCommand)
{
	ULONG    CR0Reg;
	__asm
	{
			cli
			mov eax, cr0
			mov CR0Reg,eax
			and eax,0xFFFEFFFF		//Clear WP bit
			mov cr0, eax
	}
	//Copy original code
	*OriginalCommand = *(JMP_COMMAND*)AddressProc;
	//Set jmp on new handle
	((JMP_COMMAND*)AddressProc)->OpcodeJmp  = 0xE9;
	((JMP_COMMAND*)AddressProc)->RelAddressHandler = (PVOID)((ULONG)AddressHandler - (ULONG)AddressProc - sizeof(JMP_COMMAND));
	__asm
	{
			mov eax, CR0Reg    
			mov cr0, eax
			sti                     
	}
}

//Return old data in start function
 VOID RaiseInterceptProc(IN PVOID AddressProc,IN JMP_COMMAND * OriginalCommand)
{
	ULONG    CR0Reg;
	__asm
	{
			cli
			mov eax, cr0
			mov CR0Reg,eax
			and eax,0xFFFEFFFF		
			mov cr0, eax
	}
	//Copy original code
	*(JMP_COMMAND*)AddressProc = *OriginalCommand;
	__asm
	{
			mov eax, CR0Reg    
			mov cr0, eax
			sti                     
	}
}

#endif