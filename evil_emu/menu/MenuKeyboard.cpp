
#include "MenuKeyboard.h"
#include "MenuCommon.h"
#include "emulator.h"

MenuKeyboard::MenuKeyboard(ion::gui::GUI& gui, Settings& settings, ion::render::Window& appWindow, const ion::Vector2i& position, const ion::Vector2i& size)
	: ion::gui::Window("Keyboard", position, size)
	, m_gui(gui)
	, m_settings(settings)
{
	SetCentred(true);
	SetBackgroundAlpha(0.8f);
	AllowMove(false);
	AllowResize(false);
	AllowRollUp(false);

	//Build keyboard map combo boxes
	for (int i = 0; i < eBtn_MAX; i++)
	{
		ion::gui::ComboBox* combo = new ion::gui::ComboBox(g_emulatorButtonNames[i], std::bind(&MenuKeyboard::OnSelectedKeyboardMapping, this, std::placeholders::_1, std::placeholders::_2));
		AddWidget(*combo);

		for (int j = 0; j < (int)ion::input::Keycode::COUNT; j++)
		{
			combo->AddItem(ion::gui::ComboBox::Item(ion::input::KeycodeNames[j], j));
		}

		combo->SetSelection((int)m_settings.keyboardMap[i]);
		m_combosKeyboardMap.push_back(combo);
	}

	//Add back button
	m_buttonBack = new ion::gui::Button("Back", std::bind(&MenuKeyboard::OnButtonBack, this, std::placeholders::_1));
	m_buttonBack->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));
	AddWidget(*m_buttonBack);

	SyncSettings();
}

MenuKeyboard::~MenuKeyboard()
{
	for (int i = 0; i < m_combosKeyboardMap.size(); i++)
	{
		delete m_combosKeyboardMap[i];
	}
	
	delete m_buttonBack;
}

void MenuKeyboard::SyncSettings()
{
	for (int i = 0; i < eBtn_MAX; i++)
	{
		m_combosKeyboardMap[i]->SetSelection((int)m_settings.keyboardMap[i]);
	}
}

void MenuKeyboard::OnSelectedKeyboardMapping(const ion::gui::ComboBox& comboBox, const ion::gui::ComboBox::Item& item)
{
	//A bit hacky
	//TODO: Add userdata/ids to widgets
	for (int i = 0; i < m_combosKeyboardMap.size(); i++)
	{
		if (&comboBox == m_combosKeyboardMap[i])
		{
			m_settings.keyboardMap[i] = (ion::input::Keycode)item.GetId();
			return;
		}
	}
}

void MenuKeyboard::OnButtonBack(const ion::gui::Button& button)
{
	m_gui.PopWindow();
}