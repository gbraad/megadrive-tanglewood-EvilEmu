#pragma once

//Type of game state to create
typedef class StateTanglewood GameStateType;

//Game type, defined in evil_emu.jam (evil_emu_config)
#define EVIL_EMU_GAME_TYPE_DEMO			0
#define EVIL_EMU_GAME_TYPE_FINAL		1
#define EVIL_EMU_GAME_TYPE_AUTOTEST		2

#ifndef EVIL_EMU_GAME_TYPE
#error EVIL_EMU_GAME_TYPE NOT DEFINED, CHECK EVIL_EMU.JAM
#endif

//Window title
#define EVIL_EMU_APP_TITLE				"TANGLEWOOD"

//ROM version
#define EVIL_EMU_ROM_VERSION			"2.0.00"

//EXE version
#define EVIL_EMU_EMU_VERSION			"2.00"

//Source for ROM file
#define EVIL_EMU_ROM_SOURCE_FILE		0	//Load from EVIL_EMU_ROM_FILENAME
#define EVIL_EMU_ROM_SOURCE_EMBEDDED	1	//Load from embedded binary, see roms/include_rom.h

#if defined ION_BUILD_MASTER
#define EVIL_EMU_ROM_SOURCE				EVIL_EMU_ROM_SOURCE_EMBEDDED
#else
#define EVIL_EMU_ROM_SOURCE				EVIL_EMU_ROM_SOURCE_FILE
#endif

//Filename for ROM, if loading from file
#define EVIL_EMU_ROM_FILENAME			"roms/rom.bin"

//Online services client IDs/keys/secrets
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO

	#if defined ION_SERVICES_STEAM
		#define EVIL_EMU_APP_ID			"886400"
		#define EVIL_EMU_APP_KEY		"0"
	#elif defined ION_SERVICES_GALAXY
		#define EVIL_EMU_APP_ID			"0"
		#define EVIL_EMU_APP_KEY		"0"
	#else
		#define EVIL_EMU_APP_ID			"0"
		#define EVIL_EMU_APP_KEY		"0"
	#endif

#elif EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL

	#if defined ION_SERVICES_STEAM
		#define EVIL_EMU_APP_ID			"837190"
		#define EVIL_EMU_APP_KEY		"0"
	#elif defined ION_SERVICES_GALAXY
		#define EVIL_EMU_APP_ID			"51933473431110495"
		#define EVIL_EMU_APP_KEY		"b19e764d90149c5233fb89185e91e10fe39bf1776071c133a45ace43b7d9335f"
	#elif defined ION_PLATFORM_SWITCH
		#define EVIL_EMU_APP_ID			"0x0100fe600f9f0000"
		#define EVIL_EMU_APP_KEY		"0"
	#else
		#define EVIL_EMU_APP_ID			"0"
		#define EVIL_EMU_APP_KEY		"0"
	#endif

#endif

//imgui font
#define EVIL_EMU_FONT_FILE				"fonts/tanglewood.ttf"

//imgui font size
#define EVIL_EMU_FONT_SIZE				13

//Max supported gamepads
#define EVIL_EMU_MAX_GAMEPADS			1

//Support save games
#define EVIL_EMU_USE_SAVES				1

#if !defined ION_PLATFORM_SWITCH

//Store user settings in %APPDATA% or equivalent
#define EVIL_EMU_STORE_SETTINGS			1

//If saves enable, support multiple save slots
#define EVIL_EMU_MULTIPLE_SAVESLOTS		0

#else

//Store user settings in %APPDATA% or equivalent
#define EVIL_EMU_STORE_SETTINGS			0

//If saves enable, support multiple save slots
#define EVIL_EMU_MULTIPLE_SAVESLOTS		0

#endif

//Save file directory within %APPDATA% or equivalent
#define EVIL_EMU_SAVE_DIRECTORY			"tanglewood"

//Save filename
#define EVIL_EMU_SAVE_FILE				"savedata_de.dat"

