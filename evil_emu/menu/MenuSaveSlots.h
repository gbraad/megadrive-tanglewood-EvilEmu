#pragma once

#include "constants.h"

#if EVIL_EMU_USE_SAVES && EVIL_EMU_MULTIPLE_SAVESLOTS

#include <ion/gui/GUI.h>
#include <ion/gui/Window.h>
#include <ion/gui/Button.h>
#include <ion/gui/CommonDlg.h>
#include <ion/gui/CheckBox.h>
#include <ion/gui/ComboBox.h>
#include <ion/gui/TextBox.h>
#include <ion/gui/MessageBox.h>
#include <ion/core/io/FileSystem.h>

#include "MenuCommon.h"
#include "savegame.h"

class MenuSaveSlots : public ion::gui::Window
{
public:
	MenuSaveSlots(ion::gui::GUI& gui, /* ion::gui::Font& font, */ ion::render::Window& appWindow, Save& save, std::function<void(int)> const& onClosed);
	~MenuSaveSlots();

private:
	void OnButtonSaveSelected(const ion::gui::Button& button);
	void OnButtonCancel(const ion::gui::Button& button);

	std::function<void(int)> m_onClosed;

	std::vector<ion::gui::Button*> m_saveSlotButtons;
	ion::gui::Button* m_btnCancel;
};

#endif