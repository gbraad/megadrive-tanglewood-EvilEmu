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

#include <ion/engine/Engine.h>

#include "tanglewood/GameStateTanglewood.h"

EvilEmu::EvilEmu() : ion::framework::Application(EVIL_EMU_APP_TITLE)
{
	m_materialBackground = NULL;
	m_quadPrimitiveBackground = NULL;

	m_statePause = NULL;
	m_stateGame = NULL;

	m_frameCount = 0;
	m_startTicks = 0;
}

bool EvilEmu::Initialise()
{
	//Create save system
	m_saveManager = new SaveManager(*ion::engine.io.fileSystem);

	//Load and apply settings (or create default)
	LoadSettings();
	ApplySettings();

	if(!InitialiseGameStates())
	{
		return false;
	}

	//Subscribe to resolution changes
	ion::engine.render.window->RegisterDisplayChangedCallback(std::bind(&EvilEmu::OnDisplayChanged, this, std::placeholders::_1, std::placeholders::_2));

	//Centre camera
	ion::engine.render.camera->SetPosition(ion::Vector3(-(float)ion::engine.render.window->GetClientAreaWidth() / 2.0f, -(float)ion::engine.render.window->GetClientAreaHeight() / 2.0f, 0.1f));

	//Hide mouse cursor
	ion::engine.render.window->ShowCursor(false);

#if defined ION_RENDERER_SHADER
	//Load shaders
	m_shaderFlatTextured = ion::engine.io.resourceManager->GetResource<ion::render::Shader>("flattextured");
#endif

	//Wait for all resources to load
	ion::engine.io.resourceManager->WaitForResources();

	return true;
}

void EvilEmu::Shutdown()
{
	ShutdownGameStates();

	delete m_quadPrimitiveBackground;
	m_textureBackground.Clear();
	delete m_saveManager;

	ion::engine.Shutdown();
}

bool EvilEmu::Update(float deltaTime)
{
	//Engine update
	bool engineQuit = !ion::engine.Update(deltaTime);

	//Update game state
	bool gameStateQuit = !UpdateGameStates(deltaTime);

	return !engineQuit && !gameStateQuit;
}

void EvilEmu::Render()
{
	if (m_settings.videoBorder == VideoBorder::VDPColour)
	{
		u32 bgColour = VDP_GetBackgroundColourRGBA();
		ion::engine.render.viewport->SetClearColour(ion::Colour(bgColour));
	}

	ion::engine.BeginRenderFrame();

	//Draw BG quad
	if (m_quadPrimitiveBackground && m_materialBackground)
	{
#if defined ION_RENDERER_SHADER
		if (m_materialBackground->GetShader())
#endif
		{
			ion::Matrix4 emuMatrix;
			emuMatrix.SetTranslation(ion::Vector3(0.0f, 0.0f, 1.0f));
			ion::engine.render.renderer->BindMaterial(*m_materialBackground, emuMatrix, ion::engine.render.camera->GetTransform().GetInverse(), ion::engine.render.renderer->GetProjectionMatrix());
			ion::engine.render.renderer->SetAlphaBlending(ion::render::Renderer::AlphaBlendType::None);
			ion::engine.render.renderer->DrawVertexBuffer(m_quadPrimitiveBackground->GetVertexBuffer(), m_quadPrimitiveBackground->GetIndexBuffer());
			ion::engine.render.renderer->UnbindMaterial(*m_materialBackground);
		}
	}

	//Render current state
	m_stateManager.Render(*ion::engine.render.renderer, *ion::engine.render.camera, *ion::engine.render.viewport);

	ion::engine.EndRenderFrame();
}

bool EvilEmu::InitialiseGameStates()
{
	//Create states
	m_statePause = new StatePause(*this, m_settings, m_stateManager, *ion::engine.io.resourceManager, *ion::engine.io.fileSystem, *ion::engine.render.window);
	m_stateGame = new GameStateType(m_stateManager, *ion::engine.io.resourceManager, m_settings, *m_saveManager, ion::Vector2i(ion::engine.render.window->GetClientAreaWidth(), ion::engine.render.window->GetClientAreaHeight()), ion::Vector2i(DRAW_BUFFER_WIDTH, DRAW_BUFFER_HEIGHT), *ion::engine.render.window, *m_statePause);

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
	return m_stateManager.Update(deltaTime, ion::engine.input.keyboard, ion::engine.input.mouse, ion::engine.input.gamepads);
}

