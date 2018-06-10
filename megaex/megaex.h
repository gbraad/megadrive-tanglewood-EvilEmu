////////////////////////////////////////////////////////////////////////////////
// megaEx
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
#include <ion/io/ResourceManager.h>

#include "emulator.h"
#include "states/StateControlsConfig.h"
#include "states/StateGame.h"
#include "states/StateMenu.h"

class MegaEx : public ion::framework::Application
{
public:
	MegaEx();

	virtual bool Initialise();
	virtual void Shutdown();
	virtual bool Update(float deltaTime);
	virtual void Render();

private:

	bool InitialiseRenderer();
	bool InitialiseInput();
	bool InitialiseGameStates();
	void ShutdownRenderer();
	void ShutdownInput();
	void ShutdownGameStates();
	bool UpdateInput(float deltaTime);
	bool UpdateGameStates(float deltaTime);

	void ChangeWindowSize(const ion::Vector2i& size);

	ion::render::Renderer* m_renderer;
	ion::render::Window* m_window;
	ion::render::Viewport* m_viewport;
	ion::render::Camera* m_camera;
	ion::input::Keyboard* m_keyboard;
	ion::input::Mouse* m_mouse;
	ion::input::Gamepad* m_gamepad;
	ion::io::ResourceManager* m_resourceManager;

	//States
	ion::gamekit::StateManager m_stateManager;
	ion::gamekit::State* m_stateMenu;
	ion::gamekit::State* m_stateGame;
	ion::gamekit::State* m_stateControlsConfig;

	//Timing
	u64 m_frameCount;
	u64 m_startTicks;

	//Button mapping
	ion::FixedArray<ion::input::Keycode, eBtn_MAX> m_keyboardMap;
	ion::FixedArray<ion::input::Gamepad::Buttons, eBtn_MAX> m_gamepadMap;
};