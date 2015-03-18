#ifndef __SRC_ADDRESS_QUANT_H__
#define __SRC_ADDRESS_QUANT_H__
#include <ntddk.h>
#include <windef.h>


#define PROCESS_PRIORITY_SEPARATION_MASK    0x00000003
#define PROCESS_PRIORITY_SEPARATION_MAX     0x00000002
#define PROCESS_QUANTUM_VARIABLE_MASK       0x0000000c
#define PROCESS_QUANTUM_VARIABLE_DEF        0x00000000
#define PROCESS_QUANTUM_VARIABLE_VALUE      0x00000004
#define PROCESS_QUANTUM_FIXED_VALUE         0x00000008
#define PROCESS_QUANTUM_LONG_MASK           0x00000030
#define PROCESS_QUANTUM_LONG_DEF            0x00000000
#define PROCESS_QUANTUM_LONG_VALUE          0x00000010
#define PROCESS_QUANTUM_SHORT_VALUE         0x00000020

typedef struct 
{
	unsigned    RTCRegisterA;
	unsigned    ClockRateIn100ns;
	unsigned    unknown2;
	unsigned    ClockRateAdjustment;
	unsigned    IpiRate;
} RTC_TIME_INCREMENT, *LPRTC_TIME_INCREMENT;
extern const RTC_TIME_INCREMENT TimeIncrementTable[];

PVOID SearchAdressVariableInProc(CONST IN PVOID AdressProc,CONST IN PVOID MaxAddress);
PVOID SearchAddressPspForegroundQuantum(IN ULONG TestValue);
PVOID GetAddressPspForegroundQuantum();
ULONG QueryPrioritySeparation();
ULONG ComputeQuantumTable (ULONG PrioritySeparation);
PVOID SearchTimeTable();
#endif