
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
#include "cpu/m68k/cpu.h"
#include "cpu/z80/z80.h"
#include "vdp/vdp.h"
#include "memory.h"
#include "mgaudio.h"
#include "gui/debugger.h"

#include "cpu/m68k/cpu_ops.inl"

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

EmulatorThread::EmulatorThread()
#if EMU_THREADED
	: ion::thread::Thread("Emulator_68K")
	, m_emuThread_Z80_PSG_FM(*this)
#else
    : m_emuThread_Z80_PSG_FM(*this)
#endif
{
	m_prevAudioClock = 0.0f;
	m_accumTime = 0.0f;
	m_prevFramesBehind = 0;

	m_tickCount_68K = 0;
	m_tickCount_Z80_PSG_FM = 0;

	m_clock68K = 0;

	m_lineCounter = 0xFF;

	m_emulatorFrameCount = 0;

	m_paused = false;
}

void EmulatorThread::Entry()
{
#if EMU_THREADED
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
#endif
}

void EmulatorThread::Pause()
{
	m_paused = true;
	AudioPausePlayback();
}

void EmulatorThread::Resume()
{
	m_paused = false;
	AudioResumePlayback();
}

bool EmulatorThread::IsPaused() const
{
	return m_paused;
}

void EmulatorThread::TickEmulator(float deltaTime)
{
	int debuggerRunning = false;

	const int cyclesPerFrame = CYCLES_PER_FRAME_68K;
	const int cyclesPerLine = CYCLES_PER_LINE_68K;

	int framesBehind = 0;

	if (!debuggerRunning && !m_paused)
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

			m_accumTime = 0.0f;

			//Begin frame
			VID_BeginFrame();

			//Process one frame at a time
			int videoCycle = 0;
#if CPU_COMBINE_STAGES
			int prevCycle = 0;
			int currCycle = 0;
			for (int i = 0; i < cyclesPerFrame;)
#else
			for (int i = 0; i < cyclesPerFrame; i++)
#endif
			{
				m_clock68K += EMU_CLOCK_DIV_68K;

#if CPU_COMBINE_STAGES
				prevCycle = i;
				i += CPU_Step();
				currCycle = i;
#else
				CPU_Step();
#endif

#if !EMU_THREADED
				//Tick Z80, PSG, and FM
				m_emuThread_Z80_PSG_FM.Tick_Z80_PSG_FM();
#endif

#if CPU_COMBINE_STAGES
				for(videoCycle = prevCycle; videoCycle < currCycle; videoCycle++)
#else
				videoCycle = i;
#endif
				{
					Globals::lineNo = videoCycle / cyclesPerLine;
					int colNo = videoCycle % cyclesPerLine;

					Globals::inVBlank = (Globals::lineNo > 223);
					Globals::inHBlank = (colNo > ((cyclesPerLine * 3) / 4));

					if (colNo == ((cyclesPerLine * 3) / 4))
					{
						if (Globals::lineNo > 224)
						{
							m_lineCounter = VDP::VDP_Registers[0x0A];
						}
						else
						{
							m_lineCounter--;
							if (m_lineCounter == 0xFFFFFFFF)
							{
								m_lineCounter = VDP::VDP_Registers[0x0A];
								CPU_SignalInterrupt(4);
							}
						}

						if (drawFrame)
						{
							int displaySizeY = (VDP::VDP_Registers[1] & 0x08) ? 30 * 8 : 28 * 8;
							if (Globals::lineNo < displaySizeY)
							{
								VID_DrawScreenRow(Globals::lineNo);
							}
						}
					}

					if ((Globals::lineNo == 225) && (colNo == 8))
					{
						CPU_SignalInterrupt(6);
						Z80_SignalInterrupt(0);
					}
				}
			}

			m_renderCritSec.End();
		}

#if EMU_THREADED
		//Wait for audio
		while (m_tickCount_68K > m_tickCount_Z80_PSG_FM)
		{
			ion::thread::Sleep(1);
		}
#endif
	}

	m_lastEmulatorState = debuggerRunning ? eState_Debugger : eState_Running;
}

EmulatorThread_Z80_PSG_FM::EmulatorThread_Z80_PSG_FM(EmulatorThread& emuThread68K)
#if EMU_THREADED
	: ion::thread::Thread("Emulator_Z80_PSG_FM")
	, m_emuThread68K(emuThread68K)
#else
    : m_emuThread68K(emuThread68K)
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

#if defined _DEBUG
	if (framesBehind > 1)
	{
		printf("Audio %i frames behind\n", framesBehind);
	}
#endif

	for (int frame = 0; frame < framesBehind; frame++)
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

#if EMU_THREADED
			if (frame == 0)
#endif
			{
				AudioTick();
			}
		}
#if EMU_THREADED
	}
#endif
}
