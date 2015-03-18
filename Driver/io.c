

#include "io.h"
#include "main.h"
#include "SrhAddressQuant.h"
#include "HiddenNtProc.h"
#include "InterceptProc.h"

QUANTUM_PACKET LastQueryPacket = {UNKNOWN_QUERY};
ULONG OffsQuantObProcess = (ULONG)-1, OffsQuantObThread = (ULONG)-1;
//
JMP_COMMAND OrigTimeIncrementCommand;
ULONG  CurrentTimeIncrement;
BOOLEAN isLocketTimeIncrement = FALSE;
BOOLEAN isChangedTimerTable = FALSE;
LPRTC_TIME_INCREMENT pTimeIncrement = NULL; 

//Get slide quantums in object EPROCESS & ETHREAD
void ComputeSlideQuntumsInOb()
{
	RTL_OSVERSIONINFOW osInfo;

	////Попытка задать свой период системному таймеру
	//LPRTC_TIME_INCREMENT tst;
	//RTC_TIME_INCREMENT NewTimer = {0x2A, 156250, 0, 0, 0x100};

	//pTimeIncrement = (RTC_TIME_INCREMENT *)SearchTimeTable();

	//DbgPrint("QUANTUM: Founded time table address: %x;",pTimeIncrement);
	//if(tst)
	//{
	//   memcpy(&(pTimeIncrement[4]),&NewTimer,sizeof(RTC_TIME_INCREMENT));
	//   isChangedTimerTable = TRUE;
	//}
	//
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	RtlGetVersion(&osInfo);

	switch(osInfo.dwMajorVersion)
	{
	case 6:
		switch(osInfo.dwMinorVersion)
		{
		case 2:   //Windows 8
			OffsQuantObProcess = 97;
			DbgPrint("QUANTUM:  Offset quantums for Win 8");
			break;
		}
		break;
	case 5:
		switch(osInfo.dwMinorVersion)
		{
		case 1://Win XP
			OffsQuantObProcess = 99;
			OffsQuantObThread =	0x6f;
			DbgPrint("QUANTUM:  Offset quantums for XP");
			break;
		}
		break;
	}
}

//Обработчик входных сообщений 
NTSTATUS InUserDataHandler(IN PVOID InData,IN ULONG SizeInBuffer,OUT PULONG_PTR Written)
{

	LPQUANTUM_PACKET pPaket;
	NTSTATUS ns = STATUS_SUCCESS;
	PVOID TestAddress;
	ULONG QuantQuery;

	LastQueryPacket.Type = UNKNOWN_QUERY;

	*Written = 0;	

	if(SizeInBuffer < GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Type))
		return STATUS_INVALID_USER_BUFFER;

	pPaket = (LPQUANTUM_PACKET)InData;

	switch(pPaket->Type)
	{
	case GET_ADDRESS_QUANT_TABLE:
	case GET_CURRENT_TABLE:
	case GET_START_VALUE_TABLE:
	case IS_TIME_INCREMENT_LOCKED:

		//Запрос на получение оригинальной таблицы квантов
		LastQueryPacket.Type = pPaket->Type;
		*Written = GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Type);
		break;

	case GET_THREAD_QUANT:
	case GET_PROCESS_QUANT:
		//Получение кванта потока
		if(SizeInBuffer < GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id))
			return STATUS_INVALID_USER_BUFFER;

		LastQueryPacket.Type = pPaket->Type;
		LastQueryPacket.Id  =  pPaket->Id;
		*Written =  GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id);
		break;
	case SET_TABLE_QUANT:
		//Установка таблицы квантов
		if(SizeInBuffer < GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,QuantTable))
			return STATUS_INVALID_USER_BUFFER;

		if(PspForegroundQuantum == NULL)
		{
			PspForegroundQuantum = (PUCHAR)GetAddressPspForegroundQuantum();
			if(PspForegroundQuantum == NULL)
			{
				ns = STATUS_CANCELLED;
				break;
			}
			RtlCopyMemory(OldForegroundQuantums,PspForegroundQuantum,sizeof(OldForegroundQuantums));
		}
		isChangedQuantum = TRUE;
		KeEnterCriticalRegion();
		RtlCopyMemory(PspForegroundQuantum, pPaket->QuantTable,sizeof(pPaket->QuantTable));
		KeLeaveCriticalRegion();
