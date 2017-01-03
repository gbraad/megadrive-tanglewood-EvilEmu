////////////////////////////////////////////////////////////////////////////////
// megaEx
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#include "megaex.h"
#include "config.h"

#include <ion/core/debug/Debug.h>

MegaEx::MegaEx() : ion::framework::Application("megaEx")
{
	m_renderer = NULL;
	m_window = NULL;
	m_viewport = NULL;
	m_camera = NULL;
	m_keyboard = NULL;
	m_mouse = NULL;
	m_gamepad = NULL;
	m_resourceManager = NULL;

	m_stateControlsConfig = NULL;
	m_stateGame = NULL;
	m_stateMenu = NULL;

	//Default keymap
#if defined ION_PLATFORM_WINDOWS
	m_keyboardMap[eBtn_Up] = DIK_UP;
	m_keyboardMap[eBtn_Down] = DIK_DOWN;
	m_keyboardMap[eBtn_Left] = DIK_LEFT;
	m_keyboardMap[eBtn_Right] = DIK_RIGHT;
	m_keyboardMap[eBtn_A] = DIK_A;
	m_keyboardMap[eBtn_B] = DIK_S;
	m_keyboardMap[eBtn_C] = DIK_D;
	m_keyboardMap[eBtn_Start] = DIK_RETURN;
#endif

	m_gamepadMap[eBtn_Up] = ion::input::Gamepad::DPAD_UP;
	m_gamepadMap[eBtn_Down] = ion::input::Gamepad::DPAD_DOWN;
	m_gamepadMap[eBtn_Left] = ion::input::Gamepad::DPAD_LEFT;
	m_gamepadMap[eBtn_Right] = ion::input::Gamepad::DPAD_RIGHT;
	m_gamepadMap[eBtn_A] = ion::input::Gamepad::BUTTON_X;
	m_gamepadMap[eBtn_B] = ion::input::Gamepad::BUTTON_A;
	m_gamepadMap[eBtn_C] = ion::input::Gamepad::BUTTON_B;
	m_gamepadMap[eBtn_Start] = ion::input::Gamepad::START;
}

bool MegaEx::Initialise()
{
	//Create resource manager
	m_resourceManager = new ion::io::ResourceManager();

	if(!InitialiseRenderer())
	{
		ion::debug::Error("Error initialising renderer");
		return false;
	}

	if(!InitialiseInput())
	{
		ion::debug::Error("Error initialising input");
		return false;
	}

#if defined ION_PLATFORM_DREAMCAST
	//if(!InitialiseEmulator("/cd/roms/sonic.md"))
	if(!InitialiseEmulator("/cd/roms/TANGLEWD.BIN"))
#else
	//if(!InitialiseEmulator("ROMS\\sonic.md"))
	if(!InitialiseEmulator("ROMS\\TANGLEWD.BIN"))
#endif
	{
		ion::debug::Error("Error initialising emulator");
		return false;
	}

	if(!InitialiseGameStates())
	{
		ion::debug::Error("Error initialising gamestates");
		return false;
	}

	//Set window title
	m_window->SetTitle(EmulatorGetROMTitle());

	//ChangeWindowSize(ion::Vector2i(m_viewport->GetWidth() * 2, m_viewport->GetHeight() * 2));

	return true;
}

void MegaEx::Shutdown()
{
	ShutdownGameStates();
	ShutdownInput();
	ShutdownRenderer();
}

bool MegaEx::Update(float deltaTime)
{
	//Update input
	bool inputQuit = !UpdateInput(deltaTime);

	//Update window
	bool windowQuit = !m_window->Update();

	//Update game state
	bool gameStateQuit = !UpdateGameStates(deltaTime);

	return !windowQuit && !inputQuit && !gameStateQuit;
}

void MegaEx::Render()
{
	m_renderer->BeginFrame(*m_viewport, m_window->GetDeviceContext());
	m_renderer->ClearColour();
	m_renderer->ClearDepth();

	//Render current state
	m_stateManager.Render(*m_renderer, *m_camera);

	m_renderer->SwapBuffers();
	m_renderer->EndFrame();
}