void EvilEmu::OnDisplayChanged(int displayIdx, const ion::Vector2i& resolution)
{
	m_settings.resolution.x = resolution.x;
	m_settings.resolution.y = resolution.y;
	ApplySettings();
}

void EvilEmu::ResetSettings()
{
	m_settings = Settings();

	//Get desktop screen size
	m_settings.resolution.x = ion::engine.render.window->GetDesktopWidth(m_settings.displayIdx);
	m_settings.resolution.y = ion::engine.render.window->GetDesktopHeight(m_settings.displayIdx);

#if EVIL_EMU_FULLSCREEN
	m_settings.fullscreen = true;
#else
	m_settings.fullscreen = false;
#endif
}

void EvilEmu::LoadSettings()
{
#if EVIL_EMU_STORE_SETTINGS
	if (!m_saveManager->LoadSettings(m_settings))
	{
		ResetSettings();
		SaveSettings();
	}
#else
	ResetSettings();
#endif
}

void EvilEmu::SaveSettings()
{
#if EVIL_EMU_STORE_SETTINGS
	m_saveManager->SaveSettings(m_settings);
#endif
}

void EvilEmu::ApplySettings()
{
	if (ion::engine.render.window && ion::engine.render.renderer)
	{
		ion::Vector2i resolution = m_settings.resolution;

#if EVIL_EMU_FULLSCREEN
		if (m_settings.fullscreen)
		{
			//Fullscreen uses desktop resolution
			resolution.x = ion::engine.render.window->GetDesktopWidth(m_settings.displayIdx);
			resolution.y = ion::engine.render.window->GetDesktopHeight(m_settings.displayIdx);
		}
#endif

		//If display changed, un-fullscreen first
		if (ion::engine.render.window->GetFullscreen() && (m_settings.displayIdx != m_currentdisplayIdx))
		{
			ion::engine.render.window->SetFullscreen(false, m_settings.displayIdx);
			m_currentdisplayIdx = m_settings.displayIdx;
		}

		//Try resizing
		if (ion::engine.render.window->Resize(resolution.x, resolution.y, !m_settings.fullscreen))
		{
#if EVIL_EMU_FULLSCREEN
			bool fullscreenChanged = ion::engine.render.window->GetFullscreen() != m_settings.fullscreen;

			//Set fullscreen
			if (!fullscreenChanged || ion::engine.render.window->SetFullscreen(m_settings.fullscreen, m_settings.displayIdx))
#endif
			{
				ion::engine.render.viewport->Resize(ion::engine.render.window->GetClientAreaWidth(), ion::engine.render.window->GetClientAreaHeight());
				ion::engine.render.camera->SetPosition(ion::Vector3(-(float)ion::engine.render.window->GetClientAreaWidth() / 2.0f, -(float)ion::engine.render.window->GetClientAreaHeight() / 2.0f, 0.1f));
				ion::engine.render.renderer->OnResize(ion::engine.render.window->GetClientAreaWidth(), ion::engine.render.window->GetClientAreaHeight());

				//Reset clear colour
				ion::engine.render.renderer->SetClearColour(ion::Colour(0.0f, 0.0f, 0.0f, 0.0f));

				//Recreate BG quad
				if (m_quadPrimitiveBackground)
				{
					delete m_quadPrimitiveBackground;
					m_quadPrimitiveBackground = nullptr;
				}

				m_textureBackground.Clear();

				//Load background texture
				switch (m_settings.videoBorder)
				{
					case VideoBorder::ImageGameCover:
					{
						m_quadPrimitiveBackground = new ion::render::Quad(ion::render::Quad::Axis::xy, ion::Vector2((float)ion::engine.render.window->GetClientAreaWidth() / 2.0f, (float)ion::engine.render.window->GetClientAreaHeight() / 2.0f));
						m_textureBackground = ion::engine.io.resourceManager->GetResource<ion::render::Texture>("emu_bg.png");
						m_materialBackground = new ion::render::Material();
						m_materialBackground->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));
						m_materialBackground->AddDiffuseMap(m_textureBackground);

#if defined ION_RENDERER_SHADER
						m_materialBackground->SetShader(m_shaderFlatTextured);
#endif

						break;
					}

					case VideoBorder::ImageBlackGrid:
					{
						m_quadPrimitiveBackground = new ion::render::Quad(ion::render::Quad::Axis::xy, ion::Vector2((float)ion::engine.render.window->GetClientAreaWidth() / 2.0f, (float)ion::engine.render.window->GetClientAreaHeight() / 2.0f));
						m_textureBackground = ion::engine.io.resourceManager->GetResource<ion::render::Texture>("emu_bg_black_grid.png");
						m_materialBackground = new ion::render::Material();
						m_materialBackground->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));
						m_materialBackground->AddDiffuseMap(m_textureBackground);
						m_textureBackground->SetWrapping(ion::render::Texture::Wrapping::Repeat);
						m_textureBackground->SetMinifyFilter(ion::render::Texture::Filter::Nearest);
						m_textureBackground->SetMagnifyFilter(ion::render::Texture::Filter::Nearest);

#if defined ION_RENDERER_SHADER
						m_materialBackground->SetShader(m_shaderFlatTextured);
#endif

						float coordx = (float)ion::engine.render.window->GetClientAreaWidth() / (float)m_textureBackground->GetWidth();
						float coordy = (float)ion::engine.render.window->GetClientAreaHeight() / (float)m_textureBackground->GetHeight();

						ion::render::TexCoord texCoords[4] =
						{
							ion::Vector2(0.0f, 0.0f),
							ion::Vector2(0.0f, coordy),
							ion::Vector2(coordx, coordy),
							ion::Vector2(coordx, 0.0f)
						};

						m_quadPrimitiveBackground->SetTexCoords(texCoords);

						break;
					}

					case VideoBorder::ImageBlueBorder:
					{
						m_quadPrimitiveBackground = new ion::render::Quad(ion::render::Quad::Axis::xy, ion::Vector2((float)ion::engine.render.window->GetClientAreaWidth() / 2.0f, (float)ion::engine.render.window->GetClientAreaHeight() / 2.0f));
						m_textureBackground = ion::engine.io.resourceManager->GetResource<ion::render::Texture>("emu_bg_blue_border.png");
						m_materialBackground = new ion::render::Material();
						m_materialBackground->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));
						m_materialBackground->AddDiffuseMap(m_textureBackground);
						m_textureBackground->SetWrapping(ion::render::Texture::Wrapping::Repeat);
						m_textureBackground->SetMinifyFilter(ion::render::Texture::Filter::Nearest);
						m_textureBackground->SetMagnifyFilter(ion::render::Texture::Filter::Nearest);

#if defined ION_RENDERER_SHADER
						m_materialBackground->SetShader(m_shaderFlatTextured);
#endif

						float coordx = (float)ion::engine.render.window->GetClientAreaWidth() / (float)m_textureBackground->GetWidth();
						float coordy = (float)ion::engine.render.window->GetClientAreaHeight() / (float)m_textureBackground->GetHeight();

						ion::render::TexCoord texCoords[4] =
						{
							ion::Vector2(0.0f, 0.0f),
							ion::Vector2(0.0f, coordy),
							ion::Vector2(coordx, coordy),
							ion::Vector2(coordx, 0.0f)
						};

						m_quadPrimitiveBackground->SetTexCoords(texCoords);

						break;
					}

					default:
						break;
				};
			}
		}
		else
		{
			//Failed to set video mode, revert to default size, windowed
			ion::engine.render.window->SetFullscreen(false, m_settings.displayIdx);
			ion::engine.render.window->Resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, true);
		}

		//Apply vsync
		ion::engine.render.renderer->EnableVSync(m_settings.vsync);
	}

	//Notify game state
	if (m_stateGame)
	{
		m_stateGame->ApplySettings();
	}

#if EVIL_EMU_STORE_SETTINGS
	SaveSettings();
#endif
}