#ifdef DEBUG
		DbgPrint("QUANTUM: Quant table success set value %x",*(PULONG)PspForegroundQuantum);
#endif
		*Written = GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,QuantTable);
		break;
	case SET_PROCESS_QUANT:
		//Установка кванта процессу
		if(SizeInBuffer < max(GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Quant),GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id)))
			return STATUS_INVALID_USER_BUFFER;
		ns = SetProcessQuantum(pPaket->Id,pPaket->Quant);
		if(NT_SUCCESS(ns))
		{
			*Written = max(GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Quant),GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id));
		}
		break;
	case SET_THREAD_QUANT:
		//Установка кванта потоку
		if(SizeInBuffer < max(GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Quant),GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id)))
			return STATUS_INVALID_USER_BUFFER;

		ns = SetThreadQuantum(pPaket->Id,pPaket->Quant);

		if(NT_SUCCESS(ns))
		{
			*Written = max(GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Quant),GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id));
		}
		break;
	case SET_PROCESS_PRIORITY_MODE:
		//Установка кванта процессу
		if(SizeInBuffer < max(GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,PriorityMode),GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id)))
			return STATUS_INVALID_USER_BUFFER;
		ns = SetProcessPriorityMode(pPaket->Id,pPaket->PriorityMode);
		if(NT_SUCCESS(ns))
		{
#ifdef DEBUG
			DbgPrint("QUANTUM: Setted process %x priority mode : %x",(ULONG)pPaket->Id, (ULONG)pPaket->PriorityMode);
#endif
			*Written = max(GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,PriorityMode),GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id));
		}
		break;
	case SET_TIMER_INCREMENT:
		if(SizeInBuffer < max(
			max(
			GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,MinResolution),
			GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,MaxResolution)), 
			GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,CurResolution)))
			return STATUS_INVALID_USER_BUFFER;
		ns = SetTimerIncrement(pPaket->MaxResolution,pPaket->MinResolution,pPaket->CurResolution);
		if(NT_SUCCESS(ns))
		{
			*Written = max(
				max(
				GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,MinResolution),
				GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,MaxResolution)), 
				GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,CurResolution));
		}
		break;
	case SEARCH_QUANTUM_BY_OTHER_DATA:
		if(SizeInBuffer < GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,QuantTable))
			return STATUS_INVALID_USER_BUFFER;
		if(PspForegroundQuantum == NULL)
		{
			QuantQuery = 0;
			RtlCopyMemory(&QuantQuery,pPaket->QuantTable ,sizeof(pPaket->QuantTable));
			TestAddress = SearchAddressPspForegroundQuantum(QuantQuery);
			if(TestAddress == NULL)
			{
				ns = STATUS_CANCELLED;
				break;
			}
			PspForegroundQuantum = (PUCHAR)TestAddress;
			RtlCopyMemory(OldForegroundQuantums,PspForegroundQuantum,sizeof(OldForegroundQuantums));
#ifdef DEBUG
			DbgPrint("QUANTUM:  Address founded ! %x %x ", *(PULONG)OldForegroundQuantums, *(PULONG)PspForegroundQuantum);
#endif
		}
		*Written = GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,QuantTable);
		break;
	case LOCK_TIME_INCREMENT:
		if(SizeInBuffer < GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,CurResolution))
			return STATUS_INVALID_USER_BUFFER;
		ns = LockTimerIncrement(pPaket->CurResolution);
		if(NT_SUCCESS(ns))
			*Written = GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,CurResolution);
		break;
	case UNLOCK_TIME_INCREMENT:
		if(SizeInBuffer < GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Type))
			return STATUS_INVALID_USER_BUFFER;

		ns = UnlockTimerIncrement();
		if(NT_SUCCESS(ns))
			*Written = GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Type);
		break;
	case UNKNOWN_QUERY:
	default:
#ifdef DEBUG
		DbgPrint("QUANTUM:  UNKNOWN_QUERY paket");
#endif
		return STATUS_INVALID_PARAMETER;
	}
	return ns;
}


