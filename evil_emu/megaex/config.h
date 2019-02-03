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

#define VDP_SCREEN_WIDTH		320
#define VDP_SCREEN_HEIGHT		224
#define LINE_LENGTH				((228)*4)					/* 228 / 227 alternating */
#define WIDTH 					(LINE_LENGTH)
#define HEIGHT 					(262*2)
#define BPP						(4)
#define DEPTH					(32)

#define VDP_SCALE_2X			1							// Draw to 2x sized output buffer

#define DEFAULT_SCREEN_WIDTH	VDP_SCREEN_WIDTH*2
#define DEFAULT_SCREEN_HEIGHT	VDP_SCREEN_HEIGHT*2
#define DEFAULT_SCREEN_RATIO	(float)DEFAULT_SCREEN_HEIGHT/(float)VDP_SCREEN_WIDTH

#define CLOCK_FREQUENCY_NTSC	53693175 //Hz
#define CLOCK_FREQUENCY_PAL		53203424 //Hz

#define CLOCK_DIVIDER_68K		7
#define CLOCK_DIVIDER_Z80		15
#define CLOCK_DIVIDER_FM		7
#define CLOCK_DIVIDER_PSG		15

#define FRAMES_PER_SECOND_NTSC	60
#define FRAMES_PER_SECOND_PAL	50

#define CLOCK_TICKS_PER_FRAME	CLOCK_FREQUENCY_NTSC / FRAMES_PER_SECOND_NTSC

#define LINES_PER_FRAME_NTSC	262
#define LINES_PER_FRAME_PAL		312

#define CYCLES_PER_FRAME_68K	CLOCK_TICKS_PER_FRAME / CLOCK_DIVIDER_68K
#define CYCLES_PER_LINE_68K		CYCLES_PER_FRAME_68K / LINES_PER_FRAME_NTSC

// Tick emulator 1/8th of a line at a time
#define CYCLES_PER_EMU_UPDATE_68K	CYCLES_PER_LINE_68K / 8

#define CPU_DEBUG_INFO				0
#define CPU_DEBUG_CALLTRACE			0
#define CPU_USE_JUMP_INDEX_TABLE	1
#define CPU_MAX_OPERANDS			4
#define CPU_UNROLL_OPERANDS			1
#define CPU_COMBINE_STAGES			1

#define CPU_CALLTRACE_SIZE			128

#define VDP_DMA_MEMCPY			1

#define EMU_SUPPORT_WINDOW_PLANE 1

#define PAL_PRETEND				0

#define ENABLE_DEBUGGER			0
#define DEBUG_BREAK_ON_BOOT		0

#define	ENABLE_32X_MODE			0

#define SMS_MODE				0			/* Attempt to emulate SMS using megadrive core */
#define ENABLE_SMS_BIOS			0
#define SMS_CART_MISSING		0

#define OPENAL_SUPPORT			0
#define USE_8BIT_OUTPUT			0

#define UNUSED_ARGUMENT(x)		(void)x

#include "gui/debugger.h"

#endif/*__CONFIG_H*/

