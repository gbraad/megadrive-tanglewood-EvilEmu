
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ion/core/time/Time.h>
#include <ion/core/thread/Sleep.h>
#include <ion/maths/Maths.h>

#include "emuthread.h"

#include "config.h"
#include "emulator.h"
#include "platform.h"
#include "cpu.h"
#include "z80.h"
#include "memory.h"
#include "mgaudio.h"

#define EMU_TIMESTEP		(1.0f / (float)FRAMES_PER_SECOND_NTSC)
#define EMU_CLOCK_DIV_68K	7
#define EMU_CLOCK_DIV_Z80	15
#define EMU_CLOCK_DIV_FM	7
#define EMU_CLOCK_DIV_PSG	15
#define EMU_MAX_TICK_FRAMES	2

#if defined DEBUG
#define EMU_FRAME_SKIP		1
#else
#define EMU_FRAME_SKIP		1
#endif

ION_C_API U8 VDP_Registers[0x20];
ION_C_API void VID_DrawScreen(int lineNo);

float TEST_FPS = 0.0f;
ion::thread::CriticalSection TEST_CRIT_SEC;

EmulatorThread::EmulatorThread()
#if EMU_THREADED
	: ion::thread::Thread("Emulator")
#endif
{
	m_totalTime = 0.0f;
	m_accumTime = 0.0f;

	m_clock68K = 0;
	m_clockZ80 = 0;
	m_clockFM = 0;
	m_clockPSG = 0;

	m_inVBlank = false;
	m_inHBlank = false;
	m_lineCounter = 0xFF;
}

u64 m_frameCount = 0;
u64 m_startTicks = 0;

void EmulatorThread::Entry()
{
	float deltaTime = 0.0f;
	bool run = true;
	while (run)
	{
		u64 startTicks = ion::time::GetSystemTicks();

		TickEmulator(deltaTime);

		u64 endTicks = ion::time::GetSystemTicks();
		deltaTime = (float)ion::time::TicksToSeconds(endTicks - startTicks);
	}
}

void EmulatorThread::TickEmulator(float deltaTime)
{
	int debuggerRunning = UpdateDebugger();

	const int cyclesPerFrame = CYCLES_PER_FRAME_68K;
	const int cyclesPerLine = CYCLES_PER_LINE_68K;

	int framesBehind = 0;

	if (!debuggerRunning)
	{
		m_totalTime += deltaTime;
		m_accumTime += deltaTime;

		framesBehind = (int)ion::maths::Floor(m_accumTime / EMU_TIMESTEP);

		if (framesBehind > 1)
		{
			printf("Lagging behind by %i frames\n", framesBehind);
		}

		int framesToProcess = ion::maths::Min(framesBehind, EMU_MAX_TICK_FRAMES);

#if EMU_FRAME_SKIP
		if(framesBehind > 0)
#else
		for (int f = 0; f < framesToProcess; f++)
#endif
		{

			if (m_frameCount++ % 60 == 0)
			{
				//Get 60-frame end time and diff
				u64 endTicks = ion::time::GetSystemTicks();
				u64 diffTicks = endTicks - m_startTicks;

				//Calc frame time and frames per second
				float frameTime = (float)ion::time::TicksToSeconds(diffTicks) / 60.0f;
				float framesPerSecond = 1.0f / frameTime;

				TEST_CRIT_SEC.Begin();
				TEST_FPS = framesPerSecond;
				TEST_CRIT_SEC.End();

				//Reset timer
				m_startTicks = ion::time::GetSystemTicks();
			}

			m_renderCritSec.Begin();

			m_accumTime -= EMU_TIMESTEP;

			//Process one frame at a time
			for (int i = 0; i < cyclesPerFrame; i++)
			{
				m_clock68K += EMU_CLOCK_DIV_68K;
				m_clockZ80 += EMU_CLOCK_DIV_68K;
				m_clockFM += EMU_CLOCK_DIV_68K;
				m_clockPSG += EMU_CLOCK_DIV_68K;

				CPU_Step();

				if (m_clockZ80 > EMU_CLOCK_DIV_Z80)
				{
					Z80_Step();
					m_clockZ80 -= EMU_CLOCK_DIV_Z80;
				}

				if (m_clockFM > EMU_CLOCK_DIV_FM)
				{
					AudioFMUpdate();
					m_clockFM -= EMU_CLOCK_DIV_FM;
				}

				if (m_clockPSG > EMU_CLOCK_DIV_PSG)
				{
					AudioPSGUpdate();
					m_clockPSG -= EMU_CLOCK_DIV_PSG;
				}

				int lineNo = i / cyclesPerLine;
				int colNo = i % cyclesPerLine;

				m_inVBlank = (lineNo > 223);
				m_inHBlank = (colNo > ((cyclesPerLine * 3) / 4));

				if (colNo == ((cyclesPerLine * 3) / 4))
				{
					if (lineNo > 224)
					{
						m_lineCounter = VDP_Registers[0x0A];
					}
					else
					{
						m_lineCounter--;
						if (m_lineCounter == 0xFFFFFFFF)
						{
							m_lineCounter = VDP_Registers[0x0A];
							CPU_SignalInterrupt(4);
						}
					}

					int displaySizeY = (VDP_Registers[1] & 0x08) ? 30 * 8 : 28 * 8;
					if (lineNo < displaySizeY)
					{
						VID_DrawScreen(lineNo);
					}
				}

				if ((lineNo == 225) && (colNo == 8))
				{
					CPU_SignalInterrupt(6);
					Z80_SignalInterrupt(0);
				}

				AudioTick(deltaTime);
			}

			m_renderCritSec.End();

			//Catch up audio
			for (int i = 0; i < ion::maths::Max(0, framesBehind - framesToProcess); i++)
			{
				AudioTick(deltaTime);
			}
		}
	}

	if (framesBehind == 0)
	{
		ion::thread::Sleep(1);
	}

	m_lastEmulatorState = debuggerRunning ? eState_Debugger : eState_Running;
}