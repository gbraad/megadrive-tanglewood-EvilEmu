
#include <ion/core/Types.h>
#include "config.h"

#pragma once

extern U8 videoMemory[LINE_LENGTH*HEIGHT*sizeof(U32)];

enum EmulatorButtons
{
	eBtn_Up = 0,
	eBtn_Down,
	eBtn_Left,
	eBtn_Right,
	eBtn_A,
	eBtn_B,
	eBtn_C,
	eBtn_Start,
	eBtn_MAX
};

static u16 g_emulatorButtonBits[] =
{
	0x0101,
	0x0202,
	0x0400,
	0x0800,
	0x2000,
	0x1000,
	0x0010,
	0x0020
};

static const char* g_emulatorButtonNames[] =
{
	"Up",
	"Down",
	"Left",
	"Right",
	"A",
	"B",
	"C",
	"Start"
};

enum EmulatorState
{
	eState_Running,
	eState_Debugger,
	eState_Shutdown
};

bool InitialiseEmulator(const char* rom);
EmulatorState TickEmulator();
void ShutdownEmulator();
void EmulatorSetButtonState(u16 buttonState);
const char* EmulatorGetROMTitle();