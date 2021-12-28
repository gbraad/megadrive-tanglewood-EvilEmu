#pragma once

#include <ion/core/thread/Thread.h>
#include <ion/core/thread/Event.h>
#include <ion/core/time/Time.h>

#include "emulator.h"

class EmulatorThread;

class FPSCounter
{
public:
	FPSCounter()
	{
		m_frameCount = 0;
		m_startTicks = 0;
		m_lastFPS = 0.0f;
	}

	void Update()
	{
		if (m_frameCount++ % 30 == 0)
		{
			//Get 60-frame end time and diff
			u64 endTicks = ion::time::GetSystemTicks();
			u64 diffTicks = endTicks - m_startTicks;

			//Calc frame time and frames per second
			float frameTime = (float)ion::time::TicksToSeconds(diffTicks) / 30.0f;
			float framesPerSecond = 1.0f / frameTime;
			m_lastFPS = framesPerSecond;

			//Reset timer
			m_startTicks = ion::time::GetSystemTicks();
		}
	}

	float GetLastFPS() const { return m_lastFPS; }
	u64 GetFrame() const { return m_frameCount; }

private:
	u64 m_frameCount;
	u64 m_startTicks;
	float m_lastFPS;
};

class EmulatorThread_Z80_PSG_FM_DAC
#if EMU_THREADED
	: public ion::thread::Thread
#endif
{
public:
	EmulatorThread_Z80_PSG_FM_DAC();
	virtual ~EmulatorThread_Z80_PSG_FM_DAC();

	void Tick_Z80_PSG_FM_DAC(double deltaTime);

protected:
	virtual void Entry();

private:
	u64 m_clockZ80;
	u64 m_clockFM;
	u64 m_clockPSG;

	bool m_running;
};

class EmulatorThread
#if EMU_THREADED
	: public ion::thread::Thread
#endif
{
public:
	EmulatorThread();
	virtual ~EmulatorThread();

	void TickEmulator(double deltaTime);

	u64 Get68KCycle() const;

	void Pause();
	void Resume();
	bool IsPaused() const;

	FPSCounter m_fpsCounterRender;

protected:
	virtual void Entry();

private:

	EmulatorThread_Z80_PSG_FM_DAC m_emuThread_Z80_PSG_FM_DAC;
	
	u64 m_clock68K;
	u64 m_cycle68K;

	u32 m_lineCounter;

	bool m_paused;
	bool m_running;
};