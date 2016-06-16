
#include <ion/core/Types.h>

#include "config.h"

extern U8 videoMemory[LINE_LENGTH*HEIGHT*sizeof(U32)];

enum EmulatorButton
{
	eBtn_Up = 0x0101,
	eBtn_Down = 0x0202,
	eBtn_Left = 0x0400,
	eBtn_Right = 0x0800,
	eBtn_A = 0x2000,
	eBtn_B = 0x1000,
	eBtn_C = 0x0010,
	eBtn_Start = 0x0020
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