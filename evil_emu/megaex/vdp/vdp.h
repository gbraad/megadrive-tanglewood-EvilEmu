#pragma once

#include "globals.h"
#include "config.h"

void VDP_GetRegisterContents(U16 offset, char *buffer);
U32 VDP_GetBackgroundColourRGBA();
void VID_BeginFrame();
void VID_DrawScreenRow(int lineNo);

const char *VDP_DumpRegisterName(U16 byte);
const char *SMS_VDP_DumpRegisterName(U16 byte);

class VDP
{
public:
	static int cramCacheDirty;
	static U8 VDP_Registers[0x20];

#if VDP_SCALE_2X
	static U8 videoMemory[LINE_LENGTH*HEIGHT * sizeof(U32) * 4];
#else
	static U8 videoMemory[LINE_LENGTH*HEIGHT * sizeof(U32)];
#endif
};