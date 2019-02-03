#pragma once

#include <ion/gui/GUI.h>
#include <ion/gui/Window.h>
#include <ion/gui/Button.h>
#include <ion/gui/CommonDlg.h>
#include <ion/gui/CheckBox.h>
#include <ion/gui/ComboBox.h>
#include <ion/gui/TextBox.h>
#include <ion/gui/MessageBox.h>
#include <ion/io/FileSystem.h>

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

	ion::gui::GUI& m_gui;
	//ion::gui::Font& m_font;
	ion::render::Window& m_appWindow;

	Save& m_save;

	std::vector<ion::gui::Button*> m_saveSlotButtons;
	ion::gui::Button* m_btnCancel;
};