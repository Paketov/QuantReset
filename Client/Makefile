CFLAGS = \
	/O2 \
	/Oi \
	/GL \
	/D "WIN32" \
	/D "NDEBUG" \
	/D "_WINDOWS" \
	/D "_UNICODE" \
	/D "UNICODE" \
	/I "Client/" \
	/FD \
	/EHsc \
	/MT \
	/Zp1 \
	/GS- \
	/Gy \
	/nologo \
	/c \
	/Zi \
	/TP \
	/W0

LFLAGS =  \
	/INCREMENTAL:NO \
	/NOLOGO \
	/MANIFESTUAC:"level='requireAdministrator' uiAccess='false'" \
	/SUBSYSTEM:WINDOWS \
	/OPT:REF \
	/OPT:ICF \
	/LTCG \
	/DYNAMICBASE \
	/NXCOMPAT \
	/MACHINE:X86 \
	/ERRORREPORT:PROMPT

RESFLAGS = \
	/d "_UNICODE" \
	/d "UNICODE"

LIBS = \
	kernel32.lib \
	user32.lib \
	gdi32.lib \
	winspool.lib \
	comdlg32.lib \
	advapi32.lib \
	shell32.lib \
	ole32.lib \
	oleaut32.lib \
	uuid.lib \
	odbc32.lib \
	odbccp32.lib
	

SOURCES = \
	ProcessThread.cpp \
	iodriver.cpp \
	load_driver.cpp \
	main.cpp \
	mylib.cpp 

RESOURCES = \
	main.rc

EXECUTABLE = QuantReset.exe


IS_EMBEDDED_MANIFEST = 1

include ..\Build.mk