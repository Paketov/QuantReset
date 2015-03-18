#ifndef __HIDDENNTPROC_H__
#define __HIDDENNTPROC_H__
#include <ntddk.h>

typedef enum _PSPROCESSPRIORITYMODE {
    PsProcessPriorityBackground,
    PsProcessPriorityForeground,
    PsProcessPrioritySpinning
} PSPROCESSPRIORITYMODE;

NTKERNELAPI
NTSTATUS
PsLookupThreadByThreadId(
    __in HANDLE ThreadId,
    __deref_out PETHREAD *Thread
    );

NTKERNELAPI
NTSTATUS
PsLookupProcessByProcessId(
    __in HANDLE ProcessId,
    __deref_out PEPROCESS *Process
    );

NTSTATUS
PsLookupProcessThreadByCid(
    __in PCLIENT_ID Cid,
    __deref_opt_out PEPROCESS *Process,
    __deref_out PETHREAD *Thread
    );

VOID
PsSetProcessPriorityByClass (
    __inout PEPROCESS Process,
    __in PSPROCESSPRIORITYMODE PriorityMode
    );


VOID
KeSetTimeIncrement (
    IN ULONG MaximumIncrement,
    IN ULONG MinimumIncrement
    );

ULONG NTAPI HalSetTimeIncrement(IN ULONG Increment);
#endif