//Возврат значений из драйвера
NTSTATUS OutUserDataHandler(PVOID OutData, ULONG SizeOutBuffer, PULONG_PTR Written)
{
	LPQUANTUM_PACKET pPaket;
	NTSTATUS ns = STATUS_SUCCESS;
	ULONG RegPrioritySeparation;
	pPaket = (LPQUANTUM_PACKET)OutData;
	*Written = 0;

	switch(LastQueryPacket.Type)
	{
	case UNKNOWN_QUERY:
		return STATUS_INVALID_CONNECTION;
		break;
	case GET_ADDRESS_QUANT_TABLE:
		//Запрос на получение адреса таблицы в ядре
		if(SizeOutBuffer < GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,AddressTable))
		{
			ns = STATUS_INVALID_USER_BUFFER;
			break;
		}
		if(PspForegroundQuantum == NULL)
		{
			PspForegroundQuantum = (PUCHAR)GetAddressPspForegroundQuantum();
			if(PspForegroundQuantum == NULL)
			{
				ns = STATUS_CANCELLED;
				break;
			}
			RtlCopyMemory(OldForegroundQuantums,PspForegroundQuantum,sizeof(OldForegroundQuantums));
		}
		pPaket->Type = GET_ADDRESS_QUANT_TABLE;
		pPaket->AddressTable = PspForegroundQuantum;
		*Written = GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,AddressTable);
		break;
	case GET_CURRENT_TABLE:
		//Запрос на получение адреса таблицы в ядре
		if(SizeOutBuffer < GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,QuantTable))
		{
			ns = STATUS_INVALID_USER_BUFFER;
			break;
		}
		if(PspForegroundQuantum == NULL)
		{
			PspForegroundQuantum = (PUCHAR)GetAddressPspForegroundQuantum();
			if(PspForegroundQuantum == NULL)
			{
				ns = STATUS_CANCELLED;
				break;
			}
			RtlCopyMemory(OldForegroundQuantums,PspForegroundQuantum,sizeof(OldForegroundQuantums));
		}
		pPaket->Type = GET_CURRENT_TABLE;
		RtlCopyMemory(pPaket->QuantTable,PspForegroundQuantum,sizeof(pPaket->QuantTable));
		*Written = GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,QuantTable);
		break;
	case GET_START_VALUE_TABLE:
		//Запрос на получение адреса таблицы в ядре
		if(SizeOutBuffer < GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,QuantTable))
		{
			ns = STATUS_INVALID_USER_BUFFER;
			break;
		}
		if(PspForegroundQuantum != NULL)
		{
			RtlCopyMemory(pPaket->QuantTable,OldForegroundQuantums,sizeof(OldForegroundQuantums));
		}else
		{
			PspForegroundQuantum = (PUCHAR)GetAddressPspForegroundQuantum();

			if(PspForegroundQuantum == NULL)
			{
				RegPrioritySeparation = QueryPrioritySeparation();
				if(RegPrioritySeparation == (ULONG)-1)
				{		
					ns = STATUS_CANCELLED;
					break;
				}
				RegPrioritySeparation = ComputeQuantumTable (RegPrioritySeparation);
				RtlCopyMemory(pPaket->QuantTable,&RegPrioritySeparation,sizeof(pPaket->QuantTable));
				goto lblSuccessfully;
			}
			RtlCopyMemory(OldForegroundQuantums,PspForegroundQuantum,sizeof(OldForegroundQuantums));
			RtlCopyMemory(pPaket->QuantTable,PspForegroundQuantum,sizeof(pPaket->QuantTable));
		}
lblSuccessfully:
		pPaket->Type = GET_START_VALUE_TABLE;
		*Written = GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,QuantTable);
		break;
	case GET_THREAD_QUANT:
		//Получение кванта потока
		if(SizeOutBuffer < max(GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Quant),GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id)))
		{
			ns = STATUS_INVALID_USER_BUFFER;
			break;
		}
		ns = GetThreadQuantum(LastQueryPacket.Id, &pPaket->Quant);
		if(NT_SUCCESS(ns))
		{
			pPaket->Type = GET_THREAD_QUANT;
			pPaket->Id = LastQueryPacket.Id;
			*Written = max(GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Quant),GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id));
		}
		break;
	case GET_PROCESS_QUANT:
		//Получение кванта процесса

		if(SizeOutBuffer < max(GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Quant),GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id)))
		{
			ns = STATUS_INVALID_USER_BUFFER;
			break;
		}
		ns = GetProcessQuantum(LastQueryPacket.Id, &pPaket->Quant);
		if(NT_SUCCESS(ns))
		{
			pPaket->Type = GET_PROCESS_QUANT;
			pPaket->Id = LastQueryPacket.Id;
			*Written = max(GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Quant),GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,Id));
		}
		break;
	case IS_TIME_INCREMENT_LOCKED:
		if(SizeOutBuffer < GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,isTimeIncrementLocked))
		{
			ns = STATUS_INVALID_USER_BUFFER;
			break;
		}
		pPaket->isTimeIncrementLocked = isLocketTimeIncrement;
		pPaket->Type = IS_TIME_INCREMENT_LOCKED;
		*Written = GET_SIZE_STRUCT_FIELD(QUANTUM_PACKET,isTimeIncrementLocked);
		break;
	default:
		LastQueryPacket.Type = UNKNOWN_QUERY;
		return STATUS_INVALID_PARAMETER;
	}
	LastQueryPacket.Type = UNKNOWN_QUERY;
	return ns;
}

