
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

#define EMU_TIMESTEP			(1.0f / (float)FRAMES_PER_SECOND_NTSC)
#define EMU_CLOCK_DIV_68K		7
#define EMU_CLOCK_DIV_Z80		15
#define EMU_CLOCK_DIV_FM		7
#define EMU_CLOCK_DIV_PSG		15
#define EMU_MAX_TICK_FRAMES		2

//Threading
#define EMU_LOCKSTEP_THREADS	1
#define EMU_THREAD_SLEEP_68K	1
#define EMU_THREAD_SLEEP_Z80	2

#if defined DEBUG
#define EMU_FRAME_SKIP		1
#else
#define EMU_FRAME_SKIP		0
#endif

#if defined DEBUG
#define EMU_DEBUG_OUTPUT	1
#else
#define EMU_DEBUG_OUTPUT	0
#endif

ION_C_API U8 VDP_Registers[0x20];
ION_C_API void VID_DrawScreenRow(int lineNo);

float TEST_FPS = 0.0f;
ion::thread::CriticalSection TEST_CRIT_SEC;

EmulatorThread::EmulatorThread()
#if EMU_THREADED
	: ion::thread::Thread("Emulator_68K")
	, m_Z80_PSG_FM_TickSema(1)
	, m_Z80_PSG_FM_WaitSema(1)
	, m_emuThread_Z80_PSG_FM(*this)
#endif
{
	m_prevAudioClock = 0.0f;
	m_accumTime = 0.0f;

	m_clock68K = 0;

	m_inVBlank = false;
	m_inHBlank = false;
	m_lineCounter = 0xFF;
}

u64 m_frameCount = 0;
u64 m_startTicks = 0;

void EmulatorThread::Entry()
{
	m_emuThread_Z80_PSG_FM.Run();
	m_emuThread_Z80_PSG_FM.SetPriority(ion::thread::Thread::Priority::High);

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
		//Begin audio playback
		AudioBeginPlayback();

		//Get audio clock
		const float audioClock = AudioGetClock();
		const float audioClockDelta = audioClock - m_prevAudioClock;
		m_prevAudioClock = audioClock;
		m_accumTime += audioClockDelta;

		framesBehind = (int)ion::maths::Floor(m_accumTime / EMU_TIMESTEP);

#if EMU_DEBUG_OUTPUT
		if (framesBehind > 1)
		{
			printf("Emulator lagging behind by %i frames (audio clock: %.4f prev clock: %.4f audio delta: %.4f accum time: %.4f)\n",
				framesBehind, audioClock, m_prevAudioClock, audioClockDelta, m_accumTime);
		}
#endif

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

#if EMU_THREADED && EMU_LOCKSTEP_THREADS
			//Signal Z80, PSG, and FM to tick
			m_Z80_PSG_FM_TickSema.Signal();
#endif

			m_renderCritSec.Begin();

			m_accumTime -= EMU_TIMESTEP;

			//Process one frame at a time
			for (int i = 0; i < cyclesPerFrame; i++)
			{
				m_clock68K += EMU_CLOCK_DIV_68K;

				CPU_Step();

#if !EMU_THREADED
				//Tick Z80, PSG, and FM
				m_emuThread_Z80_PSG_FM.Tick_Z80_PSG_FM(deltaTime);
#endif

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
						VID_DrawScreenRow(lineNo);
					}
				}

				if ((lineNo == 225) && (colNo == 8))
				{
					CPU_SignalInterrupt(6);
					Z80_SignalInterrupt(0);
				}
			}

			m_renderCritSec.End();

#if EMU_THREADED
			ion::thread::Sleep(EMU_THREAD_SLEEP_68K);
#endif

#if EMU_THREADED && EMU_LOCKSTEP_THREADS
			if (framesBehind <= 1)
			{
				//Wait for Z80/PSG/FM
				m_Z80_PSG_FM_WaitSema.Wait();
			}
#endif
		}
	}

	m_lastEmulatorState = debuggerRunning ? eState_Debugger : eState_Running;
}

EmulatorThread_Z80_PSG_FM::EmulatorThread_Z80_PSG_FM(EmulatorThread& emuThread68K)
#if EMU_THREADED
	: ion::thread::Thread("Emulator_Z80_PSG_FM")
	, m_emuThread68K(emuThread68K)
#endif
{
	m_clockZ80 = 0;
	m_clockFM = 0;
	m_clockPSG = 0;
}

void EmulatorThread_Z80_PSG_FM::Entry()
{
	float deltaTime = 0.0f;
	bool run = true;
	while (run)
	{
		u64 startTicks = ion::time::GetSystemTicks();

		Tick_Z80_PSG_FM(deltaTime);

		u64 endTicks = ion::time::GetSystemTicks();
		deltaTime = (float)ion::time::TicksToSeconds(endTicks - startTicks);
	}
}

void EmulatorThread_Z80_PSG_FM::Tick_Z80_PSG_FM(float deltaTime)
{
#if EMU_THREADED && EMU_LOCKSTEP_THREADS
	m_emuThread68K.m_Z80_PSG_FM_TickSema.Wait();
#endif

#if EMU_THREADED
	for (int i = 0; i < CYCLES_PER_FRAME_68K; i++)
#endif
	{
		m_clockZ80 += EMU_CLOCK_DIV_68K;
		m_clockFM += EMU_CLOCK_DIV_68K;
		m_clockPSG += EMU_CLOCK_DIV_68K;

		if (m_clockZ80 >= EMU_CLOCK_DIV_Z80)
		{
			Z80_Step();
			m_clockZ80 -= EMU_CLOCK_DIV_Z80;
		}

		if (m_clockFM >= EMU_CLOCK_DIV_FM)
		{
			AudioFMUpdate();
			m_clockFM -= EMU_CLOCK_DIV_FM;
		}

		if (m_clockPSG >= EMU_CLOCK_DIV_PSG)
		{
			AudioPSGUpdate();
			m_clockPSG -= EMU_CLOCK_DIV_PSG;
		}

		AudioTick(deltaTime);
	}

#if EMU_THREADED && EMU_LOCKSTEP_THREADS
	m_emuThread68K.m_Z80_PSG_FM_WaitSema.Signal();
#endif

#if EMU_THREADED
	ion::thread::Sleep(EMU_THREAD_SLEEP_Z80);
#endif
}