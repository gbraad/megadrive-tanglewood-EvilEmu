
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

#include "cpu_ops.inl"

#define EMU_TIMESTEP			(1.0 / (double)FRAMES_PER_SECOND_NTSC)
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
#define EMU_DEBUG_OUTPUT	1
#else
#define EMU_DEBUG_OUTPUT	0
#endif

ION_C_API U8 VDP_Registers[0x20];
ION_C_API void VID_DrawScreenRow(int lineNo);

EmulatorThread::EmulatorThread()
#if EMU_THREADED
	: ion::thread::Thread("Emulator_68K")
	, m_emuThread_Z80_PSG_FM(*this)
#endif
{
	m_prevAudioClock = 0.0f;
	m_accumTime = 0.0f;
	m_prevFramesBehind = 0;

	m_tickCount_68K = 0;
	m_tickCount_Z80_PSG_FM = 0;

	m_clock68K = 0;

	m_inVBlank = false;
	m_inHBlank = false;
	m_lineCounter = 0xFF;

	m_emulatorFrameCount = 0;
}

void EmulatorThread::Entry()
{
	m_emuThread_Z80_PSG_FM.Run();
	m_emuThread_Z80_PSG_FM.SetPriority(ion::thread::Thread::Priority::Critical);

	float deltaTime = 0.0f;
	bool run = true;
	while (run)
	{
		u64 startTicks = ion::time::GetSystemTicks();

		TickEmulator(deltaTime);
		ion::thread::Sleep(1);

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
		const double audioClock = AudioGetClock();
		const double audioClockDelta = audioClock - m_prevAudioClock;
		m_prevAudioClock = audioClock;
		m_accumTime += audioClockDelta;

		framesBehind = (int)ion::maths::Floor(m_accumTime / EMU_TIMESTEP);

		//If lagging behind, only draw even frames
		bool drawFrame = (framesBehind <= 1) || ((m_emulatorFrameCount & 1) == 0);

		if(framesBehind > 0)
		{
			ion::thread::atomic::Increment(m_tickCount_68K);

			m_renderCritSec.Begin();

			//Calculate emulator tick FPS
			m_fpsCounterEmulator.Update();

			//Calculate render FPS
			if (drawFrame)
			{
				m_fpsCounterRender.Update();
			}

			m_accumTime -= EMU_TIMESTEP;

			//Process one frame at a time
			for (int i = 0; i < cyclesPerFrame; i++)
			{
				m_clock68K += EMU_CLOCK_DIV_68K;

				CPU_Step();

#if !EMU_THREADED
				//Tick Z80, PSG, and FM
				m_emuThread_Z80_PSG_FM.Tick_Z80_PSG_FM();
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

					if (drawFrame)
					{
						int displaySizeY = (VDP_Registers[1] & 0x08) ? 30 * 8 : 28 * 8;
						if (lineNo < displaySizeY)
						{
							VID_DrawScreenRow(lineNo);
						}
					}
				}

				if ((lineNo == 225) && (colNo == 8))
				{
					CPU_SignalInterrupt(6);
					Z80_SignalInterrupt(0);
				}
			}

			m_renderCritSec.End();
		}

		//Wait for audio
		while (m_tickCount_68K > m_tickCount_Z80_PSG_FM)
		{
			ion::thread::Sleep(1);
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
	m_prevAudioClock = 0.0f;
	m_accumTime = 0.0f;

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
		Tick_Z80_PSG_FM();
		ion::thread::Sleep(1);
	}
}

void EmulatorThread_Z80_PSG_FM::Tick_Z80_PSG_FM()
{
#if EMU_THREADED
	//Get audio clock
	const double audioClock = AudioGetClock();
	const double audioClockDelta = audioClock - m_prevAudioClock;
	m_prevAudioClock = audioClock;
	m_accumTime += audioClockDelta;

	int framesBehind = (int)ion::maths::Floor(m_accumTime / EMU_TIMESTEP);

	if (framesBehind > 1)
	{
		printf("Audio %i frames behind\n", framesBehind);
	}

	for(int frame = 0; frame < framesBehind; frame++)
	{
		ion::thread::atomic::Increment(m_emuThread68K.m_tickCount_Z80_PSG_FM);
		m_emuThread68K.m_fpsCounterAudio.Update();
		m_accumTime -= EMU_TIMESTEP;

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

			if (frame == 0)
			{
				AudioTick();
			}
		}
	}
}