#include "SrhAddressQuant.h"
#include "section.h"
/*
*	Move opcode.
*	Example:
*	MOV AL, BYTE PTR DS:[EDX + 0x492740]
*	8A82	40274900
*	where 0x8A prefix which defined: mov <byte register>, byte ptr cs:[<dword register> + <Address>],
*   	and 82 defined number <byte register> and number <dword register>.
*	Searched address located after opcode (in example 40274900). 
*/

#define MAX_SCANNING_FUNC 101

const UCHAR MovPrefixOpcode = 0x8A;
const UCHAR MovByteOpcode[] = 
{
	/*mov ..., byte ptr ds:[eax + Address]*/
	/*al	ah		bl		bh		cl		ch		dl		dh */ 
	0x80, 0xA0,   0x98,   0xB8,   0x88,		0xA8,	0x90,	0xB0,
	/*mov ..., byte ptr ds:[ebx + Address]*/
	/*al	ah		bl		bh		cl		ch		dl		dh */
	0x83, 0xA3,   0x9B,   0xBB,   0x8B,		0xAB,	0x93,	0xB3,
	/*mov ..., byte ptr ds:[ecx + Address]*/
	/*al	ah		bl		bh		cl		ch		dl		dh */
	0x81, 0xA1,   0x99,   0xB9,   0x89,	   0xA9,	0x91,	0xB1,
	/*mov ..., byte ptr ds:[edx + Address]*/
	/*al	ah		bl		bh		cl		ch		dl		dh */
	0x82,  0xA2,   0x9A,   0xBA,   0x8A,   0xAA,	0x92,   0xB2,

	/*mov ..., byte ptr ds:[esi + Address]*/
	/*al	ah		bl		bh		cl		ch		dl		dh */
	0x86,  0xA6,   0x9E,   0xBE,   0x8E,   0xAE,	0x96,   0xB6,
	/*mov ..., byte ptr ds:[edi + Address]*/
	/*al	ah		bl		bh		cl		ch		dl		dh */
	0x87,  0xA7,   0x9F,   0xBF,   0x8F,   0xAF,	0x97,   0xBF
};

/*RETN 8*/
const UCHAR CommandRetn8[] = 
{
	0xC2,		/*opcode*/
	0x08, 0x00  /*word 8*/
};

const RTC_TIME_INCREMENT TimeIncrementTable[] = 
{
	{0x26, 9766, 0x26, 0x60, 0x10},
	{0x27, 19532, 0x4B, 0xC0, 0x20},
	{0x28, 39063, 0x32, 0x80, 0x40},
	{0x29, 78125, 0, 0, 0x80},
	{0x2A, 156250, 0, 0, 0x100}
};

PVOID SearchAdressVariableInProc(CONST IN PVOID AdressProc,CONST IN PVOID MaxAddress)
{
	PUSHORT InteratorSearch = (PUSHORT)AdressProc;
	for(;InteratorSearch < MaxAddress;)
	{
		UCHAR i = 0;
		for(;i < (sizeof(MovByteOpcode) / sizeof(UCHAR));i++)
		{
			if(*InteratorSearch == (USHORT)((MovByteOpcode[i] << 8) | MovPrefixOpcode))
				return (PVOID)(InteratorSearch+1);
		}
		InteratorSearch = (PUSHORT)((ULONG)InteratorSearch + 1);
	}
	return NULL;
}

PVOID GetAddressPspForegroundQuantum()
{
	ULONG CurPrioritySeparation;
	ULONG CurQuantumTable;
	PVOID Result = NULL;

	//Getting cur priority Separation from registry
	CurPrioritySeparation = QueryPrioritySeparation();
	if(CurPrioritySeparation == (ULONG)-1)
		return NULL;

	CurQuantumTable = ComputeQuantumTable(CurPrioritySeparation);
	Result = SearchAddressPspForegroundQuantum(CurQuantumTable);
	if(Result == NULL)
		DbgPrint("QUANTUM: Quntum table not be founded !");
	return Result;
}