NTSTATUS LockTimerIncrement(ULONG NewCur)
{
	int i = 0;
	UNICODE_STRING NameHalSetTimeIncrement;
	if(isLocketTimeIncrement)
		return STATUS_SUCCESS;
	if(NewCur != 0)
	{
		if(ExSetTimerResolution(NewCur,TRUE) != NewCur)
		{
			for(;i < 50;i++)
				ExSetTimerResolution(0,FALSE);
			if(ExSetTimerResolution(NewCur,TRUE) == NewCur)
			{		
				isChangedTimerResolution = TRUE;
				DbgPrint("QUANTUM: New quantum resolution: %d", NewCur);
			}
		}
		_disable();
		KeSetSystemAffinityThread(1);
		CurrentTimeIncrement = HalSetTimeIncrement(NewCur);
		KeRevertToUserAffinityThread();
		_enable();
		DbgPrint("QUANTUM: Hal setted: %d", CurrentTimeIncrement);
	}else
		CurrentTimeIncrement = ExSetTimerResolution(0,FALSE);
#ifdef DEBUG
	DbgPrint("QUANTUM: Locked quant resolution: %d", CurrentTimeIncrement);
#endif
	RtlInitUnicodeString(&NameHalSetTimeIncrement, L"HalSetTimeIncrement"); 
	SetInterceptProc(MmGetSystemRoutineAddress(&NameHalSetTimeIncrement),HalSetTimeIncrementHandler,&OrigTimeIncrementCommand);
	isLocketTimeIncrement = TRUE;

	return STATUS_SUCCESS;
}

VOID ReestablishTimerTable()
{
	if(isChangedTimerTable)
		memcpy(pTimeIncrement,TimeIncrementTable,sizeof(RTC_TIME_INCREMENT) * 5);
}
NTSTATUS UnlockTimerIncrement()
{
	UNICODE_STRING NameHalSetTimeIncrement;
	if(isLocketTimeIncrement)
	{
		RtlInitUnicodeString(&NameHalSetTimeIncrement, L"HalSetTimeIncrement"); 
		RaiseInterceptProc(MmGetSystemRoutineAddress(&NameHalSetTimeIncrement),&OrigTimeIncrementCommand);
		isLocketTimeIncrement = FALSE;
	}
	return STATUS_SUCCESS;
}


