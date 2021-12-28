////////////////////////////////////////////////////////////////////////////////
// EvilEmu
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#include <ion/core/Platform.h>
#include <ion/core/io/File.h>
#include <ion/core/io/FileSystem.h>
#include <ion/renderer/Renderer.h>
#include <ion/renderer/Camera.h>
#include <ion/input/Keyboard.h>
#include <ion/input/Mouse.h>
#include <ion/input/Gamepad.h>
#include <ion/engine/Engine.h>

#include "StateGame.h"
#include "constants.h"
#include "evil_emu.h"

#include "megaex/memory.h"
#include "megaex/vdp/vdp.h"
#include "megaex/mgaudio.h"

#if EVIL_EMU_ROM_SOURCE==EVIL_EMU_ROM_SOURCE_EMBEDDED
#include "roms/include_rom.h"
#endif

const ion::render::TexCoord StateGame::s_texCoordsGame[4] =
{
	ion::Vector2(0.0f, 0.0f),
	ion::Vector2(0.0f, 1.0f),
	ion::Vector2(1.0f, 1.0f),
	ion::Vector2(1.0f, 0.0f)
};

const ion::render::TexCoord StateGame::s_texCoordsDebugger[4] =
{
	ion::Vector2(0.0f, 0.0f),
	ion::Vector2(0.0f, 1.0f),
	ion::Vector2(1.0f, 1.0f),
	ion::Vector2(1.0f, 0.0f)
};

StateGame::StateGame(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, Settings& settings, SaveManager& saveManager, const ion::Vector2i& windowSize, const ion::Vector2i& emulatorSize, ion::render::Window& window, StatePause& pauseState)
	: ion::gamekit::State("gameplay", stateManager, resourceManager)
#if EVIL_EMU_USE_SAVES
	, m_saveManager(saveManager)
#endif
	, m_window(window)
	, m_windowSize(windowSize)
	, m_emulatorSize(emulatorSize)
#if defined ION_PLATFORM_DESKTOP
	, m_pauseState(pauseState)
#endif
	, m_settings(settings)
{
	m_gui = NULL;

#if EMU_USE_INPUT_CALLBACKS
	m_keyboard = NULL;
#endif

	for (int i = 0; i < EVIL_EMU_NUM_RENDER_BUFFERS; i++)
	{
		m_renderTextures[i].Clear();
		m_renderMaterials[i] = NULL;
	}

#if EVIL_EMU_USE_2X_BUFFER
	m_pixelScaleBuffer = NULL;
#endif

#if EVIL_EMU_USE_SCANLINES
	m_scanlineTexture.Clear();
	m_scanlineMaterial = NULL;
#endif

	m_quadPrimitiveEmu = NULL;
	m_currentBufferIdx = 0;

	m_emulatorThread = NULL;
	m_emulatorThreadRunning = false;
	m_prevEmulatorState = eState_Running;
	m_emuStartTimer = EVIL_EMU_START_TIMER;

#if defined ION_SERVICES
	m_loginSaveQueryState = LoginSaveQueryState::Idle;
	m_currentUser = NULL;
#endif

#if EVIL_EMU_USE_SAVES && EVIL_EMU_MULTIPLE_SAVESLOTS
	m_saveSlotsMenu = NULL;
#endif

#if EMU_USE_INPUT_CALLBACKS
	m_lastInputRequestTime = 0;
	m_windowHasFocus = false;
#endif
}

