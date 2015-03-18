#ifndef __LOAD_DRIVER_H__
#define __LOAD_DRIVER_H__
#include <windef.h>

bool SetPrivilege();
bool UnloadDriver(IN PTCHAR DriverName);
bool LoadDriver(IN PTCHAR DriverPatch,IN PTCHAR DriverName);
bool DeleteDriverFromReg(IN PTCHAR DriverName);
#endif