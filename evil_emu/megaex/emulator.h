#pragma once

#include <ion/core/Types.h>
#include "config.h"

#if EMU_USE_INPUT_CALLBACKS
#include <functional>
#endif

class Globals
{
public:
#if EMU_USE_INPUT_CALLBACKS
	typedef std::function<U16(int gamepadIdx, bool latch)> InputCallback;
#endif

	static U32 lineNo;
	static U32 colNo;

	static unsigned char* SRAM;
	static U32 SRAM_Size;
	static U32 SRAM_StartAddress;
	static U32 SRAM_EndAddress;
	static U32 SRAM_OddAccess;
	static U32 SRAM_EvenAccess;
	static int SRAM_Lock;

	static U8 inHBlank;
	static U8 inVBlank;

	static int g_newScreenNotify;

#if EMU_USE_INPUT_CALLBACKS
	static InputCallback getGamepadState;
#else
	static U32 gamepadStates[EMU_MAX_GAMEPADS];
#endif

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
	eBtn_X,
	eBtn_Y,
	eBtn_Z,
	eBtn_Mode,
	eBtn_MAX
};

static u32 g_emulatorButtonBits[] =
{
	//0xAABBCCDD
	// AA = 6-button, latch hi
	// BB = 6-button, latch lo
	// CC = 3-button, latch hi
	// DD = 3-button, latch lo

	0x00000101, //Up
	0x00000202, //Down
	0x00000400, //Left
	0x00000800, //Right
	0x00100010, //A
	0x10001000, //B
	0x20002000, //C
	0x00200020, //Start
	0x04000000, //X
	0x02000000, //Y
	0x01000000, //Z
	0x08000000, //Mode
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
	"Start",
	"X",
	"Y",
	"Z",
	"Mode"
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
const char* EmulatorGetROMTitle();

#if !EMU_USE_INPUT_CALLBACKS
void EmulatorSetButtonState(int gamepadIdx, u16 buttonState);
#endif
