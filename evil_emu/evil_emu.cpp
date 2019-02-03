////////////////////////////////////////////////////////////////////////////////
// EvilEmu
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#include "evil_emu.h"
#include "constants.h"
#include "megaex/config.h"
#include "megaex/vdp/vdp.h"

#include <ion/core/time/Time.h>

EvilEmu::EvilEmu() : ion::framework::Application("EvilEmu")
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
	m_textureBackground = NULL;

	m_statePause = NULL;
	m_stateGame = NULL;

	m_frameCount = 0;
	m_startTicks = 0;
}

bool EvilEmu::Initialise()
{
#if ION_ONLINE_STEAM
	//Initialise Steam API
	m_steamInterface = new ion::online::Steam();
#elif ION_ONLINE_GALAXY
	//Initialise GOG API
	m_galaxyInterface = new ion::online::Galaxy(EVIL_EMU_APP_ID_GALAXY, EVIL_EMU_APP_KEY_GALAXY);
#endif

	//Create filesystem
	m_fileSystem = new ion::io::FileSystem();

	//Create resource manager
	m_resourceManager = new ion::io::ResourceManager();
	m_resourceManager->SetResourceDirectory<ion::render::Texture>("textures", ".ion.texture");

	//Create save system
	m_saveManager = new SaveManager(*m_fileSystem);

	if(!InitialiseRenderer())
	{
		ion::debug::Log("Failed to intialise renderer - please check your graphics device drivers");
		return false;
	}

	if(!InitialiseInput())
	{
		ion::debug::Log("Failed to intialise input system - please check your device drivers");
		return false;
	}

	if(!InitialiseGameStates())
	{
		return false;
	}

	//Load and apply settings (or create default)
	LoadSettings();
	ApplySettings();

	//Hide mouse cursor
	m_window->ShowCursor(false);

	ion::debug::Log("\nT A N G L E W O O D");
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
	ion::debug::Log("-- DEMO --");
#endif
	ion::debug::Log("Copyright (C) 2018 Big Evil Corporation Ltd");
	ion::debug::Log("http://www.evil_emugame.com\n");

	return true;
}

void EvilEmu::Shutdown()
{
	ShutdownGameStates();
	ShutdownInput();
	ShutdownRenderer();

	delete m_quadPrimitiveBackground;
	delete m_textureBackground;
	delete m_saveManager;
	delete m_resourceManager;
	delete m_fileSystem;

#if ION_ONLINE_STEAM
	delete m_steamInterface;
#endif
}

bool EvilEmu::Update(float deltaTime)
{
	//Update input
	bool inputQuit = !UpdateInput(deltaTime);

	//Update window
	bool windowQuit = !m_window->Update();

	//Update game state
	bool gameStateQuit = !UpdateGameStates(deltaTime);

	//Update online system
#if ION_ONLINE_GALAXY
	m_galaxyInterface->Update();
#endif

	return !windowQuit && !inputQuit && !gameStateQuit;
}

void EvilEmu::Render()
{
	m_renderer->BeginFrame(*m_viewport, m_window->GetDeviceContext());

	if (m_settings.videoBorder == VideoBorder::VDPColour)
	{
		u32 bgColour = VDP_GetBackgroundColourRGBA();
		m_renderer->SetClearColour(ion::Colour(bgColour));
	}

	m_renderer->ClearColour();
	m_renderer->ClearDepth();

	//Draw BG quad
	if (m_quadPrimitiveBackground && m_materialBackground)
	{
		m_materialBackground->Bind(ion::Matrix4(), m_camera->GetTransform().GetInverse(), m_renderer->GetProjectionMatrix());
		m_renderer->SetAlphaBlending(ion::render::Renderer::eNoBlend);
		m_renderer->DrawVertexBuffer(m_quadPrimitiveBackground->GetVertexBuffer(), m_quadPrimitiveBackground->GetIndexBuffer());
		m_materialBackground->Unbind();
	}

	//Render current state
	m_stateManager.Render(*m_renderer, *m_camera, *m_viewport);

	m_renderer->SwapBuffers();
	m_renderer->EndFrame();
}

