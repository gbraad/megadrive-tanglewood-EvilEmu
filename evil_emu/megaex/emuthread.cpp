
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ion/engine/Engine.h>
#include <ion/core/time/Time.h>
#include <ion/core/thread/Sleep.h>
#include <ion/core/debug/Debug.h>
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
#endif
{
	m_clock68K = 0;
	m_cycle68K = 0;

	m_lineCounter = 0xFF;

	m_paused = false;
	m_running = true;
}

EmulatorThread::~EmulatorThread()
{
	m_running = false;
}

void EmulatorThread::Entry()
{
#if EMU_THREADED
	m_emuThread_Z80_PSG_FM_DAC.Run();
	m_emuThread_Z80_PSG_FM_DAC.SetPriority(ion::thread::Thread::Priority::Critical);

#if defined ION_PLATFORM_SWITCH
	m_emuThread_Z80_PSG_FM_DAC.SetCoreAffinity(1 << EMU_THREAD_CORE_Z80);
	ion::engine.audio.thread->SetCoreAffinity(1 << EMU_THREAD_CORE_AUDIO);
#endif

	double prevTime = 0.0f;

	while (m_running)
	{
		//Get audio clock
		double targetTime = AudioGetClock();
		double deltaTime = targetTime - prevTime;

		TickEmulator(deltaTime);
		Yield();

		prevTime = targetTime;
	}
#endif
}

u64 EmulatorThread::Get68KCycle() const
{
	return m_cycle68K;
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
	return m_paused || Globals::g_pause;
}

void EmulatorThread::TickEmulator(double deltaTime)
{
	if (!m_paused && !Globals::g_pause)
	{
		//Calculate cycles to process
		u64 num68kCycles = ion::maths::Round(deltaTime * CYCLES_PER_SECOND_68K);

#if CPU_COMBINE_STAGES
		for (u64 i = 0; i < num68kCycles && !Globals::g_pause;)
#else
		for (u64 i = 0; i < cyclesPerFrame && !Globals::g_pause; i++)
#endif
		{
			m_clock68K += EMU_CLOCK_DIV_68K;

#if CPU_COMBINE_STAGES
			u64 prevCycle = m_cycle68K;
			u64 cyclesStepped = CPU_Step();
			m_cycle68K += cyclesStepped;
			i += cyclesStepped;
#else
			CPU_Step();
#endif

#if EMU_ENABLE_68K_DEBUGGER
			DebuggerStep();
#endif

#if !EMU_THREADED
			//Tick Z80, PSG, and FM
			m_emuThread_Z80_PSG_FM_DAC.Tick_Z80_PSG_FM_DAC();
#endif

			//Emulate latch capacitor drain in 6-button controller
			IO_UpdateControllerLatch(cyclesStepped);

#if CPU_COMBINE_STAGES
			for(u64 videoCycle = prevCycle; videoCycle < m_cycle68K; videoCycle++)
#else
			videoCycle = i;
#endif
			{
				u64 frameCycle = (videoCycle % CYCLES_PER_FRAME_68K);

				if (frameCycle == 0)
				{
					VDP_WriteLock();
					VID_BeginFrame();
				}

				Globals::lineNo = frameCycle / CYCLES_PER_LINE_68K;
				Globals::colNo = frameCycle % CYCLES_PER_LINE_68K;

				bool vblank = (Globals::lineNo > (VDP_SCREEN_HEIGHT - 1));
				bool hblank = (Globals::colNo > ((CYCLES_PER_LINE_68K * 3) / 4));

				bool exitedActiveScan = vblank && !Globals::inVBlank;

				Globals::inVBlank = vblank;
				Globals::inHBlank = hblank;

				if (Globals::colNo == ((CYCLES_PER_LINE_68K * 3) / 4))
				{
					if (Globals::lineNo > VDP_SCREEN_HEIGHT)
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

					int displaySizeY = (VDP::VDP_Registers[1] & 0x08) ? 30 * 8 : 28 * 8;
					if (Globals::lineNo < (u32)displaySizeY)
					{
						VID_DrawScreenRow(Globals::lineNo);
					}
				}

				if ((Globals::lineNo == (VDP_SCREEN_HEIGHT+1)) && (Globals::colNo == 8))
				{
					CPU_SignalInterrupt(6);
					Z80_SignalInterrupt(0);
				}

				if (exitedActiveScan)
				{
					VDP_WriteUnlock();
					m_fpsCounterRender.Update();
				}
			}
		}
	}
}

EmulatorThread_Z80_PSG_FM_DAC::EmulatorThread_Z80_PSG_FM_DAC()
#if EMU_THREADED
	: ion::thread::Thread("Emulator_Z80_PSG_FM_DAC")
#endif
{
	m_clockZ80 = 0;
	m_clockFM = 0;
	m_clockPSG = 0;

	m_running = true;
}

EmulatorThread_Z80_PSG_FM_DAC::~EmulatorThread_Z80_PSG_FM_DAC()
{
	AudioStopPlayback();
	m_running = false;
}

void EmulatorThread_Z80_PSG_FM_DAC::Entry()
{
	//Start audio playback
	AudioBeginPlayback();

	double prevTime = 0.0f;

	while (m_running)
	{
		//Get audio clock
		double targetTime = AudioGetClock();
		double deltaTime = targetTime - prevTime;

		Tick_Z80_PSG_FM_DAC(deltaTime);

		Yield();

		prevTime = targetTime;
	}
}

void EmulatorThread_Z80_PSG_FM_DAC::Tick_Z80_PSG_FM_DAC(double deltaTime)
{
#if EMU_THREADED
	u64 numDACCycles = ion::maths::Round(deltaTime * AUDIO_SAMPLE_RATE_HZ);
	u64 num68kCyclesPerDAC = CYCLES_PER_SECOND_68K / AUDIO_SAMPLE_RATE_HZ;

	for (int i = 0; i < numDACCycles; i++)
#endif
	{
		if (!Globals::g_pause)
		{
			for (int j = 0; j < num68kCyclesPerDAC; j++)
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
			}
		}

		AudioDACUpdate();
	}
}