void StateGame::OnEnterState()
{
	SetupRendering();

	ion::platform::RegisterCallbackSystemMenu(std::bind(&StateGame::OnSystemMenu, this, std::placeholders::_1, std::placeholders::_2));

#if defined ION_SERVICES
	//Catch user login events
	ion::engine.services.userManager->RegisterCallbackLogout(std::bind(&StateGame::OnUserLoggedOut, this, std::placeholders::_1));
#endif

#if defined ION_RENDERER_SHADER
	//Load shaders
	m_shaderFlatTextured = m_resourceManager.GetResource<ion::render::Shader>("flattextured");

	m_gui->SetShader(m_shaderFlatTextured);

	for (int i = 0; i < EVIL_EMU_NUM_RENDER_BUFFERS; i++)
		m_renderMaterials[i]->SetShader(m_shaderFlatTextured);
#endif

	//Initialise emulator
	m_quadPrimitiveEmu->SetTexCoords(s_texCoordsGame);

#if EVIL_EMU_ROM_SOURCE==EVIL_EMU_ROM_SOURCE_EMBEDDED
	if (!InitialiseEmulator(m68k_binary, m68k_binary_size))
#else
	if (!InitialiseEmulator(EVIL_EMU_ROM_FILENAME))
#endif
	{
		ion::debug::error << "Unable to initialise emulator" << ion::debug::end;
	}

	//Initialise gamepad callback
#if EMU_USE_INPUT_CALLBACKS
	Globals::getGamepadState = std::bind(&StateGame::OnInputRequest, this, std::placeholders::_1, std::placeholders::_2);
#endif

	//Create emulator thread
	m_emulatorThread = new EmulatorThread();

	//Create debugger UI
#if EVIL_EMU_INCLUDE_DEBUG_UI
	m_debuggerUI = new DebuggerUI(*m_gui, *m_emulatorThread, ion::Vector2i(20, 20), ion::Vector2i());
#if !defined ION_PLATFORM_SWITCH
	m_debuggerUI->RollUp(true);
#endif
	m_gui->AddWindow(*m_debuggerUI);
#endif

	//Start timer to allow display change to settle down
	m_emuStartTimer = EVIL_EMU_START_TIMER;
}

void StateGame::OnLeaveState()
{
	if (m_debuggerUI)
		delete m_debuggerUI;

	if (m_gui)
		delete m_gui;

#if EVIL_EMU_USE_2X_BUFFER
	if (m_pixelScaleBuffer)
		delete m_pixelScaleBuffer;
#endif

	if(m_quadPrimitiveEmu)
		delete m_quadPrimitiveEmu;

	for (int i = 0; i < EVIL_EMU_NUM_RENDER_BUFFERS; i++)
	{
		if (m_renderMaterials[i])
			delete m_renderMaterials[i];

		m_renderTextures[i].Clear();
	}
	

#if defined ION_RENDERER_SHADER
	m_shaderFlatTextured.Clear();
#endif
}

void StateGame::OnPauseState()
{
	m_emulatorThread->Pause();
}

void StateGame::OnResumeState()
{
	m_emulatorThread->Resume();
}

