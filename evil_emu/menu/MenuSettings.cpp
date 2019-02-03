
#include "MenuSettings.h"

MenuSettings::MenuSettings(ion::gui::GUI& gui, ion::gui::Font& font, ion::io::FileSystem& fileSystem, Settings& settings, ion::render::Window& appWindow, const ion::Vector2i& position, const ion::Vector2i& size)
	: ion::gui::Window("Settings", position, size)
	, m_gui(gui)
	, m_font(font)
	, m_fileSystem(fileSystem)
	, m_settings(settings)
	, m_appWindow(appWindow)
{
	m_dlgROMDisclaimer = nullptr;

	SetCentred(true);
	SetFont(font);
	SetBackgroundAlpha(0.6f);
	AllowMove(false);
	AllowResize(false);
	AllowRollUp(false);

	//Add child windows
	m_menuManual = new MenuManual(gui, settings, ion::Vector2i(), ion::Vector2i());
	m_menuVideo = new MenuVideo(gui, settings, appWindow, ion::Vector2i(), ion::Vector2i());
	m_menuKeyboard = new MenuKeyboard(gui, settings, appWindow, ion::Vector2i(), ion::Vector2i());
	m_menuGamepad = new MenuGamepad(gui, settings, appWindow, ion::Vector2i(), ion::Vector2i());

	//Set fonts
	m_menuManual->SetFont(font);
	m_menuVideo->SetFont(font);
	m_menuKeyboard->SetFont(font);
	m_menuGamepad->SetFont(font);

	//Add menu buttons
	m_buttonManual = new ion::gui::Button("View Game Manual", std::bind(&MenuSettings::OnButtonManual, this, std::placeholders::_1));
	m_buttonCopyROM = new ion::gui::Button("Save Mega Drive ROM", std::bind(&MenuSettings::OnButtonCopyROM, this, std::placeholders::_1));
	m_buttonVideo = new ion::gui::Button("Video", std::bind(&MenuSettings::OnButtonVideo, this, std::placeholders::_1));
	m_buttonKeyboard = new ion::gui::Button("Keyboard", std::bind(&MenuSettings::OnButtonKeyboard, this, std::placeholders::_1));
	m_buttonGamepad = new ion::gui::Button("Gamepad", std::bind(&MenuSettings::OnButtonGamepad, this, std::placeholders::_1));

	m_buttonManual->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));
	m_buttonCopyROM->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));
	m_buttonVideo->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));
	m_buttonKeyboard->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));
	m_buttonGamepad->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));

	AddWidget(*m_buttonManual);

#if EVIL_EMU_ROM_DOWNLOAD
	AddWidget(*m_buttonCopyROM);
#endif

	AddWidget(*m_buttonVideo);
	AddWidget(*m_buttonKeyboard);
	AddWidget(*m_buttonGamepad);
}

MenuSettings::~MenuSettings()
{
	delete m_menuManual;
	delete m_buttonCopyROM;
	delete m_menuVideo;
	delete m_menuKeyboard;
	delete m_menuGamepad;
	delete m_buttonVideo;
	delete m_buttonKeyboard;
	delete m_buttonGamepad;
}

void MenuSettings::SyncSettings()
{
	m_menuManual->SyncSettings();
	m_menuVideo->SyncSettings();
	m_menuKeyboard->SyncSettings();
	m_menuGamepad->SyncSettings();
}

void MenuSettings::OnButtonManual(const ion::gui::Button& button)
{
	m_gui.PushWindow(*m_menuManual);
}

void MenuSettings::OnButtonCopyROM(const ion::gui::Button& button)
{
	m_dlgROMDisclaimer = new DlgROMDisclaimer(m_gui, m_font, std::bind(&MenuSettings::OnDlgClosedROMDisclaimer, this, std::placeholders::_1));
	m_gui.AddWindow(*m_dlgROMDisclaimer);
}

void MenuSettings::OnButtonVideo(const ion::gui::Button& button)
{
	m_gui.PushWindow(*m_menuVideo);
}

void MenuSettings::OnButtonKeyboard(const ion::gui::Button& button)
{
	m_gui.PushWindow(*m_menuKeyboard);
}

void MenuSettings::OnButtonGamepad(const ion::gui::Button& button)
{
	m_gui.PushWindow(*m_menuGamepad);
}

void MenuSettings::OnDlgClosedROMDisclaimer(const ion::gui::DialogBox& dialog)
{
#if EVIL_EMU_ROM_DOWNLOAD
	bool agreed = m_dlgROMDisclaimer->Agreed();

	m_gui.RemoveWindow(*m_dlgROMDisclaimer);
	m_gui.DeleteWindow(*m_dlgROMDisclaimer);
	m_dlgROMDisclaimer = nullptr;

	if (agreed)
	{
		if (ion::io::FileDevice* device = m_fileSystem.GetDefaultFileDevice())
		{
#if defined ION_PLATFORM_MACOSX
            bool fullscreen = m_appWindow.GetFullscreen();
            int width = m_appWindow.GetClientAreaWidth();
            int height = m_appWindow.GetClientAreaHeight();
            if(fullscreen)
            {
                m_appWindow.SetFullscreen(false, m_settings.displayIdx);
                
                std::vector<ion::Vector2i> resolutions;
                m_appWindow.GetSupportedResolutions(resolutions, m_settings.displayIdx);
                m_appWindow.Resize(resolutions[0].x, resolutions[0].y, false);
            }
#endif
            
			m_appWindow.ShowCursor(true);
			std::string destination = ion::gui::commondlg::DirectoryDialog(&m_appWindow, "Download Mega Drive ROM", "");
			m_appWindow.ShowCursor(false);
            
#if defined ION_PLATFORM_MACOSX
            if(fullscreen)
            {
                m_appWindow.Resize(width, height, true);
                m_appWindow.SetFullscreen(true, m_settings.displayIdx);
            }
#endif

			if (destination.size() > 0)
			{
				destination += std::string("/") + std::string(EVIL_EMU_ROM_FILE);
				std::string source = std::string(EVIL_EMU_ROM_DIRECTORY) + std::string("/") + std::string(EVIL_EMU_ROM_FILE);

				if (device->GetFileExists(destination))
				{
					device->DeleteFile(destination);
				}
			
				if (device->Copyfile(source, destination))
				{
					std::string successMsg = std::string("ROM saved successfully: ") + std::string(EVIL_EMU_ROM_FILE);
					m_msgROMInstructions = new ion::gui::MessageBox("Success", successMsg, ion::gui::MessageBox::Ok,
						[&](const ion::gui::MessageBox&, ion::gui::MessageBox::ButtonType)
						{
							m_gui.RemoveWindow(*m_msgROMInstructions);
							m_gui.DeleteWindow(*m_msgROMInstructions);
							m_msgROMInstructions = nullptr;
						});

					m_gui.AddWindow(*m_msgROMInstructions);
				}
				else
				{
					m_msgROMInstructions = new ion::gui::MessageBox("Error", "Could not write ROM file", ion::gui::MessageBox::Ok,
						[&](const ion::gui::MessageBox&, ion::gui::MessageBox::ButtonType)
						{
							m_gui.RemoveWindow(*m_msgROMInstructions);
							m_gui.DeleteWindow(*m_msgROMInstructions);
							m_msgROMInstructions = nullptr;
						});

					m_gui.AddWindow(*m_msgROMInstructions);
				}
			}
		}
	}
#endif
}
