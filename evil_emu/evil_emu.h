////////////////////////////////////////////////////////////////////////////////
// EvilEmu
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#include <ion/core/bootstrap/Application.h>
#include <ion/core/debug/Debug.h>
#include <ion/core/containers/FixedArray.h>
#include <ion/renderer/Renderer.h>
#include <ion/renderer/Window.h>
#include <ion/renderer/Viewport.h>
#include <ion/renderer/Texture.h>
#include <ion/renderer/Shader.h>
#include <ion/renderer/Material.h>
#include <ion/renderer/Primitive.h>
#include <ion/renderer/Camera.h>
#include <ion/input/Keyboard.h>
#include <ion/input/Mouse.h>
#include <ion/input/Gamepad.h>
#include <ion/io/FileSystem.h>
#include <ion/io/ResourceManager.h>

#include "settings.h"
#include "savegame.h"
#include "megaex/emulator.h"
#include "states/StateGame.h"
#include "states/StatePause.h"

#if ION_ONLINE_STEAM
#include <ion/online/Steam/Steam.h>
#elif ION_ONLINE_GALAXY
#include <ion/online/Galaxy/Galaxy.h>
#endif

class EvilEmu : public ion::framework::Application
{
public:
	EvilEmu();

	virtual bool Initialise();
	virtual void Shutdown();
	virtual bool Update(float deltaTime);
	virtual void Render();

	void ApplySettings();
	void ResetSettings();
	void LoadSettings();
	void SaveSettings();

	Settings m_settings;

private:

	bool InitialiseRenderer();
	bool InitialiseInput();
	bool InitialiseGameStates();
	void ShutdownRenderer();
	void ShutdownInput();
	void ShutdownGameStates();
	bool UpdateInput(float deltaTime);
	bool UpdateGameStates(float deltaTime);

	ion::render::Renderer* m_renderer;
	ion::render::Window* m_window;
	ion::render::Viewport* m_viewport;
	ion::render::Camera* m_camera;
	ion::input::Keyboard* m_keyboard;
	ion::input::Mouse* m_mouse;
	ion::input::Gamepad* m_gamepad;

	ion::io::ResourceManager* m_resourceManager;
	ion::io::FileSystem* m_fileSystem;

#if ION_ONLINE_STEAM
	ion::online::Steam* m_steamInterface;
#elif ION_ONLINE_GALAXY
	ion::online::Galaxy* m_galaxyInterface;
#endif

	ion::render::Texture* m_textureBackground;
	ion::render::Material* m_materialBackground;
	ion::render::Quad* m_quadPrimitiveBackground;

	//States
	ion::gamekit::StateManager m_stateManager;
	StateGame* m_stateGame;
	StatePause* m_statePause;

	//Save system
	SaveManager* m_saveManager;

	//Timing
	u64 m_frameCount;
	u64 m_startTicks;

	int m_currentdisplayIdx;
};