bool MegaEx::InitialiseRenderer()
{
	m_window = ion::render::Window::Create("megaEx", DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, false);
	m_renderer = ion::render::Renderer::Create(m_window->GetDeviceContext());
	m_viewport = new ion::render::Viewport(m_window->GetClientAreaWidth(), m_window->GetClientAreaHeight(), ion::render::Viewport::eOrtho2DAbsolute);
	m_camera = new ion::render::Camera();

	m_viewport->SetClearColour(ion::Colour(0.0f, 0.0f, 0.0f, 1.0f));
	m_camera->SetPosition(ion::Vector3(-(float)m_window->GetClientAreaWidth() / 2.0f, -(float)m_window->GetClientAreaHeight() / 2.0f, -0.1f));

	return true;
}

void MegaEx::ShutdownRenderer()
{
	if(m_camera)
		delete m_camera;

	if(m_viewport)
		delete m_viewport;

	if(m_renderer)
		delete m_renderer;

	if(m_window)
		delete m_window;
}

bool MegaEx::InitialiseInput()
{
	m_keyboard = new ion::input::Keyboard();
	m_mouse = new ion::input::Mouse();
	m_gamepad = new ion::input::Gamepad();
	return true;
}

void MegaEx::ShutdownInput()
{
	if(m_gamepad)
		delete m_gamepad;

	if(m_mouse)
		delete m_mouse;

	if(m_keyboard)
		delete m_keyboard;
}

bool MegaEx::UpdateInput(float deltaTime)
{
	m_keyboard->Update();
	m_mouse->Update();
	m_gamepad->Update();

	u16 buttonState = 0;

#if !defined ION_PLATFORM_DREAMCAST
	for(int i = 0; i < eBtn_MAX; i++)
	{
		if(m_keyboard->KeyDown(m_keyboardMap[i]) || m_gamepad->ButtonDown((ion::input::Gamepad::Buttons)m_gamepadMap[i]))
		{
			buttonState |= g_emulatorButtonBits[i];
		}
	}
#endif

	EmulatorSetButtonState(buttonState);

#if defined ION_PLATFORM_WINDOWS
	return !m_keyboard->KeyDown(DIK_ESCAPE);
#else
	return true;
#endif
}

bool MegaEx::InitialiseGameStates()
{
	//Create states
	m_stateControlsConfig = new StateControlsConfig(m_stateManager, *m_resourceManager, m_keyboardMap);
	m_stateGame = new StateGame(m_stateManager, *m_resourceManager, ion::Vector2i(m_window->GetClientAreaWidth(), m_window->GetClientAreaHeight()), ion::Vector2i(WIDTH, HEIGHT));
	m_stateMenu = new StateMenu(m_stateManager, *m_resourceManager, m_stateControlsConfig, m_stateGame);

	//Push first state
	m_stateManager.PushState(*m_stateMenu);

	return true;
}

void MegaEx::ShutdownGameStates()
{
	if(m_stateMenu)
		delete m_stateMenu;

	if(m_stateGame)
		delete m_stateGame;

	if(m_stateControlsConfig)
		delete m_stateControlsConfig;
}

bool MegaEx::UpdateGameStates(float deltaTime)
{
	m_stateManager.Update(deltaTime, m_keyboard, m_mouse, m_gamepad);
	return true;
}

void MegaEx::ChangeWindowSize(const ion::Vector2i& size)
{
	m_window->Resize(size.x, size.y);
	m_viewport->Resize(size.x, size.y);
	m_camera->SetPosition(ion::Vector3(-size.x / 2.0f, -size.y / 2.0f, 0.1f));
	m_renderer->OnResize(size.x, size.y);

	//Recreate quad
	//delete m_quadPrimitive;
	//m_quadPrimitive = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2(size.x / 2, size.y / 2));
	//m_quadPrimitive->SetTexCoords(s_texCoordsGame);
}