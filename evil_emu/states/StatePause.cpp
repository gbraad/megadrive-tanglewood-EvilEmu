////////////////////////////////////////////////////////////////////////////////
// EvilEmu
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#include <ion/renderer/Renderer.h>
#include <ion/renderer/Camera.h>
#include <ion/input/Keyboard.h>
#include <ion/input/Mouse.h>
#include <ion/input/Gamepad.h>

#include <sstream>

#include "StatePause.h"
#include "constants.h"
#include "evil_emu.h"

std::string MakeVersionString()
{
	std::stringstream versionText;
	versionText << "ROM: " << EVIL_EMU_ROM_VERSION;

#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
	versionText << "d";
#endif

	versionText << "\nEMU: " << EVIL_EMU_EMU_VERSION;

#if defined ION_PLATFORM_32BIT
	versionText << "-32";
#else
	versionText << "-64";
#endif

#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
	versionText << "d";
#endif

#if EVIL_EMU_DISTRIBUTION==EVIL_EMU_DISTRIBUTION_STEAM
	versionText << "s";
#endif

#if EVIL_EMU_DISTRIBUTION==EVIL_EMU_DISTRIBUTION_GALAXY
	versionText << "g";
#endif

	return versionText.str();
}

StatePause::StatePause(EvilEmu& mainApp, Settings& settings, ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, ion::io::FileSystem& fileSystem, ion::render::Window& window)
	: m_mainApp(mainApp)
	, m_settings(settings)
	, ion::gamekit::State("pause", stateManager, resourceManager)
	, m_appWindow(window)
{
	m_windowMain = NULL;
	m_buttonResumeGame = NULL;
	m_buttonQuitGame = NULL;
	m_running = true;

	//Create UI
	m_gui = new ion::gui::GUI(ion::Vector2i(m_appWindow.GetClientAreaWidth(), m_appWindow.GetClientAreaHeight()));

	//Load font
	m_font = m_gui->LoadFontTTF(EVIL_EMU_FONT_FILE, EVIL_EMU_FONT_SIZE);

	//Set GUI style
	m_gui->StyleSetTitleAlignment(ion::Vector2(0.5f, 0.5f));

	m_windowMain = new MenuSettings(*m_gui, *m_font, fileSystem, m_settings, m_appWindow, ion::Vector2i(), ion::Vector2i());
	m_gui->PushWindow(*m_windowMain);

	//Add pause menu buttons
	m_buttonResumeGame = new ion::gui::Button("Resume Game", std::bind(&StatePause::OnButtonResumeGame, this, std::placeholders::_1));
	m_buttonQuitGame = new ion::gui::Button("Quit Game", std::bind(&StatePause::OnButtonQuitGame, this, std::placeholders::_1));

	m_buttonResumeGame->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));
	m_buttonQuitGame->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));

	m_windowMain->AddWidget(*m_buttonResumeGame);
	m_windowMain->AddWidget(*m_buttonQuitGame);

	//Add version text
	m_textVersion = new ion::gui::TextBox(MakeVersionString());

	m_windowMain->AddWidget(*m_textVersion);

	//Hide GUI
	m_gui->SetVisible(false);
}

StatePause::~StatePause()
{
	delete m_windowMain;
	delete m_buttonResumeGame;
	delete m_buttonQuitGame;
	delete m_textVersion;
	delete m_font;
	delete m_gui;
}

void StatePause::OnEnterState()
{
	//Update GUI size
	ion::Vector2i resolution = m_settings.resolution;

	if (m_settings.fullscreen)
	{
		//Fullscreen uses desktop resolution
		resolution.x = m_appWindow.GetDesktopWidth(m_settings.displayIdx);
		resolution.y = m_appWindow.GetDesktopHeight(m_settings.displayIdx);
	}

	m_gui->SetSize(resolution);

	//Sync settings
	m_windowMain->SyncSettings();

	//Show GUI
	m_gui->SetVisible(true);
}

void StatePause::OnLeaveState()
{
	//Hide GUI
	m_gui->SetVisible(false);
}

void StatePause::OnPauseState()
{

}

void StatePause::OnResumeState()
{

}

void StatePause::OnButtonResumeGame(const ion::gui::Button& button)
{
	//Apply new settings
	m_mainApp.ApplySettings();

	//Pop state
	m_stateManager.PopState();
}

void StatePause::OnButtonQuitGame(const ion::gui::Button& button)
{
	m_running = false;
}

bool StatePause::Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, ion::input::Gamepad* gamepad)
{
	m_gui->Update(deltaTime, keyboard, mouse, gamepad);

	if (m_appWindow.HasFocus())
	{
		if (keyboard->KeyPressedThisFrame(ion::input::Keycode::ESCAPE) || gamepad->ButtonPressedThisFrame(ion::input::GamepadButtons::SELECT))
		{
			//Apply new settings
			m_mainApp.ApplySettings();

			//Pop state
			m_stateManager.PopState();
		}
	}

	return m_running;
}

void StatePause::Render(ion::render::Renderer& renderer, const ion::render::Camera& camera, ion::render::Viewport& viewport)
{
	m_gui->Render(renderer, viewport);
}