bool StateGame::Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, const std::vector<ion::input::Gamepad*> & gamepads)
{
#if defined ION_SERVICES
	//Update the login and save query sequence state machine
	UpdateLoginSaveQueryState();
#endif

	if (!m_emulatorThreadRunning)
	{
		m_emuStartTimer -= deltaTime;

		if (m_emuStartTimer < 0.0f)
		{
			//Run emulator
#if EMU_THREADED
			m_emulatorThread->Run();
			m_emulatorThread->SetPriority(ion::thread::Thread::Priority::High);

#if defined ION_PLATFORM_SWITCH
			m_emulatorThread->SetCoreAffinity(1 << EMU_THREAD_CORE_68K);
#endif
#endif
			m_emulatorThreadRunning = true;
		}
	}
	else
	{
#if EMU_USE_INPUT_CALLBACKS
		//Cache for use in input callback
		m_keyboard = keyboard;
		m_gamepads = gamepads;
		m_windowHasFocus = m_window.HasFocus();
#else
		//Update inputs
		for (int i = 0; i < EVIL_EMU_MAX_GAMEPADS; i++)
		{
			//Update input if window has focus
			u32 buttonState = 0;

			if (m_window.HasFocus())
			{
				//Update digital input
				for (int j = 0; j < eBtn_MAX; j++)
				{
					if (i == 0 && keyboard->KeyDown(m_settings.keyboardMap[j]))
					{
						buttonState |= g_emulatorButtonBits[j];
					}

					bool buttonDown = false;
					for (int k = 0; k < m_settings.gamepadMap[j].size(); k++)
					{
						buttonDown |= gamepads[i]->ButtonDown(m_settings.gamepadMap[j][k]);
					}

					if (buttonDown)
					{
						buttonState |= g_emulatorButtonBits[j];
					}
				}

				//Update analogue input
				if (gamepads[i]->GetLeftStick().x < -m_settings.analogueDeadzone)
					buttonState |= g_emulatorButtonBits[eBtn_Left];
				if (gamepads[i]->GetLeftStick().x > m_settings.analogueDeadzone)
					buttonState |= g_emulatorButtonBits[eBtn_Right];
				if (gamepads[i]->GetLeftStick().y > m_settings.analogueDeadzone)
					buttonState |= g_emulatorButtonBits[eBtn_Up];
				if (gamepads[i]->GetLeftStick().y < -m_settings.analogueDeadzone)
					buttonState |= g_emulatorButtonBits[eBtn_Down];
			}

			//Apply input
			EmulatorSetButtonState(i, buttonState);
		}
#endif

#if !EMU_THREADED
		m_emulatorThread->TickEmulator(deltaTime);
#endif

#if EVIL_EMU_USE_2X_BUFFER
		//Copy output to scalar buffer (scale x2 without filtering)
		u32 pixel = 0;
		const int bufferWidth = m_emulatorSize.x * 2;
		const int bufferHeight = m_emulatorSize.y * 2;
		
		for (int y = 0; y < m_emulatorSize.y; y++)
		{
			for (int x = 0; x < m_emulatorSize.x; x++)
			{
#if EVIL_EMU_PIXEL_ALIGN_TEST
				pixel = ((x & 1) && (y & 1)) ? 0xFF0000FF : 0xFF00FF00;
#else
				pixel = ((u32*)VDP_GetReadBuffer())[(y * m_emulatorSize.x) + x];
#endif

				m_pixelScaleBuffer[(((y * 2) + 0) * bufferWidth) + (x * 2) + 0] = pixel;
				m_pixelScaleBuffer[(((y * 2) + 0) * bufferWidth) + (x * 2) + 1] = pixel;
				m_pixelScaleBuffer[(((y * 2) + 1) * bufferWidth) + (x * 2) + 0] = pixel;
				m_pixelScaleBuffer[(((y * 2) + 1) * bufferWidth) + (x * 2) + 1] = pixel;
			}
		}
#else
#if EVIL_EMU_PIXEL_ALIGN_TEST
		u32 pixel = 0;
		for (int x = 0; x < m_emulatorSize.x; x++)
		{
			for (int y = 0; y < m_emulatorSize.y; y++)
			{
				pixel = ((x & 1) && (y & 1)) ? 0xFF0000FF : 0xFF00FF00;
				((u32*)VDP_GetReadBuffer())[(y * m_emulatorSize.x) + x] = pixel;
			}
		}
#endif
#endif

		//Update FPS display
		std::stringstream text;
		text.setf(std::ios::fixed, std::ios::floatfield);
		text.precision(2);

		text << "Audio clock: " << AudioGetClock()
			<< " :: 68000 clock: " << ((float)m_emulatorThread->Get68KCycle() / (float)CYCLES_PER_SECOND_68K)
			<< " :: Sample clock: " << ((float)AudioGetSamplesWritten() / (float)AUDIO_SAMPLE_RATE_HZ)
			<< " :: Render frame: " << m_emulatorThread->m_fpsCounterRender.GetFrame()
			<< " :: 68000 FPS: " << m_emulatorThread->m_fpsCounterRender.GetLastFPS()
			<< " :: Window FPS: " << m_fpsCounter.GetLastFPS()
			<< " :: Queued buffers: " << AudioGetBuffersQueued();
		m_window.SetTitle(text.str().c_str());

#if EVIL_EMU_USE_DATA_BRIDGE
		m_dataBridge.Update();
#endif

#if EMU_ENABLE_68K_DEBUGGER
		UpdateDebugger(*keyboard);
#endif
	}

#if defined ION_PLATFORM_DESKTOP
	if (m_window.HasFocus())
	{
		if (keyboard->KeyPressedThisFrame(ion::input::Keycode::ESCAPE) || gamepads[0]->ButtonPressedThisFrame(ion::input::GamepadButtons::SELECT))
		{
			m_stateManager.PushState(m_pauseState);
		}
	}
#endif

#if !defined ION_BUILD_MASTER
	m_gui->Update(deltaTime, keyboard, mouse, gamepads[0]);
#endif

	return true;
}

