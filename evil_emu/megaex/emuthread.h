#pragma once

#include <ion/core/thread/Thread.h>
#include <ion/core/thread/Semaphore.h>
#include <ion/core/thread/CriticalSection.h>
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

private:
	u64 m_frameCount;
	u64 m_startTicks;
	float m_lastFPS;
};

class EmulatorThread_Z80_PSG_FM
#if EMU_THREADED
	: public ion::thread::Thread
#endif
{
public:
	EmulatorThread_Z80_PSG_FM(EmulatorThread& emuThread68K);

	void Tick_Z80_PSG_FM();

protected:
	virtual void Entry();

private:
	EmulatorThread& m_emuThread68K;

	double m_prevAudioClock;
	double m_accumTime;

	u64 m_clockZ80;
	u64 m_clockFM;
	u64 m_clockPSG;
};

class EmulatorThread
#if EMU_THREADED
	: public ion::thread::Thread
#endif
{
public:
	EmulatorThread();

	void TickEmulator(float deltaTime);

	void Pause();
	void Resume();
	bool IsPaused() const;

	FPSCounter m_fpsCounterEmulator;
	FPSCounter m_fpsCounterRender;
	FPSCounter m_fpsCounterAudio;

	ion::thread::CriticalSection m_renderCritSec;

	u32 m_tickCount_68K;
	u32 m_tickCount_Z80_PSG_FM;

protected:
	virtual void Entry();

private:

	EmulatorThread_Z80_PSG_FM m_emuThread_Z80_PSG_FM;

	EmulatorState m_lastEmulatorState;

	double m_prevAudioClock;
	double m_accumTime;
	int m_prevFramesBehind;
	u64 m_emulatorFrameCount;
	
	u64 m_clock68K;

	u32 m_lineCounter;

	bool m_paused;
};