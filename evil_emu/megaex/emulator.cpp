#include "emulator.h"

U32 Globals::lineNo = 0;
U32 Globals::colNo = 0;
unsigned char* Globals::SRAM = nullptr;
U32 Globals::SRAM_Size = 0;
U32 Globals::SRAM_StartAddress = 0;
U32 Globals::SRAM_EndAddress = 0;
U32 Globals::SRAM_OddAccess = 0;
U32 Globals::SRAM_EvenAccess = 0;
int Globals::SRAM_Lock = 0;
U8 Globals::inHBlank = 0;
U8 Globals::inVBlank = 0;
int Globals::g_newScreenNotify = 0;
int Globals::g_pause = 0;

#if EMU_USE_INPUT_CALLBACKS
Globals::InputCallback Globals::getGamepadState;
#else
U32 Globals::gamepadStates[EMU_MAX_GAMEPADS];
#endif