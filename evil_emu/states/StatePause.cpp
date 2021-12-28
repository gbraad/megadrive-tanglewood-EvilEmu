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
#include <ion/engine/Engine.h>

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

#if defined ION_SERVICES_STEAM
	versionText << "s";
#elif defined ION_SERVICES_GALAXY
	versionText << "g";
#elif defined ION_PLATFORM_SWITCH
	versionText << "nx";
#endif

	return versionText.str();
}

StatePause::StatePause(EvilEmu& mainApp, Settings& settings, ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, ion::io::FileSystem& fileSystem, ion::render::Window& window)
	: ion::gamekit::State("pause", stateManager, resourceManager)
	, m_mainApp(mainApp)
	, m_settings(settings)
	, m_appWindow(window)
{
#if EVIL_EMU_USE_UTILITY_MENUS
	m_windowMain = NULL;
	m_buttonResumeGame = NULL;
	m_buttonQuitGame = NULL;
	m_running = true;

	//Create UI
	m_gui = new ion::gui::GUI(ion::Vector2i(m_appWindow.GetClientAreaWidth(), m_appWindow.GetClientAreaHeight()));

	//Load font
	m_font = m_gui->LoadFontTTF(EVIL_EMU_FONT_FILE, EVIL_EMU_FONT_SIZE);

	//Load shader
#if defined ION_RENDERER_SHADER
	m_shaderFlatTextured = m_resourceManager.GetResource<ion::render::Shader>("flattextured",
		[this](ion::render::Shader& shader)
	{
		if(m_gui)
			m_gui->SetShader(&shader);
		if(m_font)
			m_font->SetShader(&shader);
	});
#endif

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
#endif
}

StatePause::~StatePause()
{
#if EVIL_EMU_USE_UTILITY_MENUS
	delete m_windowMain;
	delete m_buttonResumeGame;
	delete m_buttonQuitGame;
	delete m_textVersion;
	delete m_font;
	delete m_gui;
#endif
}

void StatePause::OnEnterState()
{
#if EVIL_EMU_USE_UTILITY_MENUS
	//Update GUI size
	ion::Vector2i resolution(ion::engine.render.window->GetClientAreaWidth(), ion::engine.render.window->GetClientAreaHeight());
	m_gui->SetSize(resolution);

	//Sync settings
	m_windowMain->SyncSettings();

	//Show GUI
	m_gui->SetVisible(true);
#endif
}

void StatePause::OnLeaveState()
{
#if EVIL_EMU_USE_UTILITY_MENUS
	//Hide GUI
	m_gui->SetVisible(false);
#endif
}

void StatePause::OnPauseState()
{

}

void StatePause::OnResumeState()
{

}

void StatePause::OnButtonResumeGame(const ion::gui::Button& button)
{
#if EVIL_EMU_USE_UTILITY_MENUS
	//Apply new settings
	m_mainApp.ApplySettings();

	//Pop state
	m_stateManager.PopState();
#endif
}

void StatePause::OnButtonQuitGame(const ion::gui::Button& button)
{
	m_running = false;
}

bool StatePause::Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, const std::vector<ion::input::Gamepad*>& gamepads)
{
#if EVIL_EMU_USE_UTILITY_MENUS
	m_gui->Update(deltaTime, keyboard, mouse, gamepads[0]);

	if (m_appWindow.HasFocus())
	{
		bool unpause = false;

		for (auto gamepad : gamepads)
		{
			unpause |= gamepad->ButtonPressedThisFrame(ion::input::GamepadButtons::SELECT);
		}

		if (keyboard)
		{
			unpause |= keyboard->KeyPressedThisFrame(ion::input::Keycode::ESCAPE);
		}

		if (unpause)
		{
			//Apply new settings
			m_mainApp.ApplySettings();

			//Pop state
			m_stateManager.PopState();
		}
	}
#endif

	return m_running;
}

void StatePause::Render(ion::render::Renderer& renderer, const ion::render::Camera& camera, ion::render::Viewport& viewport)
{
#if EVIL_EMU_USE_UTILITY_MENUS
	m_gui->Render(renderer, viewport);
#endif
}