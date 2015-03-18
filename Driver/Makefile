

CFLAGS = \
	/O2 \
	/Oi \
	/I "$(DDK_PATH)\inc\ddk\wxp" \
	/I "$(DDK_PATH)\inc\ifs\wxp" \
	/I "$(DDK_PATH)\inc\wxp" \
	/I "$(DDK_PATH)\inc\api" \
	/I "$(DDK_PATH)\inc\ddk" \
	/I "$(DDK_PATH)\inc\wdf\kmdf\1.9" \
	/I "$(DDK_PATH)\inc\crt" \
	/I "Driver/" \
	/D "WIN32" \
	/D "_WINDOWS" \
	/D "i386=1" \
	/D "_X86_=1" \
	/D "STD_CALL" \
	/D "CONDITION_HANDLING=1" \
	/D "_WINDLL" \
	/GF \
	/FD \
	/EHsc \
	/MD \
	/Zp8 \
	/GS- \
	/Gy \
	/GR- \
	/W0 \
	/nologo \
	/c \
	/Zi \
	/Gz \
	/TC \
	/errorReport:prompt

LFLAGS = \
	/INCREMENTAL:NO \
	/NOLOGO \
	/LIBPATH:"$(DDK_PATH)\lib\wxp\i386" \
	/DLL \
	/MANIFEST:NO \
	/NODEFAULTLIB \
	/SUBSYSTEM:NATIVE \
	/DRIVER \
	/OPT:REF \
	/OPT:ICF \
	/ENTRY:"DriverEntry@8" \
	/BASE:"0x10000" \
	/MACHINE:X86 \
	/ERRORREPORT:PROMPT \


RESFLAGS = 

LIBS = \
	ntoskrnl.lib \
	int64.lib \
	Hal.lib

SOURCES = \
	main.c \
	section.c \
	SrhAddressQuant.c \
	io.c

RESOURCES = \
	setquant.rc


EXECUTABLE = setquant.sys

include ..\Build.mk