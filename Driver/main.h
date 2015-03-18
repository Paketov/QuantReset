#ifndef __MAIN_H__
#define __MAIN_H__



NTSTATUS NTAPI DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
VOID DriverUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS DispatchCreateClose( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchWrite( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchRead( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
PVOID SearchAdressVariableInProc(PVOID AdressProc,PVOID MaxAddress);
PVOID SearchAddressPspForegroundQuantum(IN ULONG TestValue);

extern UCHAR OldForegroundQuantums[3];
extern PUCHAR PspForegroundQuantum;
extern BOOLEAN isChangedQuantum;
extern BOOLEAN isChangedTimerResolution;
#endif