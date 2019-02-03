#pragma once

#define EVIL_EMU_GAME_TANGLEWOOD		0

#define EVIL_EMU_GAME_TYPE_DEMO			0	//Include demo ROM
#define EVIL_EMU_GAME_TYPE_FINAL		1	//Include final ROM
#define EVIL_EMU_GAME_TYPE_AUTOTEST		2	//Include autotest ROM
#define EVIL_EMU_GAME_TYPE_ROMFILE		3	//Load ROM from file

#define EVIL_EMU_ROM_FILENAME			"roms/rom.bin"

#define EVIL_EMU_DISTRIBUTION_NONE		0
#define EVIL_EMU_DISTRIBUTION_STEAM		1
#define EVIL_EMU_DISTRIBUTION_ITCH		2
#define EVIL_EMU_DISTRIBUTION_GALAXY	3

#define EVIL_EMU_ROM_VERSION			"1.0.00"
#define EVIL_EMU_EMU_VERSION			"1.00"

#ifndef EVIL_EMU_GAME_TYPE
#error EVIL_EMU_GAME_TYPE NOT DEFINED, CHECK JAMFILE
#endif

#ifndef EVIL_EMU_DISTRIBUTION
#error EVIL_EMU_DISTRIBUTION NOT DEFINED, CHECK JAMFILE
#endif

//Online services client IDs/keys/secrets
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
#define EVIL_EMU_APP_ID_STEAM			0
#define EVIL_EMU_APP_ID_GALAXY			"0"
#define EVIL_EMU_APP_KEY_GALAXY			"0"
#elif EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL
#define EVIL_EMU_APP_ID_STEAM			0
#define EVIL_EMU_APP_ID_GALAXY			"0"
#define EVIL_EMU_APP_KEY_GALAXY			"0"
#endif

//UI font
#define EVIL_EMU_FONT_FILE				"fonts/OpenSans-Regular.ttf"
#define EVIL_EMU_FONT_SIZE				13

//Allow ROM download menu
#if (EVIL_EMU_DISTRIBUTION==EVIL_EMU_DISTRIBUTION_NONE)||(EVIL_EMU_DISTRIBUTION==EVIL_EMU_DISTRIBUTION_GALAXY)
#define EVIL_EMU_ROM_DOWNLOAD			0
#else
#define EVIL_EMU_ROM_DOWNLOAD			1
#endif

#define EVIL_EMU_USE_SAVES				0
#define EVIL_EMU_SAVE_DIRECTORY			"evil_emu"
#define EVIL_EMU_SAVE_FILE				"savedata.dat"
#define EVIL_EMU_SETTINGS_FILE			"settings.dat"
#define EVIL_EMU_ROM_DIRECTORY			"bin"

//Download ROM feature
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
#define EVIL_EMU_ROM_FILE				"TANGLEWD_MD_DEMO_0_9_37.BIN"
#elif EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL
#define EVIL_EMU_ROM_FILE				"TANGLEWD_MD_1_0.BIN"
#elif EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_AUTOTEST
#define EVIL_EMU_ROM_FILE				"TANGLEWD_MD_1_0.BIN"
#endif

#if defined ION_ONLINE && (EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL)
#define EVIL_EMU_ACHEVEMENTS			1
#else
#define EVIL_EMU_ACHEVEMENTS			0
#endif

#define EMU_NUM_RENDER_BUFFERS			6
#define EMU_USE_2X_BUFFER				0
#define EMU_PIXEL_ALIGN_TEST			0

#define EMU_USE_SCANLINES				1
#define EMU_SCANLINE_DEFAULT_COLOUR		ion::ColourRGB(0.0f, 0.0f, 0.0f)
#define EMU_SCANLINE_DEFAULT_ALPHA		0.15f

//Time to allow new screen resolution to settle before starting emulator
#define EMU_START_TIMER					0.5f

#if defined DEBUG
#define EMU_FULLSCREEN					0
#define EMU_INCLUDE_DEBUGGER			1
#else
#define EMU_FULLSCREEN					1
#define EMU_INCLUDE_DEBUGGER			0
#endif

#define M68K_SIZE_BYTE					1
#define M68K_SIZE_WORD					2
#define M68K_SIZE_LONG					4

enum class VideoBorder
{
	ImageGameCover,
	ImageBlackGrid,
	ImageBlueBorder,
	VDPColour,
	Black
};