bool EvilEmu::InitialiseRenderer()
{
	//Initialise windowed (will go fullscreen when settings applied)
	m_window = ion::render::Window::Create("EVIL_EMU", m_settings.resolution.x, m_settings.resolution.y, false);
	m_renderer = ion::render::Renderer::Create(m_window->GetDeviceContext());
	m_viewport = new ion::render::Viewport(m_window->GetClientAreaWidth(), m_window->GetClientAreaHeight(), ion::render::Viewport::eOrtho2DAbsolute);
	m_camera = new ion::render::Camera();

	m_viewport->SetClearColour(ion::Colour(0.0f, 0.0f, 0.0f, 1.0f));
	m_camera->SetPosition(ion::Vector3(-(float)m_window->GetClientAreaWidth() / 2.0f, -(float)m_window->GetClientAreaHeight() / 2.0f, 0.1f));

	return true;
}

void EvilEmu::ShutdownRenderer()
{
	//Exit fullscreen mode
	m_window->SetFullscreen(false, 0);

	if(m_camera)
		delete m_camera;

	if(m_viewport)
		delete m_viewport;

	if(m_renderer)
		delete m_renderer;

	if(m_window)
		delete m_window;
}

bool EvilEmu::InitialiseInput()
{
	m_keyboard = new ion::input::Keyboard();
	m_mouse = new ion::input::Mouse();
	m_gamepad = new ion::input::Gamepad();
	return true;
}

void EvilEmu::ShutdownInput()
{
	if(m_gamepad)
		delete m_gamepad;

	if(m_mouse)
		delete m_mouse;

	if(m_keyboard)
		delete m_keyboard;
}

bool EvilEmu::UpdateInput(float deltaTime)
{
	m_keyboard->Update();
	m_mouse->Update();
	m_gamepad->Update();

	return true;
}

bool EvilEmu::InitialiseGameStates()
{
	//Create states
	m_statePause = new StatePause(*this, m_settings, m_stateManager, *m_resourceManager, *m_fileSystem, *m_window);
	m_stateGame = new StateGame(m_stateManager, *m_resourceManager, m_settings, *m_saveManager, ion::Vector2i(m_window->GetClientAreaWidth(), m_window->GetClientAreaHeight()), ion::Vector2i(WIDTH, HEIGHT), *m_window, *m_statePause);

	//Push first state
	m_stateManager.PushState(*m_stateGame);

	return true;
}

void EvilEmu::ShutdownGameStates()
{
	delete m_stateGame;
	delete m_statePause;
}

bool EvilEmu::UpdateGameStates(float deltaTime)
{
	return m_stateManager.Update(deltaTime, m_keyboard, m_mouse, m_gamepad);
}

void EvilEmu::ResetSettings()
{
	m_settings = Settings();

	//Get desktop screen size
	m_settings.resolution.x = m_window->GetDesktopWidth(m_settings.displayIdx);
	m_settings.resolution.y = m_window->GetDesktopHeight(m_settings.displayIdx);

#if EMU_FULLSCREEN
	m_settings.fullscreen = true;
#else
	m_settings.fullscreen = false;
#endif
}

void EvilEmu::LoadSettings()
{
	if (!m_saveManager->LoadSettings(m_settings))
	{
		ResetSettings();
		SaveSettings();
	}
}

void EvilEmu::SaveSettings()
{
	m_saveManager->SaveSettings(m_settings);
}