void StateGame::Render(ion::render::Renderer& renderer, const ion::render::Camera& camera, ion::render::Viewport& viewport)
{
	ion::Matrix4 emuMatrix;
	emuMatrix.SetTranslation(ion::Vector3(0.0f, 0.0f, 1.0f));

	bool readVDP = (m_emulatorThreadRunning && (!m_emulatorThread->IsPaused() || DEB_GetDebugMode() != DebugMode::Off));

	if (readVDP)
	{
#if EMU_ENABLE_68K_DEBUGGER
		if (DEB_GetDebugMode() != DebugMode::Off)
		{
			//Debugger draws direct to VDP framebuffer
			DisplayDebugger();
		}
#endif

#if EVIL_EMU_USE_2X_BUFFER
		//Copy scale buffer to render texture (scaled to target resolution with filtering)
		m_renderTextures[m_currentBufferIdx]->SetPixels(ion::render::Texture::Format::BGRA, false, (u8*)m_pixelScaleBuffer);
#else
		m_renderTextures[m_currentBufferIdx]->SetPixels(ion::render::Texture::Format::BGRA, false, VDP_ReadLock());
#endif

#if defined ION_RENDERER_SHADER
		//Bind material and draw quad
		if (m_renderMaterials[m_currentBufferIdx]->GetShader())
#endif
		{
			renderer.BindMaterial(*m_renderMaterials[m_currentBufferIdx], emuMatrix, camera.GetTransform().GetInverse(), renderer.GetProjectionMatrix());
			renderer.SetAlphaBlending(ion::render::Renderer::AlphaBlendType::Translucent);
			renderer.DrawVertexBuffer(m_quadPrimitiveEmu->GetVertexBuffer(), m_quadPrimitiveEmu->GetIndexBuffer());
			renderer.SetAlphaBlending(ion::render::Renderer::AlphaBlendType::None);
			renderer.UnbindMaterial(*m_renderMaterials[m_currentBufferIdx]);
		}

#if !EVIL_EMU_USE_2X_BUFFER
		VDP_ReadUnlock();
#endif
	}

	//Next buffer
	m_currentBufferIdx = (m_currentBufferIdx + 1) % EVIL_EMU_NUM_RENDER_BUFFERS;

#if EVIL_EMU_USE_SCANLINES

	if (m_settings.scanlineAlpha > 0.0f)
	{
#if defined ION_RENDERER_SHADER
		if (m_scanlineMaterial->GetShader())
#endif
		{
			//Draw scanlines
			emuMatrix.SetTranslation(ion::Vector3(0.0f, 0.0f, 1.1f));
			renderer.BindMaterial(*m_scanlineMaterial, emuMatrix, camera.GetTransform().GetInverse(), renderer.GetProjectionMatrix());
			renderer.SetAlphaBlending(ion::render::Renderer::AlphaBlendType::Translucent);
			renderer.DrawVertexBuffer(m_quadPrimitiveEmu->GetVertexBuffer(), m_quadPrimitiveEmu->GetIndexBuffer());
			renderer.SetAlphaBlending(ion::render::Renderer::AlphaBlendType::None);
			renderer.UnbindMaterial(*m_scanlineMaterial);
		}
	}
#endif

#if !defined ION_BUILD_MASTER
	m_gui->Render(renderer, viewport);
#endif

	m_fpsCounter.Update();
}

