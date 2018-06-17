
#include <ion/core/thread/Thread.h>
#include <ion/core/thread/Semaphore.h>
#include <ion/core/thread/CriticalSection.h>

#include "emulator.h"

class EmulatorThread
#if EMU_THREADED
	: public ion::thread::Thread
#endif
{
public:
	EmulatorThread();

#if !EMU_THREADED
	void TickEmulator(float deltaTime);
#endif

	ion::thread::CriticalSection m_renderCritSec;

protected:
	virtual void Entry();

private:
#if EMU_THREADED
	void TickEmulator(float deltaTime);
#endif

	EmulatorState m_lastEmulatorState;

	float m_prevAudioClock;
	float m_accumTime;
	
	u64 m_clock68K;
	u64 m_clockZ80;
	u64 m_clockFM;
	u64 m_clockPSG;

	bool m_inVBlank;
	bool m_inHBlank;
	u32 m_lineCounter;
};