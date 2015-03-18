#ifndef __IO_H__
#define __IO_H__
#include <ntddk.h>
#include <windef.h>
#include "HiddenNtProc.h"

#define GET_SIZE_STRUCT_FIELD(Struct, Field) ((ULONG)(&((Struct*)0)->Field) + sizeof(((Struct*)0)->Field))
#define _disable() __asm{cli}
#define _enable() __asm{sti}

//Типы пакетов
typedef enum
{
	UNKNOWN_QUERY,

	GET_ADDRESS_QUANT_TABLE,		//Получение адреса таблицы квантов
	GET_CURRENT_TABLE,				//Получение текущей таблицы
	GET_START_VALUE_TABLE,
	GET_THREAD_QUANT,
	GET_PROCESS_QUANT,

	SET_TABLE_QUANT,
	SET_PROCESS_QUANT,
	SET_THREAD_QUANT,
	SET_PROCESS_PRIORITY_MODE,
	SET_TIMER_INCREMENT,

	SEARCH_QUANTUM_BY_OTHER_DATA,
	LOCK_TIME_INCREMENT,
	UNLOCK_TIME_INCREMENT,
	IS_TIME_INCREMENT_LOCKED,

	RESET_THREAD_QUANTUMS_IN_PROCESS
} TYPE_IN_PACKET;

#pragma pack(push, 1)
//Пакет на запрос информации
typedef struct 
{
   TYPE_IN_PACKET Type;
   union
   {
		UCHAR QuantTable[3];
		LPVOID AddressTable;
		UCHAR Quant;
		PSPROCESSPRIORITYMODE PriorityMode;
		ULONG CurResolution;
		BOOLEAN isTimeIncrementLocked;
   };
   union
   {
	   DWORD Id;
	   ULONG MaxResolution;
   };
   ULONG MinResolution;
} QUANTUM_PACKET, *LPQUANTUM_PACKET;
#pragma pack(pop)



NTSTATUS InUserDataHandler(IN PVOID InData, IN ULONG SizeInBuffer,OUT PULONG_PTR Written);
NTSTATUS OutUserDataHandler(PVOID OutData, ULONG SizeOutBuffer, PULONG_PTR Written);

NTSTATUS SetThreadQuantum(DWORD idThread, UCHAR NewQuantum);
NTSTATUS GetThreadQuantum(DWORD idThread, PUCHAR CurQuantum);
NTSTATUS SetProcessQuantum(DWORD idProcess, UCHAR NewQuantum);
NTSTATUS GetProcessQuantum(DWORD idProcess, PUCHAR CurQuantum);
NTSTATUS SetProcessPriorityMode(DWORD idProcess, PSPROCESSPRIORITYMODE ProcessPriority);
//Setting timer period tick
NTSTATUS SetTimerIncrement(ULONG Max, ULONG Min, ULONG NewCur);
NTSTATUS LockTimerIncrement(ULONG NewCur);
NTSTATUS UnlockTimerIncrement();
ULONG NTAPI HalSetTimeIncrementHandler(IN ULONG Increment);
void ComputeSlideQuntumsInOb();
VOID ReestablishTimerTable();

#endif