void StateGame::SetupRendering()
{
	ion::Vector2i renderBufferSize = m_emulatorSize;

#if EVIL_EMU_USE_2X_BUFFER || VDP_SCALE_2X
	renderBufferSize = renderBufferSize * 2;
#endif

#if EVIL_EMU_USE_SCANLINES
	if (!m_scanlineTexture)
	{
		m_scanlineTexture = ion::render::Texture::Create(renderBufferSize.x, renderBufferSize.y, ion::render::Texture::Format::BGRA, ion::render::Texture::Format::BGRA, ion::render::Texture::BitsPerPixel::BPP24, false, false, NULL);
		m_scanlineTexture->SetMinifyFilter(ion::render::Texture::Filter::Linear);
		m_scanlineTexture->SetMagnifyFilter(ion::render::Texture::Filter::Linear);
		m_scanlineTexture->SetWrapping(ion::render::Texture::Wrapping::Clamp);
	}

	if (!m_scanlineMaterial)
	{
		m_scanlineMaterial = new ion::render::Material();
		m_scanlineMaterial->AddDiffuseMap(m_scanlineTexture);
		m_scanlineMaterial->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));
	}
#endif

#if EVIL_EMU_USE_2X_BUFFER
	if (!m_pixelScaleBuffer)
	{
		m_pixelScaleBuffer = new u32[(m_emulatorSize.x * 2) * (m_emulatorSize.y * 2)];
		ion::memory::MemSet(m_pixelScaleBuffer, 0, (m_emulatorSize.x * 2) * (m_emulatorSize.y * 2));
	}
#endif

	//Recreate textures
	for (int i = 0; i < EVIL_EMU_NUM_RENDER_BUFFERS; i++)
	{
		m_renderTextures[i].Clear();

		m_renderTextures[i] = ion::render::Texture::Create(renderBufferSize.x, renderBufferSize.y, ion::render::Texture::Format::BGRA, ion::render::Texture::Format::BGRA, ion::render::Texture::BitsPerPixel::BPP24, false, m_settings.pixelBuffer, NULL);

		//Setup materials
		m_renderMaterials[i] = new ion::render::Material();
		m_renderMaterials[i]->AddDiffuseMap(m_renderTextures[i]);
		m_renderMaterials[i]->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));

		//Setup texture filtering
		m_renderTextures[i]->SetMinifyFilter(ion::render::Texture::Filter::Linear);
		m_renderTextures[i]->SetMagnifyFilter(ion::render::Texture::Filter::Linear);
		m_renderTextures[i]->SetWrapping(ion::render::Texture::Wrapping::Clamp);
	}

	//Recreate quad
	if (m_quadPrimitiveEmu)
		delete m_quadPrimitiveEmu;

	m_quadPrimitiveEmu = new ion::render::Quad(ion::render::Quad::Axis::xy, ion::Vector2(((float)m_window.GetClientAreaHeight() * DEFAULT_WINDOW_RATIO) / 2.0f, (float)m_window.GetClientAreaHeight() / 2.0f));
	m_quadPrimitiveEmu->SetTexCoords(s_texCoordsGame);

	//Redraw scanlines
#if EVIL_EMU_USE_SCANLINES
	ion::ColourRGB colour = EVIL_EMU_SCANLINE_DEFAULT_COLOUR;
	DrawScanlineTexture(ion::Colour(colour.r, colour.g, colour.b, m_settings.scanlineAlpha));
#endif

	m_windowSize = ion::Vector2i(m_window.GetClientAreaWidth(), m_window.GetClientAreaHeight());

	//Create or resize GUI
	if(m_gui)
		m_gui->SetSize(m_windowSize);
	else
		m_gui = new ion::gui::GUI(m_windowSize);

	//Reload shaders
