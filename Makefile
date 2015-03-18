#VC Makefile

CC = cl
LNK = link
CRC = rc


!ifndef DDK_PATH
DDK_PATH = C:\WinDDK\7600.16385.1\
!endif

all:
	cd Driver
	NMAKE DDK_PATH=$(DDK_PATH) CC=$(CC) LNK=$(LNK) CRC=$(CRC)
	copy /B .\setquant.sys ..\Client\setquant.sys  
	cd ..\Client
	NMAKE CC=$(CC) LNK=$(LNK) CRC=$(CRC)
	copy /B .\QuantReset.exe ..\QuantReset.exe

	
