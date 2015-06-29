#include <ntddk.h>
#include "main.h"
#include "io.h"

UNICODE_STRING DeviceName;
UNICODE_STRING SymbolicLinkName;
PDEVICE_OBJECT deviceObject = NULL;

UCHAR OldForegroundQuantums[3];
PUCHAR PspForegroundQuantum = NULL;
BOOLEAN isChangedQuantum = FALSE;
BOOLEAN isChangedTimerResolution = FALSE;


VOID DriverUnload(IN PDRIVER_OBJECT DriverObject) 
{
	DbgPrint("QUANTUM: Driver unload;");
	if(isChangedQuantum)
	{
		//Upper irql
		KeEnterCriticalRegion();
		RtlCopyMemory(PspForegroundQuantum,OldForegroundQuantums,sizeof(OldForegroundQuantums));
		KeLeaveCriticalRegion();
	}
	ReestablishTimerTable();
	UnlockTimerIncrement();
	if(isChangedTimerResolution)
		ExSetTimerResolution(0,FALSE);

	IoDeleteSymbolicLink(&SymbolicLinkName); // óäàëÿåì ñèìâîëè÷åñêóþ ññûëêó
	IoDeleteDevice(deviceObject);            // óäàëÿåì óñòðîéñòâî return;
}

NTSTATUS DispatchCreateClose( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION piosl; 
	PCSTR              Data;
	piosl = IoGetCurrentIrpStackLocation(Irp);
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchWrite( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION piosl; 
	NTSTATUS           ns = STATUS_SUCCESS;

	piosl = IoGetCurrentIrpStackLocation(Irp);
	__try
	{
		//Ïðîâåðÿåì âõîäíîé áóôåð íà êîñÿêè
		ProbeForRead(Irp->UserBuffer,piosl->Parameters.Write.Length,sizeof(UCHAR));
	}__except(EXCEPTION_EXECUTE_HANDLER)            
	{
		Irp->IoStatus.Information = 0;
		ns = GetExceptionCode();
	}

	if(ns == STATUS_SUCCESS)
		ns = InUserDataHandler(Irp->UserBuffer, piosl->Parameters.Write.Length,&Irp->IoStatus.Information);

	Irp->IoStatus.Status = ns;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return ns;
}

NTSTATUS DispatchRead( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{

	ULONG SizeWritten;
	NTSTATUS           ns = STATUS_SUCCESS;
	PIO_STACK_LOCATION piosl;

	piosl = IoGetCurrentIrpStackLocation(Irp);

	__try
	{
		//Ïðîâåðÿåì âõîäíîé áóôåð íà êîñÿêè
		ProbeForRead(Irp->UserBuffer,piosl->Parameters.Read.Length,sizeof(UCHAR));
	}__except(EXCEPTION_EXECUTE_HANDLER)            
	{
		ns = STATUS_IN_PAGE_ERROR;
		Irp->IoStatus.Information = 0;
	}
	if(ns == STATUS_SUCCESS)
		ns = OutUserDataHandler(Irp->UserBuffer, piosl->Parameters.Read.Length, &(Irp->IoStatus.Information));
	Irp->IoStatus.Status = ns;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS st;
	PCWSTR   dDeviceName = L"\\Device\\QuantumDriver";
	PCWSTR   dSymbolicLinkName = L"\\DosDevices\\QuantumDriver";
	PVOID    pAddressPspForegroundQuantum;

	DbgPrint("QUANTUM: Driver loaded;");
	ComputeSlideQuntumsInOb();

	RtlInitUnicodeString(&DeviceName, dDeviceName);
	RtlInitUnicodeString(&SymbolicLinkName, dSymbolicLinkName); 
	st = IoCreateDevice
	(
		DriverObject,       // óêàçàòåëü íà DriverObject
		0,                  // ðàçìåð ïàìÿòè (device extension)
		&DeviceName,        // èìÿ ñîçäàâàåìîãî óñòðîéñòâà
		FILE_DEVICE_NULL,   // òèï ñîçäàâàåìîãî óñòðîéñòâà
		0,                  // õàðàêòåðèñòèêè óñòðîéñòâà
		FALSE,              // "ýêñêëþçèâíîå" óñòðîéñòâî
		&deviceObject       // óêàçàòåëü íà îáúåêò óñòðîéñòâà
	);

	if((st == STATUS_SUCCESS))
		st = IoCreateSymbolicLink(&SymbolicLinkName,&DeviceName);      // èìÿ óñòðîéñòâà
	else
		DbgPrint("QUANTUM: Not create device !");

	if(st != STATUS_SUCCESS)
		DbgPrint("QUANTUM: Not create Symbolic Link !");

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchWrite;
	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	DriverObject->DriverUnload = DriverUnload;

	return st;
}
