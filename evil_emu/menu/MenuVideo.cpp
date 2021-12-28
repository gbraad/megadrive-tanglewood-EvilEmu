
#include "MenuVideo.h"
#include "MenuCommon.h"

#include <ion/core/utils/STL.h>
#include <ion/engine/Engine.h>

#include <algorithm>
#include <cctype>
#include <functional>

MenuVideo::MenuVideo(ion::gui::GUI& gui, Settings& settings, ion::render::Window& appWindow, const ion::Vector2i& position, const ion::Vector2i& size)
	: ion::gui::Window("Video", position, size)
	, m_appWindow(appWindow)
	, m_gui(gui)
	, m_settings(settings)
{
	SetCentred(true);
	SetBackgroundAlpha(0.8f);
	AllowMove(false);
	AllowResize(false);
	AllowRollUp(false);

	//Add widgets
	m_comboDisplays = new ion::gui::ComboBox("Display", std::bind(&MenuVideo::OnSelectedDisplay, this, std::placeholders::_1, std::placeholders::_2));
	m_comboResolution = new ion::gui::ComboBox("Window Resolution", std::bind(&MenuVideo::OnSelectedResolution, this, std::placeholders::_1, std::placeholders::_2));
	m_checkBoxFullscreen = new ion::gui::CheckBox("Fullscreen", std::bind(&MenuVideo::OnSelectedFullscreen, this, std::placeholders::_1, std::placeholders::_2));
	m_checkBoxVSync = new ion::gui::CheckBox("VSync", std::bind(&MenuVideo::OnSelectedVSync, this, std::placeholders::_1, std::placeholders::_2));
	m_checkBoxPixelBuffer = new ion::gui::CheckBox("Use Pixel Buffer", std::bind(&MenuVideo::OnSelectedPixelBuffer, this, std::placeholders::_1, std::placeholders::_2));
	m_sliderScanlines = new ion::gui::Slider("Scanline %", 0.0f, 1.0f, 0.0f, 0.1f, std::bind(&MenuVideo::OnSliderScanlines, this, std::placeholders::_1, std::placeholders::_2));
	m_comboBorder = new ion::gui::ComboBox("Border", std::bind(&MenuVideo::OnSelectedBorder, this, std::placeholders::_1, std::placeholders::_2));
	m_buttonBack = new ion::gui::Button("Back", std::bind(&MenuVideo::OnButtonBack, this, std::placeholders::_1));

	m_buttonBack->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));

	//Populate displays
	m_displays.clear();
	appWindow.GetDisplays(m_displays);

	for (int i = 0; i < m_displays.size(); i++)
	{
		std::string name = m_displays[i].name;
		name.erase(std::remove_if(name.begin(), name.end(), [](char const& c) -> bool { return !std::isalnum(c); }), name.end());
		m_comboDisplays->AddItem(ion::gui::ComboBox::Item(name, i));
	}

	//Populate border types
	m_comboBorder->AddItem(ion::gui::ComboBox::Item("EVIL_EMU", (int)VideoBorder::ImageGameCover));
	m_comboBorder->AddItem(ion::gui::ComboBox::Item("Classic Grid", (int)VideoBorder::ImageBlackGrid));
	m_comboBorder->AddItem(ion::gui::ComboBox::Item("European Blue", (int)VideoBorder::ImageBlueBorder));
	m_comboBorder->AddItem(ion::gui::ComboBox::Item("Video Background", (int)VideoBorder::VDPColour));
	m_comboBorder->AddItem(ion::gui::ComboBox::Item("Off", (int)VideoBorder::Black));

	AddWidget(*m_checkBoxFullscreen);
	AddWidget(*m_comboDisplays);
	AddWidget(*m_comboResolution);
	AddWidget(*m_checkBoxVSync);
#if defined ION_PLATFORM_WINDOWS
	AddWidget(*m_checkBoxPixelBuffer);
#endif
	AddWidget(*m_sliderScanlines);
	AddWidget(*m_comboBorder);
	AddWidget(*m_buttonBack);

	SyncSettings();
}

