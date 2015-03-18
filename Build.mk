

!ifndef CC
CC=cl
!endif

!ifndef LNK
LNK=link
!endif

!ifndef CRC
CRC = rc
!endif

RESS = $(RESOURCES:.rc=.res)
__TMP_OBJ = $(SOURCES:.c=.obj)
OBJS = $(__TMP_OBJ:.objpp=.obj) $(RESS)

!ifndef IS_EMBEDDED_MANIFEST
IS_EMBEDDED_MANIFEST = 0
!endif

#<name file>.cpp -> <name file>.obj 
.cpp.obj: 
	$(CC) $(CFLAGS) /Fo$@ $<

#<name file>.c -> <name file>.obj 
.c.obj:
	$(CC) $(CFLAGS) /Fo$@ $<

#<name file>.rc -> <name file>.res 
.res.rc:
	$(CRC) /Fo$@ $<

$(EXECUTABLE): $(OBJS)
	$(LNK) /OUT:$@ $(LFLAGS) $(OBJS) $(LIBS)
	if $(IS_EMBEDDED_MANIFEST) equ 1 (if exist $(EXECUTABLE).manifest (mt.exe -manifest $(EXECUTABLE).manifest -outputresource:$(EXECUTABLE);1))