#include "iodriver.h"
/*
Sending command in driver
*/

extern HANDLE OpenedDriver;
extern const PTCHAR NameDriver;

bool OpenDriver()
{
	if(OpenedDriver != (HANDLE)-1)
		CloseHandle(OpenedDriver);
	OpenedDriver = CreateFile(NameDriver, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
	return OpenedDriver != (HANDLE)-1;
}


bool CloseDriver()
{
  if(OpenedDriver != NULL)
		return CloseHandle(OpenedDriver);
  return true;
}	


bool GetPacketFromDriver(LPQUANTUM_PACKET lpPacket)
{
	DWORD Written;
	bool Result = ReadFile(OpenedDriver,lpPacket,sizeof(QUANTUM_PACKET),&Written,NULL);
	return Result;
}

bool SendPacketToDriver(LPQUANTUM_PACKET lpPacket)
{
	DWORD Written;
	bool Result = WriteFile(OpenedDriver, lpPacket, sizeof(QUANTUM_PACKET), &Written, NULL);
	return Result && (Written > 0);
}

bool FindQuantumsByOtherData(PUCHAR lpTable)
{
	 QUANTUM_PACKET Packet = {SEARCH_QUANTUM_BY_OTHER_DATA};
	 memcpy(Packet.QuantTable, lpTable, sizeof(Packet.QuantTable));
	 if(!SendPacketToDriver(&Packet))
		return false;
	return true;

}

bool GetPspForegroundQuantumAddress(LPVOID * lpAddress)
{
	QUANTUM_PACKET Packet = {GET_ADDRESS_QUANT_TABLE};
	if(!SendPacketToDriver(&Packet))
		return false;
	Packet.Type = UNKNOWN_QUERY;
	if(!GetPacketFromDriver(&Packet))
		return false;
	if(Packet.Type == UNKNOWN_QUERY)
		return false;
	*lpAddress = Packet.AddressTable;
	return true;
}

bool GetQuantumTable(PUCHAR lpTable)
{
	QUANTUM_PACKET Packet = {GET_CURRENT_TABLE};
	if(!SendPacketToDriver(&Packet))
		return false;

	Packet.Type = UNKNOWN_QUERY;
	if(!GetPacketFromDriver(&Packet))
		return false;
	if(Packet.Type == UNKNOWN_QUERY)
		return false;
	memcpy(lpTable, Packet.QuantTable, sizeof(Packet.QuantTable));
	return true;
}

bool GetStartQuantumTable(PUCHAR lpTable)
{
	QUANTUM_PACKET Packet = {GET_START_VALUE_TABLE};
	if(!SendPacketToDriver(&Packet))
		return false;
	Packet.Type = UNKNOWN_QUERY;
	if(!GetPacketFromDriver(&Packet))
		return false;
	if(Packet.Type == UNKNOWN_QUERY)
		return false;
	memcpy(lpTable, Packet.QuantTable, sizeof(Packet.QuantTable));
	return true;
}

bool GetThreadQuantum(DWORD idThread, PUCHAR lpQuantum)
{
	QUANTUM_PACKET Packet = {GET_THREAD_QUANT};
	Packet.Id = idThread;
	if(!SendPacketToDriver(&Packet))
		return false;
	Packet.Type = UNKNOWN_QUERY;
	if(!GetPacketFromDriver(&Packet))
		return false;
	if(Packet.Type == UNKNOWN_QUERY)
		return false;
	*lpQuantum = Packet.Quant;
	return true;
}

bool GetProcessQuantum(DWORD idProcess, PUCHAR lpQuantum)
{
	QUANTUM_PACKET Packet = {GET_PROCESS_QUANT};
	Packet.Id = idProcess;
	if(!SendPacketToDriver(&Packet))
		return false;
	Packet.Type = UNKNOWN_QUERY;
	if(!GetPacketFromDriver(&Packet))
		return false;
	if(Packet.Type == UNKNOWN_QUERY)
		return false;
	*lpQuantum = Packet.Quant;
	return true;
}

bool SetProcessQuantum(DWORD idProcess, UCHAR Quantum)
{
	QUANTUM_PACKET Packet = {SET_PROCESS_QUANT};
	Packet.Id = idProcess;
	Packet.Quant = Quantum;
	if(!SendPacketToDriver(&Packet))
		return false;
	return true;
}

bool SetThreadQuantum(DWORD idThread, UCHAR Quantum)
{
	QUANTUM_PACKET Packet = {SET_THREAD_QUANT};
	Packet.Id = idThread;
	Packet.Quant = Quantum;
	if(!SendPacketToDriver(&Packet))
		return false;
	return true;
}

bool SetQuantumTable(PUCHAR pQuantum)
{
	QUANTUM_PACKET Packet = {SET_TABLE_QUANT};
	memcpy(Packet.QuantTable, pQuantum, sizeof(Packet.QuantTable));
	if(!SendPacketToDriver(&Packet))
		return false;
	return true;
}

bool SetTimerResolution(ULONG MinimumResolution,ULONG MaximumResolution,ULONG CurrentResolution)
{
	QUANTUM_PACKET Packet = {SET_TIMER_INCREMENT};
	Packet.MinResolution = MinimumResolution;
	Packet.MaxResolution = MaximumResolution;
	Packet.CurResolution = CurrentResolution;
	if(!SendPacketToDriver(&Packet))
		return false;
	return true;
}

bool SetProcessPriorityMode(DWORD idProcess, PSPROCESSPRIORITYMODE newPriority)
{
	QUANTUM_PACKET Packet = {SET_PROCESS_PRIORITY_MODE};
	Packet.Id = idProcess;
	Packet.PriorityMode = newPriority;
	if(!SendPacketToDriver(&Packet))
		return false;
	return true;
}

bool GetLockIntervalState(bool * bState)
{
 	QUANTUM_PACKET Packet = {IS_TIME_INCREMENT_LOCKED};
	if(!SendPacketToDriver(&Packet))
		return false;

	Packet.Type = UNKNOWN_QUERY;
	if(!GetPacketFromDriver(&Packet))
		return false;
	if(Packet.Type == UNKNOWN_QUERY)
		return false;
	*bState = Packet.isTimeIncrementLocked;
	return true;
}

bool SetLockInterval(bool bState, ULONG LockResolution)
{
	QUANTUM_PACKET Packet = {bState? LOCK_TIME_INCREMENT : UNLOCK_TIME_INCREMENT};
	Packet.CurResolution = LockResolution;
	if(!SendPacketToDriver(&Packet))
		return false;
	return true;

}