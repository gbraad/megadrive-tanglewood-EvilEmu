#include "DebuggerUI.h"
#include "constants.h"

#include "megaex/cpu/m68k/cpu.h"
#include "megaex/cpu/z80/z80.h"
#include "megaex/audio/lj_ym2612.h"
#include "megaex/memory.h"
#include "megaex/mgaudio.h"

#include <sstream>
#include <iomanip>

#define HEX2(val) std::hex << std::setfill('0') << std::setw(2) << std::uppercase << (int)val
#define HEX4(val) std::hex << std::setfill('0') << std::setw(4) << std::uppercase << (int)val
#define HEX8(val) std::hex << std::setfill('0') << std::setw(8) << std::uppercase << (int)val

DebuggerUI::DebuggerUI(ion::gui::GUI& gui, EmulatorThread& emulator, const ion::Vector2i& position, const ion::Vector2i& size)
	: ion::gui::Window("Debugger", position, size)
	, m_emulator(emulator)
{
	for (int i = 0; i < 8; i++)
	{
		AddWidget(m_regs68k.dregs[i]);
		m_regs68k.aregs[i].SetArrangement(ion::gui::Widget::Arrangement::Horizontal);
		AddWidget(m_regs68k.aregs[i]);
	}

	AddWidget(m_regs68k.pc);
	AddWidget(m_regs68k.sr);
	AddWidget(m_regsZ80.pc);
	AddWidget(m_regsFM.timerA);
	AddWidget(m_regsFM.timerB);

	AddWidget(m_gameVars.windowFPS);
	AddWidget(m_gameVars.m68kFPS);
	AddWidget(m_gameVars.deltaTime);
	AddWidget(m_gameVars.audioClock);
	AddWidget(m_gameVars.audioBuffersQueued);
}

DebuggerUI::~DebuggerUI()
{

}

void DebuggerUI::Update(float deltaTime)
{
	m_fpsCounter.Update();

	Update68KRegs();
	UpdateZ80Regs();
	UpdateFMRegs();

	std::stringstream text;
	text.setf(std::ios::fixed, std::ios::floatfield);
	text.precision(2);

	text << "Main thread FPS: " << m_fpsCounter.GetLastFPS();
	m_gameVars.windowFPS.SetText(text.str());

	text.str("");
	text << "68000 FPS: " << m_emulator.m_fpsCounterRender.GetLastFPS();
	m_gameVars.m68kFPS.SetText(text.str());

	std::stringstream stream;
	stream << "Delta: " << deltaTime;
	m_gameVars.deltaTime.SetText(stream.str());

	stream.str("");
	stream << "Audio clock: " << AudioGetClock();
	m_gameVars.audioClock.SetText(stream.str());

	stream.str("");
	stream << "Audio buffers queued: " << AudioGetBuffersQueued();
	m_gameVars.audioBuffersQueued.SetText(stream.str());

	Window::Update(deltaTime);
}

void DebuggerUI::Update68KRegs()
{
	std::stringstream stream;

	for (int i = 0; i < 8; i++)
	{
		stream.str("");
		stream << "68K d" << i << ": 0x" << HEX8(M68K::cpu_regs.D[i]);
		m_regs68k.dregs[i].SetText(stream.str());
	}

	for (int i = 0; i < 8; i++)
	{
		stream.str("");
		stream << "68K a" << i << ": 0x" << HEX8(M68K::cpu_regs.A[i]);
		m_regs68k.aregs[i].SetText(stream.str());
	}

	stream.str("");
	stream << "68K PC: 0x" << HEX8(M68K::cpu_regs.PC);
	m_regs68k.pc.SetText(stream.str());

	stream.str("");
	stream << "68K SR: 0x" << HEX4(M68K::cpu_regs.SR);
	m_regs68k.sr.SetText(stream.str());
}

void DebuggerUI::UpdateZ80Regs()
{
	std::stringstream stream;

	stream.str("");
	stream << "FM Timer A: " << LJ_YM2612_getTimerA(YM2612::ym2612_chip);
	m_regsFM.timerA.SetText(stream.str());

	stream.str("");
	stream << "FM Timer B: " << LJ_YM2612_getTimerB(YM2612::ym2612_chip);
	m_regsFM.timerB.SetText(stream.str());
}

void DebuggerUI::UpdateFMRegs()
{
	std::stringstream stream;

	stream.str("");
	stream << "Z80 PC: 0x" << HEX4(Z80::Z80_regs.PC);
	m_regsZ80.pc.SetText(stream.str());
}

