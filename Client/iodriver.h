#ifndef __IO_H__
#define __IO_H__
#include <windows.h>

typedef enum _PSPROCESSPRIORITYMODE {
    PsProcessPriorityBackground,
    PsProcessPriorityForeground,
    PsProcessPrioritySpinning
} PSPROCESSPRIORITYMODE;

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

bool OpenDriver();

bool CloseDriver();

bool GetPacketFromDriver(LPQUANTUM_PACKET lpPacket);

bool SendPacketToDriver(LPQUANTUM_PACKET lpPacket);

bool GetPspForegroundQuantumAddress(LPVOID * lpAddress);

bool GetQuantumTable(PUCHAR lpTable);

bool GetStartQuantumTable(PUCHAR lpTable);

bool GetThreadQuantum(DWORD idThread, PUCHAR lpQuantum);

bool GetProcessQuantum(DWORD idProcess, PUCHAR lpQuantum);

bool SetProcessQuantum(DWORD idProcess, UCHAR Quantum);

bool SetThreadQuantum(DWORD idThread, UCHAR Quantum);

bool SetQuantumTable(PUCHAR pQuantum);

bool SetProcessPriorityMode(DWORD idProcess, PSPROCESSPRIORITYMODE newPriority);

bool FindQuantumsByOtherData(PUCHAR lpTable);

bool SetTimerResolution(ULONG MinimumResolution,ULONG MaximumResolution,ULONG CurrentResolution);

bool GetLockIntervalState(bool * bState);

bool SetLockInterval(bool bState, ULONG LockResolution = 0);

#endif