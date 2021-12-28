#pragma once

#include "globals.h"
#include "config.h"

#include <ion/core/thread/Event.h>

void VDP_Init();
void VDP_Shutdown();
void VDP_WriteLock();
void VDP_WriteUnlock();
U8* VDP_ReadLock();
void VDP_ReadUnlock();
void VDP_GetRegisterContents(U16 offset, char *buffer);
U32 VDP_GetBackgroundColourRGBA();
void VID_BeginFrame();
void VID_DrawScreenRow(int lineNo);
void VDP_SetDrawPlaneB(bool drawPlaneB);
void VDP_SetTransparentBG(bool transparentBG);
U32 VDP_GetPlaneWidthPx();
S32 VDP_GetScrollX_PlaneB(int y);
S32 VDP_GetScrollY_PlaneB();

const char *VDP_DumpRegisterName(U16 byte);
const char *SMS_VDP_DumpRegisterName(U16 byte);

class VDP
{
public:
	static int cramCacheDirty;
	static U8 VDP_Registers[0x20];

	static U8* videoMemory;
	static U8* videoBuffers[VDP_NUM_BUFFERS];
	static int readBufferIdx;
	static int writeBufferIdx;

	static bool drawPlaneB;
	static bool transparentBG;

	static ion::thread::Event readBufferReady;
};