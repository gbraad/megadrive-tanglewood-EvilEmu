
#include "MenuGamepad.h"
#include "MenuCommon.h"
#include "emulator.h"

#if EVIL_EMU_USE_UTILITY_MENUS

MenuGamepad::MenuGamepad(ion::gui::GUI& gui, Settings& settings, ion::render::Window& appWindow, const ion::Vector2i& position, const ion::Vector2i& size)
	: ion::gui::Window("Gamepad", position, size)
	, m_gui(gui)
	, m_settings(settings)
{
	SetCentred(true);
	SetBackgroundAlpha(0.8f);
	AllowMove(false);
	AllowResize(false);
	AllowRollUp(false);

	//Build gamepad map combo boxes
	for (int i = 0; i < eBtn_MAX; i++)
	{
		ion::gui::ComboBox* combo = new ion::gui::ComboBox(g_emulatorButtonNames[i], std::bind(&MenuGamepad::OnSelectedGamepadMapping, this, std::placeholders::_1, std::placeholders::_2));
		AddWidget(*combo);

		for (int j = 0; j < (int)ion::input::GamepadButtons::COUNT; j++)
		{
			combo->AddItem(ion::gui::ComboBox::Item(ion::input::GamepadButtonNames[(int)ion::input::GamepadType::Generic][j], j));
		}

		combo->SetSelection((int)m_settings.gamepadMap[i]);
		m_combosGamepadMap.push_back(combo);
	}

	//Add back button
	m_buttonBack = new ion::gui::Button("Back", std::bind(&MenuGamepad::OnButtonBack, this, std::placeholders::_1));
	m_buttonBack->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));
	AddWidget(*m_buttonBack);

	SyncSettings();
}

MenuGamepad::~MenuGamepad()
{
	for (int i = 0; i < m_combosGamepadMap.size(); i++)
	{
		delete m_combosGamepadMap[i];
	}

	delete m_buttonBack;
}

void MenuGamepad::SyncSettings()
{
	for (int i = 0; i < eBtn_MAX; i++)
	{
		m_combosGamepadMap[i]->SetSelection((int)m_settings.gamepadMap[i]);
	}
}

void MenuGamepad::OnSelectedGamepadMapping(const ion::gui::ComboBox& comboBox, const ion::gui::ComboBox::Item& item)
{
	//A bit hacky
	//TODO: Add userdata/ids to widgets
	for (int i = 0; i < m_combosGamepadMap.size(); i++)
	{
		if (&comboBox == m_combosGamepadMap[i])
		{
			m_settings.gamepadMap[i] = (ion::input::GamepadButtons)item.GetId();
			return;
		}
	}
}

void MenuGamepad::OnButtonBack(const ion::gui::Button& button)
{
	m_gui.PopWindow();
}

#endif