#pragma once

#include <ion/gui/GUI.h>
#include <ion/gui/Window.h>
#include <ion/gui/Button.h>
#include <ion/gui/CheckBox.h>
#include <ion/gui/ComboBox.h>
#include <ion/gui/TextBox.h>

#include "settings.h"

class MenuGamepad : public ion::gui::Window
{
public:
	MenuGamepad(ion::gui::GUI& gui, Settings& settings, ion::render::Window& appWindow, const ion::Vector2i& position, const ion::Vector2i& size);
	~MenuGamepad();

	void SyncSettings();

private:
	void OnSelectedGamepadMapping(const ion::gui::ComboBox& comboBox, const ion::gui::ComboBox::Item& item);
	void OnButtonBack(const ion::gui::Button& button);

	ion::gui::GUI& m_gui;
	Settings& m_settings;

	std::vector<ion::gui::ComboBox*> m_combosGamepadMap;
	ion::gui::Button* m_buttonBack;
};