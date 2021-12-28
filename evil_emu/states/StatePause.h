////////////////////////////////////////////////////////////////////////////////
// EvilEmu
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ion/gamekit/StateManager.h>
#include <ion/gui/GUI.h>
#include <ion/gui/Font.h>
#include <ion/gui/Window.h>
#include <ion/gui/Button.h>
#include <ion/gui/CheckBox.h>
#include <ion/gui/ComboBox.h>
#include <ion/gui/TextBox.h>

#include "constants.h"
#include "settings.h"
#include "menu/MenuCommon.h"
#include "menu/MenuSettings.h"

class EvilEmu;

class StatePause : public ion::gamekit::State
{
public:
	StatePause(EvilEmu& mainApp, Settings& settings, ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, ion::io::FileSystem& fileSystem, ion::render::Window& window);
	~StatePause();

	virtual void OnEnterState();
	virtual void OnLeaveState();
	virtual void OnPauseState();
	virtual void OnResumeState();

	virtual bool Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, const std::vector<ion::input::Gamepad*>& gamepads);
	virtual void Render(ion::render::Renderer& renderer, const ion::render::Camera& camera, ion::render::Viewport& viewport);

private:
	void OnButtonResumeGame(const ion::gui::Button& button);
	void OnButtonQuitGame(const ion::gui::Button& button);

	EvilEmu& m_mainApp;
	Settings& m_settings;

	ion::render::Window& m_appWindow;

#if defined ION_RENDERER_SHADER
	ion::io::ResourceHandle<ion::render::Shader> m_shaderFlatTextured;
#endif

#if EVIL_EMU_USE_UTILITY_MENUS
	ion::gui::GUI* m_gui;
	ion::gui::Font* m_font;
	ion::gui::Button* m_buttonResumeGame;
	ion::gui::Button* m_buttonQuitGame;
	ion::gui::TextBox* m_textVersion;
	MenuSettings* m_windowMain;
#endif

	bool m_running;
};