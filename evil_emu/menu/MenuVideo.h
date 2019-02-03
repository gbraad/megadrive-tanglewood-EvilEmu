#pragma once

#include <ion/gui/GUI.h>
#include <ion/gui/Window.h>
#include <ion/gui/Button.h>
#include <ion/gui/CheckBox.h>
#include <ion/gui/ComboBox.h>
#include <ion/gui/Slider.h>
#include <ion/gui/TextBox.h>

#include "settings.h"

class MenuVideo : public ion::gui::Window
{
public:
	MenuVideo(ion::gui::GUI& gui, Settings& settings, ion::render::Window& appWindow, const ion::Vector2i& position, const ion::Vector2i& size);
	~MenuVideo();

	void SyncSettings();

private:
	void OnSelectedDisplay(const ion::gui::ComboBox& comboBox, const ion::gui::ComboBox::Item& item);
	void OnSelectedResolution(const ion::gui::ComboBox& comboBox, const ion::gui::ComboBox::Item& item);
	void OnSelectedBorder(const ion::gui::ComboBox& comboBox, const ion::gui::ComboBox::Item& item);
	void OnSelectedFullscreen(const ion::gui::CheckBox& checkbox, bool checked);
	void OnSelectedVSync(const ion::gui::CheckBox& checkbox, bool checked);
	void OnSelectedPixelBuffer(const ion::gui::CheckBox& checkbox, bool checked);
	void OnSliderScanlines(const ion::gui::Slider& slider, float value);
	void OnButtonBack(const ion::gui::Button& button);

	void PopulateResolutions(int displayIdx);

	ion::render::Window& m_appWindow;
	ion::gui::GUI& m_gui;
	Settings& m_settings;

	std::vector<ion::render::Display> m_displays;
	std::vector<ion::Vector2i> m_supportedResolutions;

	ion::gui::ComboBox* m_comboDisplays;
	ion::gui::ComboBox* m_comboResolution;
	ion::gui::ComboBox* m_comboBorder;
	ion::gui::CheckBox* m_checkBoxFullscreen;
	ion::gui::CheckBox* m_checkBoxVSync;
	ion::gui::CheckBox* m_checkBoxPixelBuffer;
	ion::gui::Slider* m_sliderScanlines;
	ion::gui::Button* m_buttonBack;
};