void EvilEmu::ApplySettings()
{
	if (m_window && m_renderer)
	{
		ion::Vector2i resolution = m_settings.resolution;

		if (m_settings.fullscreen)
		{
			//Fullscreen uses desktop resolution
			resolution.x = m_window->GetDesktopWidth(m_settings.displayIdx);
			resolution.y = m_window->GetDesktopHeight(m_settings.displayIdx);
		}

		//If display changed, un-fullscreen first
		if (m_window->GetFullscreen() && (m_settings.displayIdx != m_currentdisplayIdx))
		{
			m_window->SetFullscreen(false, m_settings.displayIdx);
			m_currentdisplayIdx = m_settings.displayIdx;
		}

		//Try resizing
		if (m_window->Resize(resolution.x, resolution.y, !m_settings.fullscreen))
		{
			bool fullscreenChanged = m_window->GetFullscreen() != m_settings.fullscreen;

#if EMU_FULLSCREEN
			//Set fullscreen
			if (fullscreenChanged && !m_window->SetFullscreen(m_settings.fullscreen, m_settings.displayIdx))
			{
				//Failed, revert to original size
				m_window->Resize(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, true);
			}
			else
#endif
			{
				m_viewport->Resize(m_window->GetClientAreaWidth(), m_window->GetClientAreaHeight());
				m_camera->SetPosition(ion::Vector3(-(float)m_window->GetClientAreaWidth() / 2.0f, -(float)m_window->GetClientAreaHeight() / 2.0f, 0.1f));
				m_renderer->OnResize(m_window->GetClientAreaWidth(), m_window->GetClientAreaHeight());

				//Reset clear colour
				m_renderer->SetClearColour(ion::Colour(0.0f, 0.0f, 0.0f, 0.0f));

				//Recreate BG quad
				if (m_quadPrimitiveBackground)
				{
					delete m_quadPrimitiveBackground;
					m_quadPrimitiveBackground = nullptr;
				}

				if (m_textureBackground)
				{
					delete m_textureBackground;
					m_textureBackground = nullptr;
				}

				//Load background texture
				switch (m_settings.videoBorder)
				{
					case VideoBorder::ImageGameCover:
					{
						m_quadPrimitiveBackground = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2((float)m_window->GetClientAreaWidth() / 2.0f, (float)m_window->GetClientAreaHeight() / 2.0f));
						m_textureBackground = ion::render::Texture::Create();
						if (m_textureBackground->Load("textures/emu_bg.png"))
						{
							m_materialBackground = new ion::render::Material();
							m_materialBackground->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));
							m_materialBackground->AddDiffuseMap(m_textureBackground);
						}

						break;
					}

					case VideoBorder::ImageBlackGrid:
					{
						m_quadPrimitiveBackground = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2((float)m_window->GetClientAreaWidth() / 2.0f, (float)m_window->GetClientAreaHeight() / 2.0f));
						m_textureBackground = ion::render::Texture::Create();
						if (m_textureBackground->Load("textures/emu_bg_black_grid.png"))
						{
							m_materialBackground = new ion::render::Material();
							m_materialBackground->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));
							m_materialBackground->AddDiffuseMap(m_textureBackground);
							m_textureBackground->SetWrapping(ion::render::Texture::Wrapping::Repeat);
							m_textureBackground->SetMinifyFilter(ion::render::Texture::Filter::Nearest);
							m_textureBackground->SetMagnifyFilter(ion::render::Texture::Filter::Nearest);

							float coordx = (float)m_window->GetClientAreaWidth() / (float)m_textureBackground->GetWidth();
							float coordy = (float)m_window->GetClientAreaHeight() / (float)m_textureBackground->GetHeight();

							ion::render::TexCoord texCoords[4] =
							{
								ion::Vector2(0.0f, 0.0f),
								ion::Vector2(0.0f, coordy),
								ion::Vector2(coordx, coordy),
								ion::Vector2(coordx, 0.0f)
							};

							m_quadPrimitiveBackground->SetTexCoords(texCoords);
						}

						break;
					}

					case VideoBorder::ImageBlueBorder:
					{
						m_quadPrimitiveBackground = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2((float)m_window->GetClientAreaWidth() / 2.0f, (float)m_window->GetClientAreaHeight() / 2.0f));
						m_textureBackground = ion::render::Texture::Create();
						if (m_textureBackground->Load("textures/emu_bg_blue_border.png"))
						{
							m_materialBackground = new ion::render::Material();
							m_materialBackground->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));
							m_materialBackground->AddDiffuseMap(m_textureBackground);
							m_textureBackground->SetWrapping(ion::render::Texture::Wrapping::Repeat);
							m_textureBackground->SetMinifyFilter(ion::render::Texture::Filter::Nearest);
							m_textureBackground->SetMagnifyFilter(ion::render::Texture::Filter::Nearest);

							float coordx = (float)m_window->GetClientAreaWidth() / (float)m_textureBackground->GetWidth();
							float coordy = (float)m_window->GetClientAreaHeight() / (float)m_textureBackground->GetHeight();

							ion::render::TexCoord texCoords[4] =
							{
								ion::Vector2(0.0f, 0.0f),
								ion::Vector2(0.0f, coordy),
								ion::Vector2(coordx, coordy),
								ion::Vector2(coordx, 0.0f)
							};

							m_quadPrimitiveBackground->SetTexCoords(texCoords);
						}

						break;
					}
				};
			}
		}

		//Apply vsync
		m_renderer->EnableVSync(m_settings.vsync);
	}

	//Notify game state
	if (m_stateGame)
	{
		m_stateGame->ApplySettings();
	}

	SaveSettings();
}