//Settings filename
#define EVIL_EMU_SETTINGS_FILE			"settings_de.dat"

//Support for ROM file download from pause menu
#if (!defined ION_PLATFORM_SWITCH) && (!defined ION_SERVICES_GALAXY)
#define EVIL_EMU_ROM_DOWNLOAD			1
#else
#define EVIL_EMU_ROM_DOWNLOAD			0
#endif

//ROM source directory for EVIL_EMU_ROM_DOWNLOAD feature
#define EVIL_EMU_ROM_DOWNLOAD_DIR		"bin"

//ROM filename for EVIL_EMU_ROM_DOWNLOAD feature
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
#define EVIL_EMU_ROM_DOWNLOAD_BIN		"TANGLEWD_MD_DEMO_0_9_37.BIN"
#elif EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL
#define EVIL_EMU_ROM_DOWNLOAD_BIN		"TANGLEWD_MD_1_0.BIN"
#elif EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_AUTOTEST
#define EVIL_EMU_ROM_DOWNLOAD_BIN		"TANGLEWD_MD_1_0.BIN"
#endif

//Support achievements (triggered via databridge system)
#if defined ION_SERVICES && (EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL)
#define EVIL_EMU_ACHEVEMENTS			1
#else
#define EVIL_EMU_ACHEVEMENTS			0
#endif

//Support host pause menu
#if defined ION_PLATFORM_SWITCH
#define EVIL_EMU_USE_PAUSE_MENU		0
#else
#define EVIL_EMU_USE_PAUSE_MENU		1
#endif

//Support user manual reader UI
#define EVIL_EMU_USE_MANUAL				0

//Support data bridge feature (EXE-to-ROM communication)
#define EVIL_EMU_USE_DATA_BRIDGE		1

//Support fetching language via data bridge
#define EVIL_EMU_LANGUAGE_SELECT		1

//Support MegaCD audio playback (experimental, selects track via databridge)
#define EVIL_EMU_CD_AUDIO				1

//Support replacement plane B
#define EVIL_EMU_HD_PLANE_B				0

//Number of render targets is using synchronised texture writes (glSync or equivalent)
#define EVIL_EMU_NUM_RENDER_BUFFERS		6

//Render output to 2x buffer before scaling to final resolution (deprecated, use VDP_SCALE_2X in megaex/config.h)
#define EVIL_EMU_USE_2X_BUFFER			0

//Render a test pattern for pixel aligment
#define EVIL_EMU_PIXEL_ALIGN_TEST		0

//Support for scanlines (experimental, currently unsupported with GLSL)
#if defined ION_PLATFORM_SWITCH
#define EVIL_EMU_USE_SCANLINES			0
#else
#define EVIL_EMU_USE_SCANLINES			1
#endif

//Default scanline colour
#define EVIL_EMU_SCANLINE_DEFAULT_COLOUR ion::ColourRGB(0.0f, 0.0f, 0.0f)

//Default scaline intensity
#define EVIL_EMU_SCANLINE_DEFAULT_ALPHA	0.15f

//Time to start emulation (allows CRT displays to settle down after resolution change)
#define EVIL_EMU_START_TIMER			0.5f

#if defined ION_BUILD_MASTER

//Start application in fullscreen mode
#define EVIL_EMU_FULLSCREEN				1

//Show debug UI
#define EVIL_EMU_INCLUDE_DEBUG_UI		0

#else

//Start application in fullscreen mode
#define EVIL_EMU_FULLSCREEN				0

//Show debug UI
#define EVIL_EMU_INCLUDE_DEBUG_UI		1

#endif

//Utility macros for DataBridge functionality
#define M68K_SIZE_BYTE					1
#define M68K_SIZE_WORD					2
#define M68K_SIZE_LONG					4

//Video border types
enum class VideoBorder
{
	ImageGameCover,
	ImageBlackGrid,
	ImageBlueBorder,
	VDPColour,
	Black
};