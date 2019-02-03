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
	void UpdateGameVars();

	ion::gui::GUI& m_gui;
	EmulatorThread& m_emulator;

	struct
	{
		ion::gui::TextBox pc;
		ion::gui::TextBox sr;
		ion::gui::TextBox dregs[8];
		ion::gui::TextBox aregs[8];
	} m_regs68k;

#if EVIL_EMU_GAME_TANGLEWOOD
	struct
	{
		ion::gui::TextBox password;
		ion::gui::TextBox levelIdx;
		ion::gui::TextBox levelAddr;
		ion::gui::TextBox firefliesAct;
		ion::gui::TextBox firefliesGame;
		ion::gui::TextBox firefliesSave;
		ion::gui::TextBox boulderDrops;
	} m_gameVars;
#endif
};
