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
#include <ion/io/Resourcemanager.h>

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
	void ShutdownRenderer();
	void ShutdownInput();
	bool UpdateInput();

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
	StateControlsConfig* m_stateControlsConfig;
	StateGame* m_stateGame;
	StateMenu* m_stateMenu;
};