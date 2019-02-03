#pragma once

#include <ion/gui/GUI.h>
#include <ion/gui/Window.h>
#include <ion/gui/Button.h>
#include <ion/gui/CheckBox.h>
#include <ion/gui/ComboBox.h>
#include <ion/gui/Image.h>
#include <ion/gui/TextBox.h>

#include <ion/renderer/Texture.h>

#include "settings.h"

class MenuManual : public ion::gui::Window
{
public:
	MenuManual(ion::gui::GUI& gui, Settings& settings, const ion::Vector2i& position, const ion::Vector2i& size);
	~MenuManual();

	void SyncSettings();

private:
	void OnButtonNext(const ion::gui::Button& button);
	void OnButtonPrev(const ion::gui::Button& button);
	void OnButtonBack(const ion::gui::Button& button);

	ion::gui::GUI& m_gui;
	Settings& m_settings;

	ion::gui::Image* m_image;
	ion::gui::Button* m_buttonNext;
	ion::gui::Button* m_buttonPrev;
	ion::gui::Button* m_buttonBack;

	ion::render::Texture* m_currentTexture;
	int m_currentImageIdx;
	std::vector<std::string> m_imageFilenames;
};
