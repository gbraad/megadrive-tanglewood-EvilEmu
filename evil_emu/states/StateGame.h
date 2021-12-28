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
#include <ion/renderer/Texture.h>
#include <ion/renderer/Shader.h>
#include <ion/renderer/Material.h>
#include <ion/renderer/Primitive.h>
#include <ion/renderer/TexCoord.h>
#include <ion/gui/GUI.h>
#include <ion/core/io/Serialise.h>
#include <ion/services/User.h>
#include <ion/services/UserManager.h>

#if EMU_USE_INPUT_CALLBACKS
#include <ion/input/Keyboard.h>
#include <ion/input/Gamepad.h>
#endif

#include "constants.h"
#include "emulator.h"
#include "emuthread.h"
#include "databridge.h"
#include "debugger/DebuggerUI.h"
#include "StatePause.h"
#include "savegame.h"
#include "settings.h"

#include "menu/MenuSaveSlots.h"

class StateGame : public ion::gamekit::State
{
public:
	StateGame(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, Settings& settings, SaveManager& saveManager, const ion::Vector2i& windowSize, const ion::Vector2i& emulatorSize, ion::render::Window& window, StatePause& pauseState);

	virtual void OnEnterState();
	virtual void OnLeaveState();
	virtual void OnPauseState();
	virtual void OnResumeState();

	virtual bool Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, const std::vector<ion::input::Gamepad*>& gamepads);
	virtual void Render(ion::render::Renderer& renderer, const ion::render::Camera& camera, ion::render::Viewport& viewport);

	void ApplySettings();

protected:
	void SetupRendering();

	void OnSystemMenu(ion::platform::SystemMenu, bool opened);

#if defined ION_SERVICES
	//Begin async query for current user and save game data
	void QueryLoginSaves();

	virtual void OnUserLoggedIn(ion::services::UserManager::LoginResult result, ion::services::User* user);
	virtual void OnUserLoggedOut(ion::services::User& user);

	//Currently logged in user
	ion::services::User* m_currentUser;
#endif

#if EVIL_EMU_USE_SAVES
	//Notify the game that save data is available
	virtual void OnSaveDataLoaded() {}
#endif

#if EVIL_EMU_USE_DATA_BRIDGE
	//68K memory watcher/injector
	DataBridge m_dataBridge;
#endif

#if EVIL_EMU_USE_SAVES
	//Save game manager
	SaveManager m_saveManager;

	//Current save game
	Save m_save;

#if EVIL_EMU_MULTIPLE_SAVESLOTS
	//Multiple save slots menu
	MenuSaveSlots* m_saveSlotsMenu;
#endif
#endif

	ion::render::Window& m_window;
	ion::gui::GUI* m_gui;

private:

#if EMU_USE_INPUT_CALLBACKS
	//Read and supply gamepad input on demand, directly from Mega Drive IO port read routine. Reduces input latency.
	U16 OnInputRequest(int gamepadIdx, bool latch6button);
	ion::input::Keyboard* m_keyboard;
	std::vector<ion::input::Gamepad*> m_gamepads;
	u64 m_lastInputRequestTime;
	bool m_windowHasFocus;
#endif

#if defined ION_SERVICES
	//Mini state machine to login and load save game asynchronously
	enum class LoginSaveQueryState
	{
		Idle,
		LoggingIn,
		LoggedIn,
		SaveInitialising,
		SaveQuerying,
		SaveLoaded,
		SaveInjected,
		Error
	};

	void UpdateLoginSaveQueryState();
#endif

#if EVIL_EMU_USE_SAVES
	void LoadSavesAsync();
	bool SavesAvailable();
#endif

#if EVIL_EMU_USE_SCANLINES
	void DrawScanlineTexture(const ion::Colour& colour);
#endif

	static const ion::render::TexCoord s_texCoordsGame[4];
	static const ion::render::TexCoord s_texCoordsDebugger[4];

	EmulatorThread* m_emulatorThread;
	EmulatorState m_prevEmulatorState;
	bool m_emulatorThreadRunning;

	ion::Vector2i m_windowSize;
	ion::Vector2i m_emulatorSize;

#if EVIL_EMU_USE_2X_BUFFER
	u32* m_pixelScaleBuffer;
#endif

#if EVIL_EMU_USE_SCANLINES
	ion::io::ResourceHandle<ion::render::Texture> m_scanlineTexture;
	ion::render::Material* m_scanlineMaterial;
#endif

	ion::io::ResourceHandle<ion::render::Texture> m_renderTextures[EVIL_EMU_NUM_RENDER_BUFFERS];
	ion::render::Material* m_renderMaterials[EVIL_EMU_NUM_RENDER_BUFFERS];
	ion::render::Quad* m_quadPrimitiveEmu;

	int m_currentBufferIdx;

#if defined ION_RENDERER_SHADER
	ion::io::ResourceHandle<ion::render::Shader> m_shaderFlatTextured;
#endif

	Settings& m_settings;

#if defined ION_PLATFORM_DESKTOP
	StatePause& m_pauseState;
#endif

	DebuggerUI* m_debuggerUI;

#if defined ION_SERVICES
	LoginSaveQueryState m_loginSaveQueryState;
#endif

	//Timing
	float m_emuStartTimer;

	FPSCounter m_fpsCounter;
};
