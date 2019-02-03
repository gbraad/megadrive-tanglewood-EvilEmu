
#include <ion/core/Types.h>
#include "config.h"

#pragma once

#define EMU_THREADED 1

class Globals
{
public:
	static U32 lineNo;
	static U32 colNo;

	static unsigned char* SRAM;
	static U32 SRAM_Size;
	static U32 SRAM_StartAddress;
	static U32 SRAM_EndAddress;
	static U32 SRAM_OddAccess;
	static U32 SRAM_EvenAccess;

	static U8 inHBlank;
	static U8 inVBlank;

	static int g_newScreenNotify;

	static U16 keyStatusJoyA;

	static int g_pause;
};

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
	0x0010,
	0x1000,
	0x2000,
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
bool InitialiseEmulator(const unsigned char* romData, U32 romSize);
void ShutdownEmulator();
void EmulatorSetButtonState(u16 buttonState);
const char* EmulatorGetROMTitle();