MenuVideo::~MenuVideo()
{
	delete m_comboDisplays;
	delete m_comboResolution;
	delete m_comboBorder;
	delete m_checkBoxFullscreen;
	delete m_checkBoxVSync;
	delete m_checkBoxPixelBuffer;
	delete m_sliderScanlines;
	delete m_buttonBack;
}

void MenuVideo::SyncSettings()
{
	//Populate resolutions for current monitor
	PopulateResolutions(m_settings.displayIdx);

	//Find current resolution
	int currentResIdx = ion::utils::stl::IndexOf(m_supportedResolutions, m_settings.resolution);

	if (currentResIdx >= 0)
	{
		m_comboResolution->SetSelection(currentResIdx);
	}

	if (m_settings.displayIdx < m_displays.size())
	{
		m_comboDisplays->SetSelection(m_settings.displayIdx);
	}

	m_comboBorder->SetSelection((int)m_settings.videoBorder);
	m_checkBoxFullscreen->SetChecked(ion::engine.render.window->GetFullscreen());
	m_checkBoxVSync->SetChecked(m_settings.vsync);
	m_checkBoxPixelBuffer->SetChecked(m_settings.pixelBuffer);
	m_sliderScanlines->SetValue(m_settings.scanlineAlpha);
	m_comboDisplays->SetEnabled(ion::engine.render.window->GetFullscreen());
	m_comboResolution->SetEnabled(!ion::engine.render.window->GetFullscreen());
}

void MenuVideo::PopulateResolutions(int displayIdx)
{
	//Get supported screen resolutions
	m_supportedResolutions.clear();
	m_appWindow.GetSupportedResolutions(m_supportedResolutions, displayIdx);

	//Remove duplicates
	m_supportedResolutions.erase(std::unique(m_supportedResolutions.begin(), m_supportedResolutions.end()), m_supportedResolutions.end());

	//Sort
	std::sort(m_supportedResolutions.begin(), m_supportedResolutions.end(), [](const ion::Vector2i& a, const ion::Vector2i& b) { return (a.x*a.y) < (b.x*b.y); });

	//Build resolution combo box
	for (int i = 0; i < m_supportedResolutions.size(); i++)
	{
		std::stringstream resolutionName;
		resolutionName << m_supportedResolutions[i].x << "x" << m_supportedResolutions[i].y;
		m_comboResolution->AddItem(ion::gui::ComboBox::Item(resolutionName.str(), i));
	}
}

void MenuVideo::OnSelectedDisplay(const ion::gui::ComboBox& comboBox, const ion::gui::ComboBox::Item& item)
{
	m_settings.displayIdx = item.GetId();
	SyncSettings();
}

void MenuVideo::OnSelectedResolution(const ion::gui::ComboBox& comboBox, const ion::gui::ComboBox::Item& item)
{
	m_settings.resolution = m_supportedResolutions[item.GetId()];
}

void MenuVideo::OnSelectedBorder(const ion::gui::ComboBox& comboBox, const ion::gui::ComboBox::Item& item)
{
	m_settings.videoBorder = (VideoBorder)item.GetId();
}

void MenuVideo::OnSelectedFullscreen(const ion::gui::CheckBox& checkbox, bool checked)
{
	m_settings.fullscreen = checked;
	m_comboDisplays->SetEnabled(m_settings.fullscreen);
	m_comboResolution->SetEnabled(!m_settings.fullscreen);
}

void MenuVideo::OnSelectedVSync(const ion::gui::CheckBox& checkbox, bool checked)
{
	m_settings.vsync = checked;
}

void MenuVideo::OnSelectedPixelBuffer(const ion::gui::CheckBox& checkbox, bool checked)
{
	m_settings.pixelBuffer = checked;
}

void MenuVideo::OnSliderScanlines(const ion::gui::Slider& slider, float value)
{
	m_settings.scanlineAlpha = value;
}

void MenuVideo::OnButtonBack(const ion::gui::Button& button)
{
	m_gui.PopWindow();
}
