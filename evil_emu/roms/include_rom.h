#pragma once

#include "constants.h"

#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
#include "roms/tanglewd_demo_rom.h"
#include "roms/tanglewd_demo_vars.h"
#elif EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL
#include "roms/tanglewd_final_rom.h"
#include "roms/tanglewd_final_vars.h"
#elif EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_AUTOTEST
#include "roms/tanglewd_test_rom.h"
#include "roms/tanglewd_test_vars.h"
#endif