PVOID SearchAddressPspForegroundQuantum(IN ULONG TestValue)
{
	UNICODE_STRING NameSetPriorityByClass;
	PVOID AddressRotine;
	PVOID TestedAddress;
	PVOID BaseKernel;
	PVOID BaseDataSegment;
	PVOID EndDataSegment;
	LONG SizeScanning;
	PIMAGE_SECTION_HEADER pSection;

	RtlInitUnicodeString(&NameSetPriorityByClass, L"PsSetProcessPriorityByClass"); 

	//Getting address PsSetProcessPriorityByClass in kernel
	AddressRotine =  MmGetSystemRoutineAddress(&NameSetPriorityByClass);

	//If rotine not founded
	if(AddressRotine == NULL)
	{
		DbgPrint("QUANTUM: Address rotine \"PsSetProcessPriorityByClass\" not founded");
		return NULL;
	}

	DbgPrint("QUANTUM: Address PsSetProcessPriorityByClass: %x", AddressRotine);

	//Getting start address kernell
	BaseKernel = MmPageEntireDriver(AddressRotine);

	//If bad address
	if(BaseKernel == NULL)
	{
		DbgPrint("QUANTUM: Base kernel not getting");
		return NULL;
	}

	DbgPrint("QUANTUM: Base kernel: %x", BaseKernel);

	pSection = GetSectionByName(BaseKernel,".data");

	if(pSection)
	{
#ifdef DEBUG
		DbgPrint("QUANTUM: Virtual address .data section %x", pSection->VirtualAddress);
		DbgPrint("QUANTUM: Virtual size .data section %x", pSection->Misc.VirtualSize);
#endif
	}else
	{
		DbgPrint("QUANTUM: Not get .data section!");
		return NULL;
	}

	BaseDataSegment = (PVOID)(pSection->VirtualAddress + (DWORD)BaseKernel);
	EndDataSegment = (PVOID)((DWORD)BaseDataSegment + pSection->Misc.VirtualSize);

	TestedAddress = AddressRotine;
	SizeScanning = MAX_SCANNING_FUNC;



	for(;SizeScanning > 0;SizeScanning = MAX_SCANNING_FUNC - ((ULONG)TestedAddress - (ULONG)AddressRotine))
	{
		//Search mov byte instruction
		TestedAddress = SearchAdressVariableInProc(TestedAddress,(PVOID)((ULONG)TestedAddress + SizeScanning));
		if(TestedAddress == NULL)
			break;
		if(MmPageEntireDriver(TestedAddress) == BaseKernel)
		{
			//if this address refer in .data section
			if((*(LPDWORD)TestedAddress >= (DWORD)BaseDataSegment) && (*(LPDWORD)TestedAddress < (DWORD)EndDataSegment))
			{
				if((**(PWORD*)TestedAddress == *(PWORD)&TestValue) && (*(*(PUCHAR*)TestedAddress + 2) == *((PUCHAR)&TestValue + 2)))
				{
					DbgPrint("QUANTUM: Searched address instruction founded: %x",TestedAddress);
					return *(PUCHAR*)TestedAddress;
				}
			}
		}
	}
	//If data not equal
	return NULL;
}


ULONG QueryPrioritySeparation()
{
	HANDLE			hKey;
    	OBJECT_ATTRIBUTES	ObjAttributes;
	UNICODE_STRING		KeyPath, KeyValueName;
    	NTSTATUS		Status;
	PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
	UCHAR			Buff[20];
	ULONG			ResultLength;


	RtlInitUnicodeString(&KeyPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\PriorityControl");
	RtlInitUnicodeString(&KeyValueName, L"Win32PrioritySeparation");
	
	InitializeObjectAttributes
	(
		&ObjAttributes,
		&KeyPath,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,                 
		NULL,
		NULL
	);

	Status = ZwOpenKey(&hKey,GENERIC_READ,&ObjAttributes);

	if(!NT_SUCCESS(Status))
	{
		DbgPrint("QUANTUM: Key \"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\PriorityControl\" not opened. %x",Status);
		return (ULONG)-1;
	}

	KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)Buff;
	Status = ZwQueryValueKey
	(
		hKey,
		&KeyValueName,
		KeyValuePartialInformation,
		KeyValueInformation,
		sizeof(Buff),
		&ResultLength
	);
	
	ZwClose(hKey);

	if(!NT_SUCCESS(Status) || (KeyValueInformation->Type != REG_DWORD))
	{
		DbgPrint("QUANTUM: Value key not getted. Status: %x, Result len: %d",Status, ResultLength);
		return (ULONG)-1;
	}
	DbgPrint("QUANTUM: Value key %x ",*(PULONG)KeyValueInformation->Data);
	return *(PULONG)KeyValueInformation->Data;
}


