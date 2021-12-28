#pragma once

#include <ion/gui/GUI.h>
#include <ion/gui/Window.h>
#include <ion/gui/Button.h>
#include <ion/gui/CheckBox.h>
#include <ion/gui/ComboBox.h>
#include <ion/gui/TextBox.h>

#include "megaex/emuthread.h"

class DebuggerUI : public ion::gui::Window
{
public:
	DebuggerUI(ion::gui::GUI& gui, EmulatorThread& emulator, const ion::Vector2i& position, const ion::Vector2i& size);
	~DebuggerUI();

	virtual void Update(float deltaTime);

private:
	void Update68KRegs();
	void UpdateZ80Regs();
	void UpdateFMRegs();

	EmulatorThread& m_emulator;

	struct
	{
		ion::gui::TextBox pc;
		ion::gui::TextBox sr;
		ion::gui::TextBox dregs[8];
		ion::gui::TextBox aregs[8];
	} m_regs68k;

	struct
	{
		ion::gui::TextBox pc;
	} m_regsZ80;

	struct
	{
		ion::gui::TextBox timerA;
		ion::gui::TextBox timerB;
	} m_regsFM;

	struct
	{
		ion::gui::TextBox windowFPS;
		ion::gui::TextBox m68kFPS;
		ion::gui::TextBox deltaTime;
		ion::gui::TextBox audioClock;
		ion::gui::TextBox audioBuffersQueued;
	} m_gameVars;

	FPSCounter m_fpsCounter;
};