#if defined ION_RENDERER_SHADER
	m_shaderFlatTextured = m_resourceManager.GetResource<ion::render::Shader>("flattextured");

	m_gui->SetShader(m_shaderFlatTextured);

	for (int i = 0; i < EVIL_EMU_NUM_RENDER_BUFFERS; i++)
		m_renderMaterials[i]->SetShader(m_shaderFlatTextured);
#endif
}

void StateGame::ApplySettings()
{
	SetupRendering();
}

void StateGame::OnSystemMenu(ion::platform::SystemMenu, bool opened)
{
	if (opened)
		m_emulatorThread->Pause();
	else
		m_emulatorThread->Resume();
}

#if EVIL_EMU_USE_SCANLINES
void StateGame::DrawScanlineTexture(const ion::Colour& colour)
{
	int textureWidth = m_scanlineTexture->GetWidth();
	int textureHeight = m_scanlineTexture->GetHeight();

	u32* tempBuffer = new u32[textureWidth * textureHeight];
	ion::memory::MemSet(tempBuffer, 0, textureWidth * textureHeight * sizeof(u32));

	u32 colourBGRA = colour.AsARGB();

	for (int y = 0; y < textureHeight - 2; y += 2)
	{
		for (int x = 0; x < textureWidth; x++)
		{
			tempBuffer[(y * textureWidth) + x] = colourBGRA;
		}
	}

	m_scanlineTexture->SetPixels(ion::render::Texture::Format::BGRA, false, (u8*)tempBuffer);

	delete [] tempBuffer;
}
#endif

#if EMU_USE_INPUT_CALLBACKS
U16 StateGame::OnInputRequest(int gamepadIdx, bool latch6button)
{
	//Update input if timed out
	u64 currentTime = ion::time::GetSystemTicks();
	if (ion::time::TicksToSeconds(currentTime - m_lastInputRequestTime) > ((double)EMU_INPUT_UPDATE_MS / 1000))
	{
		m_lastInputRequestTime = currentTime;

		if (m_keyboard && gamepadIdx == 0)
		{
			m_keyboard->Update();
		}

		if (m_gamepads.size() > gamepadIdx && m_gamepads[gamepadIdx])
		{
			m_gamepads[gamepadIdx]->Poll();
		}
	}

	//Respond with input if window has focus
	u32 buttonState = 0;

	if (m_windowHasFocus)
	{
		//Update digital input
		for (int i = 0; i < eBtn_MAX; i++)
		{
			if (gamepadIdx == 0 && m_keyboard && m_keyboard->KeyDown(m_settings.keyboardMap[i]))
			{
				buttonState |= (latch6button ? (g_emulatorButtonBits[i] >> 16) : (g_emulatorButtonBits[i] & 0xFFFF));
			}

			if (m_gamepads.size() > gamepadIdx && m_gamepads[gamepadIdx])
			{
				bool buttonDown = false;
				for (int j = 0; j < m_settings.gamepadMap[i].size(); j++)
				{
					buttonDown |= m_gamepads[gamepadIdx]->ButtonDown(m_settings.gamepadMap[i][j]);
				}

				if (buttonDown)
				{
					buttonState |= (latch6button ? (g_emulatorButtonBits[i] >> 16) : (g_emulatorButtonBits[i] & 0xFFFF));
				}
			}
		}

		//Update analogue input
		if (m_gamepads.size() > gamepadIdx && m_gamepads[gamepadIdx])
		{
			if (m_gamepads[gamepadIdx]->GetLeftStick().x < -m_settings.analogueDeadzone)
				buttonState |= (latch6button ? (g_emulatorButtonBits[eBtn_Left] >> 16) : (g_emulatorButtonBits[eBtn_Left] & 0xFFFF));
			if (m_gamepads[gamepadIdx]->GetLeftStick().x > m_settings.analogueDeadzone)
				buttonState |= (latch6button ? (g_emulatorButtonBits[eBtn_Right] >> 16) : (g_emulatorButtonBits[eBtn_Right] & 0xFFFF));
			if (m_gamepads[gamepadIdx]->GetLeftStick().y > m_settings.analogueDeadzone)
				buttonState |= (latch6button ? (g_emulatorButtonBits[eBtn_Up] >> 16) : (g_emulatorButtonBits[eBtn_Up] & 0xFFFF));
			if (m_gamepads[gamepadIdx]->GetLeftStick().y < -m_settings.analogueDeadzone)
				buttonState |= (latch6button ? (g_emulatorButtonBits[eBtn_Down] >> 16) : (g_emulatorButtonBits[eBtn_Down] & 0xFFFF));
		}
	}

	//With thanks to byuu for this tip <3
	return buttonState;
}
#endif

