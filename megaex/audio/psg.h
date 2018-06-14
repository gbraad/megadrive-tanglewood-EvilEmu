#ifndef _PSG_H
#define _PSG_H

#include "mytypes.h"
#include "callbacks.h"

ION_C_API void PSG_UpdateTones();
ION_C_API void PSG_UpdateNoise();
ION_C_API U16 PSG_Output();
ION_C_API void PSG_Write(U8 data);

#endif