ULONG ComputeQuantumTable (ULONG PrioritySeparation)
{
	//RtlCheckRegistryKey
	//RtlWriteRegistryValue
	//RtlCreateRegistryKey
	enum
	{ 
		THREAD_QUANTUM =6
	};
	static const  SCHAR PspFixedQuantums[6] = 
	{
		3*THREAD_QUANTUM,
		3*THREAD_QUANTUM,
		3*THREAD_QUANTUM,
		6*THREAD_QUANTUM,
		6*THREAD_QUANTUM,
		6*THREAD_QUANTUM
	};

	static const  SCHAR PspVariableQuantums[6] = 
	{
		1*THREAD_QUANTUM,
		2*THREAD_QUANTUM,
		3*THREAD_QUANTUM,
		2*THREAD_QUANTUM,
		4*THREAD_QUANTUM,
		6*THREAD_QUANTUM
	};
	ULONG RetForegroundQuantum = 0;
	SCHAR const* QuantumTableBase;

	//
	// determine if we are using fixed or variable quantums
	//

	switch (PrioritySeparation & PROCESS_QUANTUM_VARIABLE_MASK) 
	{
	case PROCESS_QUANTUM_VARIABLE_VALUE:
		QuantumTableBase = PspVariableQuantums;
		break;
	case PROCESS_QUANTUM_FIXED_VALUE:
		QuantumTableBase = PspFixedQuantums;
		break;
	case PROCESS_QUANTUM_VARIABLE_DEF:
	default:
		if (MmIsThisAnNtAsSystem ()) 
			QuantumTableBase = PspFixedQuantums;
		else 
			QuantumTableBase = PspVariableQuantums;
		break;
	}

	//
	// determine if we are using long or short
	//

	switch (PrioritySeparation & PROCESS_QUANTUM_LONG_MASK) 
	{
	case PROCESS_QUANTUM_LONG_VALUE:
		QuantumTableBase = QuantumTableBase + 3;
		break;
	case PROCESS_QUANTUM_SHORT_VALUE:
		break;
	case PROCESS_QUANTUM_LONG_DEF:
	default:
		if (MmIsThisAnNtAsSystem ()) 
			QuantumTableBase = QuantumTableBase + 3;
		break;
	}

	RtlCopyMemory (&RetForegroundQuantum, QuantumTableBase, sizeof(SCHAR) * 3);
	return RetForegroundQuantum;
}


PVOID SearchTimeTable()
{
	UNICODE_STRING NameHalSetTimeIncrement;
	PVOID HalSetTimeIncrementEntry;
	PVOID HalBase;
	PIMAGE_SECTION_HEADER pSectionHeader;
	PVOID BaseDataSegment;
	PVOID Barier;
	register ULONG j;
	//Timer period table

   	RtlInitUnicodeString(&NameHalSetTimeIncrement, L"HalSetTimeIncrement"); 
   	
	HalSetTimeIncrementEntry = MmGetSystemRoutineAddress(&NameHalSetTimeIncrement);
	if(HalSetTimeIncrementEntry == NULL)
		return NULL;
        
	HalBase = MmPageEntireDriver(HalSetTimeIncrementEntry);
	if(HalBase == NULL)
		return NULL;

	pSectionHeader = GetSectionByName(HalBase,".data");
	if(pSectionHeader == NULL)
		return NULL;

	BaseDataSegment = (PVOID)(pSectionHeader->VirtualAddress + (DWORD)HalBase);
	Barier = (PVOID)((DWORD)BaseDataSegment + pSectionHeader->SizeOfRawData - sizeof(TimeIncrementTable));
	for(;BaseDataSegment <= Barier;BaseDataSegment = (PUCHAR)BaseDataSegment + 1)
	{
	   for(j = 0;j < (sizeof(TimeIncrementTable)/sizeof(DWORD));j++)
	      if(((LPDWORD)BaseDataSegment)[j] != ((LPDWORD)TimeIncrementTable)[j])
			break;
	   if(j == (sizeof(TimeIncrementTable)/sizeof(DWORD)))
		   return BaseDataSegment;
	}
	return NULL;
}
