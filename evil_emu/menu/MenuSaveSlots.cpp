
#include "MenuSaveSlots.h"

#include "roms/include_vars.h"

#include <ion/core/utils/STL.h>

const char* g_levelList[] =
{
	"Harlequin Forest Act 1",
	"Harlequin Forest Act 2",
	"Harlequin Forest Act 3",
	"Harlequin Forest Act 4",
	"StormWarning Act 1",
	"StormWarning Act 2",
	"StormWarning Act 3",
	"Heritage Act 1",
	"Heritage Act 2",
	"Heritage Act 3",
	"Tethered Act 1",
	"Tethered Act 2",
	"Tethered Act 3",
	"Bygone Act 1",
	"Bygone Act 2",
	"Bygone Act 3",
	"Bygone Act 4",
	"Deadwood Act 1",
	"Deadwood Act 2",
	"Deadwood Act 3",
	"Oasis Act 1",
	"Oasis Act 2",
	"Oasis Act 3",
	"Turntail Act 1",
	"Turntail Act 2",
	"Turntail Act 3",
	"Turntail Act 4",
	"Turntail Act 5",
	"Turntail Act 6",
};

MenuSaveSlots::MenuSaveSlots(ion::gui::GUI& gui, /* ion::gui::Font& font, */ ion::render::Window& appWindow, Save& save, std::function<void(int)> const& onClosed)
	: ion::gui::Window("Select Save", ion::Vector2i(), ion::Vector2i())
	, m_onClosed(onClosed)
	, m_gui(gui)
	//, m_font(font)
	, m_appWindow(appWindow)
	, m_save(save)
{
	SetCentred(true);
	//SetFont(font);
	SetBackgroundAlpha(0.6f);
	SetSize(ion::Vector2i(0, appWindow.GetClientAreaHeight() - 100));
	AllowMove(false);
	AllowResize(false);
	AllowRollUp(false);

#if EVIL_EMU_USE_SAVES

	//Populate save slot buttons (in reverse order)
	for (int i = save.m_saveSlots.size() - 1; i >= 0; i--)
	{
		Save::SaveSlot& slot = save.m_saveSlots[i];

		std::stringstream buttonLabel;
		buttonLabel << "Save " << (i + 1);
		buttonLabel << "\nTime: " << SSTREAM_INT2(slot.timeStamp.GetDay()) << "/" << SSTREAM_INT2(slot.timeStamp.GetMonth()) << "/" << SSTREAM_INT4(slot.timeStamp.GetYear())
			<< " " << slot.timeStamp.GetHour() << ":" << SSTREAM_INT2(slot.timeStamp.GetMinute()) << ":" << SSTREAM_INT2(slot.timeStamp.GetSecond());
		buttonLabel << "\nLevel: " << g_levelList[slot.levelIdx];
		buttonLabel << "\nFireflies: " << slot.firefliesGame << "/" << snasm68k_symbol_TotalFireflyCount_val;
		buttonLabel << "\nPassword: "
			<< std::string(1, (char)((slot.password >> 0) & 0x0F) + 'A')
			<< std::string(1, (char)((slot.password >> 4) & 0x0F) + 'A')
			<< std::string(1, (char)((slot.password >> 8) & 0x0F) + 'A')
			<< std::string(1, (char)((slot.password >> 12) & 0x0F) + 'A')
			<< std::string(1, (char)((slot.password >> 16) & 0x0F) + 'A')
			<< std::string(1, (char)((slot.password >> 20) & 0x0F) + 'A')
			<< std::string(1, (char)((slot.password >> 24) & 0x0F) + 'A')
			<< std::string(1, (char)((slot.password >> 28) & 0x0F) + 'A');

		ion::gui::Button* button = new ion::gui::Button(buttonLabel.str(), std::bind(&MenuSaveSlots::OnButtonSaveSelected, this, std::placeholders::_1));
		button->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));
		button->SetTextAlign(ion::gui::Button::TextAlign::Left);

		//If data saved with FINAL build, but we're running DEMO, data isn't compatible
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
		if (slot.gameType != EVIL_EMU_GAME_TYPE_DEMO)
		{
			button->SetEnabled(false);
		}
#endif

		AddWidget(*button);
		m_saveSlotButtons.push_back(button);
	}

#endif // EVIL_EMU_USE_SAVES

	m_btnCancel = new ion::gui::Button("Cancel", std::bind(&MenuSaveSlots::OnButtonCancel, this, std::placeholders::_1));
	AddWidget(*m_btnCancel);
}

MenuSaveSlots::~MenuSaveSlots()
{
	for (int i = 0; i < m_saveSlotButtons.size(); i++)
	{
		delete m_saveSlotButtons[i];
	}

	delete m_btnCancel;
}

void MenuSaveSlots::OnButtonSaveSelected(const ion::gui::Button& button)
{
	int index = 0;

	for (int i = 0; i < m_saveSlotButtons.size(); i++)
	{
		if (m_saveSlotButtons[i] == &button)
		{
			index = m_saveSlotButtons.size() - i - 1;
		}
	}

	m_onClosed(index);
}

void MenuSaveSlots::OnButtonCancel(const ion::gui::Button& button)
{
	m_onClosed(-1);
}
