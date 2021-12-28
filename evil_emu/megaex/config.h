/*
 * config.h

Copyright (c) 2011 Lee Hammerton

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

 */

#ifndef __CONFIG_H
#define __CONFIG_H

#if VDP_H54_MODE
#define VDP_SCREEN_WIDTH		448
#define VDP_SCREEN_WIDTH_TILES	54
#elif VDP_H50_MODE
#define VDP_SCREEN_WIDTH		400
#define VDP_SCREEN_WIDTH_TILES	50
#elif VDP_H106_MODE
#define VDP_SCREEN_WIDTH		848
#define VDP_SCREEN_WIDTH_TILES	106
#else
#define VDP_SCREEN_WIDTH		320
#define VDP_SCREEN_WIDTH_TILES	40
#endif

#define VDP_SCREEN_HEIGHT		224
#define VDP_SCREEN_HEIGHT_TILES	28

#define VDP_SPRITE_BORDER_X		128
#define VDP_SPRITE_BORDER_Y		128

#define DRAW_BUFFER_WIDTH		(VDP_SCREEN_WIDTH)
#define DRAW_BUFFER_HEIGHT		(VDP_SCREEN_HEIGHT)
#define DRAW_BUFFER_BPP			(4)
#define DRAW_BUFFER_DEPTH		(32)

#define VDP_SCALE_2X			1							// Draw to 2x sized output buffer
#define VDP_NUM_BUFFERS			2							// Buffered drawing

#define DEFAULT_WINDOW_WIDTH	(VDP_SCREEN_WIDTH*2)
#define DEFAULT_WINDOW_HEIGHT	(VDP_SCREEN_HEIGHT*2)
#define DEFAULT_WINDOW_RATIO	((float)VDP_SCREEN_WIDTH/(float)VDP_SCREEN_HEIGHT)

#define CLOCK_FREQUENCY_NTSC	53693175 //Hz
#define CLOCK_FREQUENCY_PAL		53203424 //Hz

#define CLOCK_DIVIDER_68K		7
#define CLOCK_DIVIDER_Z80		15
#define CLOCK_DIVIDER_FM		7
#define CLOCK_DIVIDER_PSG		15

#define FRAMES_PER_SECOND_NTSC	60
#define FRAMES_PER_SECOND_PAL	50

#define LINES_PER_FRAME_NTSC	262
#define LINES_PER_FRAME_PAL		312

#define CYCLES_PER_SECOND_68K	(CLOCK_FREQUENCY_NTSC / CLOCK_DIVIDER_68K)
#define CYCLES_PER_FRAME_68K	((CLOCK_FREQUENCY_NTSC / CLOCK_DIVIDER_68K) / FRAMES_PER_SECOND_NTSC)
#define CYCLES_PER_LINE_68K		(((CLOCK_FREQUENCY_NTSC / CLOCK_DIVIDER_68K) / FRAMES_PER_SECOND_NTSC) / LINES_PER_FRAME_NTSC)

#define CPU_DEBUG_INFO				0
#define CPU_DEBUG_CALLTRACE			0
#define CPU_USE_JUMP_INDEX_TABLE	1
#define CPU_MAX_OPERANDS			4
#define CPU_UNROLL_OPERANDS			1
#define CPU_COMBINE_STAGES			1

#define CPU_CALLTRACE_SIZE			128

#define Z80_DELAYED_INTERRUPTS		1	//Delay enabling of interrupts for 2 cycles
#define Z80_DISCARD_MISSED_INTS		0	//Discard any missed interrupts during delayed cycles

#define EMU_CLEAR_MEMORY			1	//Clear all RAM (68K RAM, Z80 RAM, VRAM, CRAM, VSRAM, SRAM) on startup

#define VDP_DMA_MEMCPY				1

#define EMU_SUPPORT_WINDOW_PLANE	1

#define PAL_PRETEND					0

#if defined ION_BUILD_DEBUG
#define EMU_ENABLE_68K_DEBUGGER		1
#else
#define EMU_ENABLE_68K_DEBUGGER		0
#endif

#define DEBUG_BREAK_ON_BOOT			0

#define EMU_ENABLE_AUDIO			1

#define EMU_THREADED				1

#if EMU_ENABLE_68K_DEBUGGER || defined ION_BUILD_DEBUG
#define EMU_USE_INPUT_CALLBACKS		0
#else
#define EMU_USE_INPUT_CALLBACKS		1
#endif

#define EMU_MAX_GAMEPADS            4
#define EMU_INPUT_UPDATE_MS			5

#define EMU_SAVE_SRAM				0

#if defined ION_PLATFORM_SWITCH
#define EMU_THREAD_CORE_68K			1
#define EMU_THREAD_CORE_Z80			2
#define EMU_THREAD_CORE_AUDIO		0
#endif

#define	ENABLE_32X_MODE				0

#define SMS_MODE					0			/* Attempt to emulate SMS using megadrive core */
#define ENABLE_SMS_BIOS				0
#define SMS_CART_MISSING			0

#define OPENAL_SUPPORT				0
#define USE_8BIT_OUTPUT				0

#define UNUSED_ARGUMENT(x)			(void)x

#if defined ION_BUILD_MASTER
#define EMU_PRINTF(fmt, ...) {}
#else
#define EMU_PRINTF printf
#endif

#include "gui/debugger.h"

#endif/*__CONFIG_H*/

