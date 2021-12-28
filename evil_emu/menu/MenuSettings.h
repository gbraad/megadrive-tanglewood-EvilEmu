#pragma once

#include <ion/gui/GUI.h>
#include <ion/gui/Window.h>
#include <ion/gui/Button.h>
#include <ion/gui/CommonDlg.h>
#include <ion/gui/CheckBox.h>
#include <ion/gui/ComboBox.h>
#include <ion/gui/TextBox.h>
#include <ion/gui/MessageBox.h>
#include <ion/core/io/FileSystem.h>

#include "constants.h"

#if EVIL_EMU_USE_UTILITY_MENUS

#include "settings.h"
#include "MenuCommon.h"
#include "MenuVideo.h"
#include "MenuKeyboard.h"
#include "MenuGamepad.h"
#include "MenuManual.h"
#include "DlgROMDisclaimer.h"

class MenuSettings : public ion::gui::Window
{
public:
	MenuSettings(ion::gui::GUI& gui, ion::gui::Font& font, ion::io::FileSystem& fileSystem, Settings& settings, ion::render::Window& appWindow, const ion::Vector2i& position, const ion::Vector2i& size);
	~MenuSettings();

	void SyncSettings();

private:
	void OnButtonManual(const ion::gui::Button& button);
	void OnButtonCopyROM(const ion::gui::Button& button);
	void OnButtonVideo(const ion::gui::Button& button);
	void OnButtonKeyboard(const ion::gui::Button& button);
	void OnButtonGamepad(const ion::gui::Button& button);

	void OnDlgClosedROMDisclaimer(const ion::gui::DialogBox& dialog);

	ion::gui::GUI& m_gui;
	ion::gui::Font& m_font;

#if defined ION_PLATFORM_MACOSX
	Settings& m_settings;
#endif

	ion::render::Window& m_appWindow;

	MenuManual* m_menuManual;
	MenuVideo* m_menuVideo;
	MenuKeyboard* m_menuKeyboard;
	MenuGamepad* m_menuGamepad;

	DlgROMDisclaimer* m_dlgROMDisclaimer;
	ion::gui::MessageBox* m_msgROMInstructions;

#if EVIL_EMU_USE_MANUAL
	ion::gui::Button* m_buttonManual;
#endif

#if EVIL_EMU_ROM_DOWNLOAD
	ion::gui::Button* m_buttonCopyROM;
#endif

	ion::gui::Button* m_buttonVideo;
	ion::gui::Button* m_buttonKeyboard;
	ion::gui::Button* m_buttonGamepad;
};

#endif