NTSTATUS SetTimerIncrement(ULONG Max, ULONG Min, ULONG NewCur)
{
	NTSTATUS ns = STATUS_SUCCESS;
	int i = 0;
	UCHAR TimerVal;

	if((Max != 0)|| (Min != 0))
		KeSetTimeIncrement ((Max) ?Max:KeQueryTimeIncrement(),Min);

	if(NewCur)
	{
		if(ExSetTimerResolution(NewCur,TRUE) != NewCur)
		{
			for(;i < 50;i++)
				ExSetTimerResolution(0,FALSE);
			if(ExSetTimerResolution(NewCur,TRUE) == NewCur)
			{		
				isChangedTimerResolution = TRUE;
#ifdef DEBUG
				DbgPrint("QUANTUM: New quantum resolution: %d", NewCur);
#endif
			}
#ifdef DEBUG
			else			
				ns = STATUS_CANCELLED;
#endif
		}
		_disable();
		KeSetSystemAffinityThread(1);
		NewCur = HalSetTimeIncrement(NewCur);
		KeRevertToUserAffinityThread();
		_enable();
#ifdef DEBUG
		if(!NT_SUCCESS(ns))
			DbgPrint("QUANTUM: Not set new resolution. Hal setted: %d", NewCur);
#endif
		// Установка собственного времени таймера
		//от 26 до 29
		//TimerVal = NewCur;
		//WRITE_PORT_UCHAR(0x70,0x0A);
		//WRITE_PORT_UCHAR(0x71, TimerVal);
	}
	return STATUS_SUCCESS;
}
// в самом драйвере объявляем буффер, под новую карту разрешения ввода-вывода
//typedef char IOPM[0x2000];
//IOPM *pNewIO = NULL;
//.....
//.....
//
//// собственно функция взаимодействия с юзерской программой
//NTSTATUS IoDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
//{
//	PIO_STACK_LOCATION IrpStack;
//
//	IrpStack = IoGetCurrentIrpStackLocation(Irp);
//	// сразу устанавливаем поля возврата
//	Irp->IoStatus.Status = STATUS_SUCCESS;
//	Irp->IoStatus.Information = 0;
//
//	// проверяем, что от нас требуют
//	switch (IrpStack->MajorFunction)
//	case IRP_MJ_DEVICE_CONTROL: // пришел запрос через DeviceIoControl
//		switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) // уточняем какой именно
//	case ENABLE: // т.е. потребовали разрешить доступ к портам
//		pIOPM = MmAllocateNonCachedMemory(sizeof(IOPM)); // выделяем память под новую карту ввода-вывода
//		if (pIOPM)
//		{
//			RtlZeroMemory(pNewIO, sizeof(IOPM)); // каждый нулевой бит разрешает доступ к своему порту
//			// и связываем новую карту с процессом, вызвавшим нас
//			Ke386IoSetAccessProcess(PsGetCurrentProcess(), 1); // устанавливаем указатель IOPM на новую карту
//			Ke386SetIoAccessMap(1, pNewIO); // копируем pNewIO в карту ввода-вывода TSS
//		}
//		else
//			Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
//		break;
//
//	case DISABLE: // требуется закрыть доступ к портам
//		if (pIOPM)
//		{
//			Ke386IoSetAccessProcess(PsGetCurrentProcess(), 0); // восстанавливаем указатель на прежнюю карту
//			Ke386SetIoAccessMap(1, pNewIO);
//			MmFreeNonCachedMemory(pNewIO, sizeof(IOPM)); // и освобождаем память
//			pNewIO = NULL;
//		}
//		break;
//}
//New Time increment proc handler
ULONG NTAPI HalSetTimeIncrementHandler(IN ULONG Increment)
{
	//DbgPrint("QUANTUM: Trying set time increment %d", Increment);
	return CurrentTimeIncrement;
}

NTSTATUS SetThreadQuantum(DWORD idThread, UCHAR NewQuantum)
{
	NTSTATUS st = STATUS_SUCCESS;
	PETHREAD Thread;

	if(OffsQuantObThread == (ULONG)-1)
		return STATUS_NOT_SUPPORTED;

	st = PsLookupThreadByThreadId((HANDLE)idThread,&Thread);

	if(!NT_SUCCESS(st))
	{
#ifdef DEBUG
		DbgPrint("QUANTUM: Not get thread object. Error : %x",st);
#endif
		return st;
	}

	//Говорим что нас не надо беспокоить и меняем квант в потоке
	KeEnterCriticalRegion();
	*(PUCHAR)((DWORD)Thread + OffsQuantObThread) = NewQuantum;
	KeLeaveCriticalRegion();
	ObDereferenceObject (Thread);

#ifdef DEBUG
	DbgPrint("QUANTUM: Quant %x be setted thread",(ULONG)NewQuantum);
#endif
	return st;
}

NTSTATUS GetThreadQuantum(DWORD idThread, PUCHAR CurQuantum)
{
	NTSTATUS st = STATUS_SUCCESS;
	PETHREAD Thread;

	if(OffsQuantObThread == (ULONG)-1)
		return STATUS_NOT_SUPPORTED;

	st = PsLookupThreadByThreadId((HANDLE)idThread,&Thread);

	if(!NT_SUCCESS(st))
	{
#ifdef DEBUG
		DbgPrint("QUANTUM: Not get thread object. Error : %x",st);
#endif
		return st;
	}

	//Говорим что нас не надо беспокоить и меняем квант в потоке
	KeEnterCriticalRegion();
	*CurQuantum = *(PUCHAR)((ULONG)Thread + OffsQuantObThread);
	KeLeaveCriticalRegion();

	ObDereferenceObject (Thread);

#ifdef DEBUG
	DbgPrint("QUANTUM: Quant %x getted from thread",(ULONG)*CurQuantum);
#endif

	return st;
}