#if defined ION_SERVICES
void StateGame::QueryLoginSaves()
{
	//Start the sequence
	m_loginSaveQueryState = LoginSaveQueryState::LoggingIn;

	//Log in
	if (!m_currentUser)
	{
		ion::engine.services.userManager->RequestLogin(std::bind(&StateGame::OnUserLoggedIn, this, std::placeholders::_1, std::placeholders::_2));
	}
}

void StateGame::UpdateLoginSaveQueryState()
{
	switch (m_loginSaveQueryState)
	{
	case LoginSaveQueryState::LoggingIn:
	{
		if (m_currentUser)
		{
#if EVIL_EMU_USE_SAVES
			//Begin async save initialise
			m_loginSaveQueryState = LoginSaveQueryState::SaveInitialising;

			m_saveManager.InitSave(*m_currentUser,
				[this](bool success)
			{
				//Begin async save query
				m_loginSaveQueryState = LoginSaveQueryState::SaveQuerying;
				LoadSavesAsync();
			});
#else
#endif
		}

		break;
	}

	case LoginSaveQueryState::SaveQuerying:
		break;

	case LoginSaveQueryState::SaveLoaded:
	{
#if EVIL_EMU_USE_SAVES
		//Inject available state
		OnSaveDataLoaded();

		m_loginSaveQueryState = LoginSaveQueryState::SaveInjected;
#endif

		break;
	}

	case LoginSaveQueryState::Idle:
	case LoginSaveQueryState::SaveInitialising:
	case LoginSaveQueryState::SaveInjected:
	case LoginSaveQueryState::Error:
	default:
		break;
	};
}

void StateGame::OnUserLoggedIn(ion::services::UserManager::LoginResult result, ion::services::User* user)
{
	m_currentUser = user;
}

void StateGame::OnUserLoggedOut(ion::services::User& user)
{
	if (&user == m_currentUser)
	{
		m_currentUser = nullptr;
	}
}
#endif

#if EVIL_EMU_USE_SAVES
bool StateGame::SavesAvailable()
{
#if EVIL_EMU_MULTIPLE_SAVESLOTS
	//Backwards compatibility with fixed slots - check slot 0 has valid password
	return (m_save.m_saveSlots.size() > 0) && (m_save.m_saveSlots[0].password != 0);
#else
	return m_save.m_saveSlots.size() > 0;
#endif
}

void StateGame::LoadSavesAsync()
{
	ion::debug::Assert(m_currentUser, "StateGame::LoadSavesAsync() - No current user");

	//Async load
	m_saveManager.LoadGame(*m_currentUser,
		[this](Save& saveData, bool success)
		{
			if (success)
			{
#if EVIL_EMU_MULTIPLE_SAVESLOTS
				//Backwards compatibility - remove saves with invalid passwords
				for (int i = 0; i < saveData.m_saveSlots.size();)
				{
					if (saveData.m_saveSlots[i].password == 0)
					{
						saveData.m_saveSlots.erase(m_save.m_saveSlots.begin() + i);
					}
					else
					{
						i++;
					}
				}
#endif
			}
			else
			{
				//Could not load, clear save slots
				saveData.m_saveSlots.clear();
			}

			m_save = saveData;
			m_loginSaveQueryState = LoginSaveQueryState::SaveLoaded;
		});
}
#endif