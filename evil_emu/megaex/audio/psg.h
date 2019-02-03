#ifndef _PSG_H
#define _PSG_H

#include "mytypes.h"
#include "callbacks.h"

void PSG_UpdateTones();
void PSG_UpdateNoise();
U16 PSG_Output();
void PSG_Write(U8 data);

#endif