NTSTATUS SetProcessQuantum(DWORD idProcess, UCHAR NewQuantum)
{
	NTSTATUS st = STATUS_SUCCESS;
	PEPROCESS Process;
	//KIRQL    OldIrql;
	if(OffsQuantObProcess == (ULONG)-1)
		return STATUS_NOT_SUPPORTED;

	st = PsLookupProcessByProcessId((HANDLE)idProcess,&Process);

	if(!NT_SUCCESS(st))
	{
#ifdef DEBUG
		DbgPrint("QUANTUM: Not get process object. Error : %x",st);
#endif
		return st;
	}

	//Говорим что нас не надо беспокоить и меняем квант в процессе
	KeEnterCriticalRegion();
	*(PUCHAR)((DWORD)Process + OffsQuantObProcess) = NewQuantum;//Process->Pcb.QuantumReset = ;
	KeLeaveCriticalRegion();

	ObDereferenceObject (Process);

#ifdef DEBUG
	DbgPrint("QUANTUM: Quant %x be setted in process",(ULONG)NewQuantum);
#endif
	return st;
}


NTSTATUS GetProcessQuantum(DWORD idProcess, PUCHAR CurQuantum)
{
	NTSTATUS st = STATUS_SUCCESS;
	PEPROCESS Process;
	//KIRQL    OldIrql;

	if(OffsQuantObProcess == (ULONG)-1)
		return STATUS_NOT_SUPPORTED;

	st = PsLookupProcessByProcessId((HANDLE)idProcess,&Process);

	if(!NT_SUCCESS(st))
	{
#ifdef DEBUG
		DbgPrint("QUANTUM: Not get process object. Error : %x",st);
#endif
		return st;
	}

	//Говорим что нас не надо беспокоить и меняем квант в процессе
	KeEnterCriticalRegion();

	*CurQuantum = *(PUCHAR)((DWORD)Process + OffsQuantObProcess);//Tested in my system
	KeLeaveCriticalRegion();

	ObDereferenceObject (Process);

#ifdef DEBUG
	DbgPrint("QUANTUM: Quant %x be getted from process",(ULONG)*CurQuantum);
#endif
	return st;
}


NTSTATUS SetProcessPriorityMode(DWORD idProcess, PSPROCESSPRIORITYMODE ProcessPriority)
{
	NTSTATUS st = STATUS_SUCCESS;
	PEPROCESS Process;
	//KIRQL    OldIrql;

	st = PsLookupProcessByProcessId((HANDLE)idProcess,&Process);

	if(!NT_SUCCESS(st))
	{
#ifdef DEBUG
		DbgPrint("QUANTUM: Not get process object. Error : %x",st);
#endif
		return st;
	}

	PsSetProcessPriorityByClass(Process,ProcessPriority);

	ObDereferenceObject (Process);

	return st;
}



//VOID
//KeSetQuantumProcess (
//    __inout PKPROCESS Process,
//    __in SCHAR QuantumReset
//    )
//{
//
//    KLOCK_QUEUE_HANDLE LockHandle;
//    PLIST_ENTRY NextEntry;
//    PKTHREAD Thread;
//
//    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
//
//    //
//    // Raise IRQL to SYNCH level and acquire the process lock.
//    //
//    // Set the new process quantum reset value and the quantum reset value
//    // of all child threads.
//    //
//
//    KeAcquireInStackQueuedSpinLockRaiseToSynch(&Process->ProcessLock, &LockHandle);
//    Process->QuantumReset = QuantumReset;
//    NextEntry = Process->ThreadListHead.Flink;
//    while (NextEntry != &Process->ThreadListHead) {
//        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);
//        Thread->QuantumReset = QuantumReset;
//        NextEntry = NextEntry->Flink;
//    }
//
//    //
//    // Unlock the process lock and return.
//    //
//
//    KeReleaseInStackQueuedSpinLock(&LockHandle);
//    return;
//}