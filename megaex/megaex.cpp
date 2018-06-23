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

#include <ion/core/time/Time.h>

#if defined DEBUG
#define EMU_FULLSCREEN 1
#else
#define EMU_FULLSCREEN 1
#endif

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

	m_materialBackground = NULL;
	m_quadPrimitiveBackground = NULL;

	m_stateControlsConfig = NULL;
	m_stateGame = NULL;
	m_stateMenu = NULL;

	m_frameCount = 0;
	m_startTicks = 0;

	//Default keymap
	m_keyboardMap[eBtn_Up] = ion::input::W;
	m_keyboardMap[eBtn_Down] = ion::input::S;
	m_keyboardMap[eBtn_Left] = ion::input::A;
	m_keyboardMap[eBtn_Right] = ion::input::D;
	m_keyboardMap[eBtn_A] = ion::input::J;
	m_keyboardMap[eBtn_B] = ion::input::K;
	m_keyboardMap[eBtn_C] = ion::input::L;
	m_keyboardMap[eBtn_Start] = ion::input::SPACE;

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
	m_resourceManager->SetResourceDirectory<ion::render::Texture>("textures");

	if(!InitialiseRenderer())
	{
		return false;
	}

	if(!InitialiseInput())
	{
		return false;
	}

	if(!InitialiseGameStates())
	{
		return false;
	}

	//Load background texture
	m_textureBackground = ion::render::Texture::Create();
	if (m_textureBackground->Load("textures/emu_bg.png"))
	{
		//Create background material and quad
		m_materialBackground = new ion::render::Material();
		m_quadPrimitiveBackground = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2((float)m_window->GetClientAreaWidth() / 2.0f, (float)m_window->GetClientAreaHeight() / 2.0f));
		m_materialBackground->SetDiffuseColour(ion::Colour(0.0f, 0.0f, 1.0f));
		m_materialBackground->AddDiffuseMap(m_textureBackground);
	}

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

	//Draw BG quad
	if (m_materialBackground)
	{
		m_materialBackground->Bind(ion::Matrix4(), m_camera->GetTransform().GetInverse(), m_renderer->GetProjectionMatrix());
		m_renderer->DrawVertexBuffer(m_quadPrimitiveBackground->GetVertexBuffer(), m_quadPrimitiveBackground->GetIndexBuffer());
		m_materialBackground->Unbind();
	}

	//Render current state
	m_stateManager.Render(*m_renderer, *m_camera);

	m_renderer->SwapBuffers();
	m_renderer->EndFrame();
}

bool MegaEx::InitialiseRenderer()
{
	//Initialise at default size, windowed, first
	m_window = ion::render::Window::Create("TANGLEWOOD", DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, false);
	m_renderer = ion::render::Renderer::Create(m_window->GetDeviceContext());
	m_viewport = new ion::render::Viewport(m_window->GetClientAreaWidth(), m_window->GetClientAreaHeight(), ion::render::Viewport::eOrtho2DAbsolute);
	m_camera = new ion::render::Camera();

	m_viewport->SetClearColour(ion::Colour(1.0f, 0.0f, 0.0f, 1.0f));
	m_camera->SetPosition(ion::Vector3(-(float)m_window->GetClientAreaWidth() / 2.0f, -(float)m_window->GetClientAreaHeight() / 2.0f, 0.1f));

#if EMU_FULLSCREEN
	//Attempt to resize to desktop
	if (ChangeWindowSize(ion::Vector2i(m_window->GetDesktopWidth(), m_window->GetDesktopHeight()), true))
	{
		//Set fullscreen
		if (!m_window->SetFullscreen(true))
		{
			//Failed, revert to original size
			ChangeWindowSize(ion::Vector2i(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT), false);
		}
	}
#endif

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

	for(int i = 0; i < eBtn_MAX; i++)
	{
		if(m_keyboard->KeyDown(m_keyboardMap[i]) || m_gamepad->ButtonDown(m_gamepadMap[i]))
		{
			buttonState |= g_emulatorButtonBits[i];
		}
	}

	EmulatorSetButtonState(buttonState);

	return !m_keyboard->KeyDown(ion::input::ESCAPE);
}

bool MegaEx::InitialiseGameStates()
{
	//Create states
	m_stateControlsConfig = new StateControlsConfig(m_stateManager, *m_resourceManager, m_keyboardMap);
	m_stateGame = new StateGame(m_stateManager, *m_resourceManager, ion::Vector2i(m_window->GetClientAreaWidth(), m_window->GetClientAreaHeight()), ion::Vector2i(WIDTH, HEIGHT), *m_window);
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

bool MegaEx::ChangeWindowSize(const ion::Vector2i& size, bool fullscreen)
{
	if (m_window->Resize(size.x, size.y, !fullscreen))
	{
		m_viewport->Resize(m_window->GetClientAreaWidth(), m_window->GetClientAreaHeight());
		m_camera->SetPosition(ion::Vector3(-(float)m_window->GetClientAreaWidth() / 2.0f, -(float)m_window->GetClientAreaHeight() / 2.0f, 0.1f));
		m_renderer->OnResize(m_window->GetClientAreaWidth(), m_window->GetClientAreaHeight());

		//Recreate BG quad
		if (m_quadPrimitiveBackground)
		{
			delete m_quadPrimitiveBackground;
			m_quadPrimitiveBackground = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2((float)m_window->GetClientAreaWidth() / 2.0f, (float)m_window->GetClientAreaHeight() / 2.0f));
		}

		//Notify game state
		if (m_stateGame)
		{
			m_stateGame->ChangeWindowSize(size);
		}

		return true;
	}

	return false;
}