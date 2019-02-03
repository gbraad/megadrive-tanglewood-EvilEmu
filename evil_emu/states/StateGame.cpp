////////////////////////////////////////////////////////////////////////////////
// EvilEmu
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#include <ion/renderer/Renderer.h>
#include <ion/renderer/Camera.h>
#include <ion/input/Keyboard.h>
#include <ion/input/Mouse.h>
#include <ion/input/Gamepad.h>
#include <ion/io/File.h>
#include <ion/io/FileSystem.h>

#include "StateGame.h"
#include "constants.h"
#include "evil_emu.h"

#include "megaex/memory.h"
#include "megaex/vdp/vdp.h"

#include "roms/include_rom.h"

static const int g_top = 128;
static const int g_bottom = 128 + 224;
static const int g_left = 128;
static const int g_right = 128 + (40 * 8);
static const float g_borderTop = (1.0f / HEIGHT) * (float)g_top;
static const float g_borderBottom = (1.0f / HEIGHT) * (float)(HEIGHT - g_bottom);
static const float g_borderLeft = (1.0f / WIDTH) * (float)g_left;
static const float g_borderRight = (1.0f / WIDTH) * (float)(WIDTH - g_right);

const ion::render::TexCoord StateGame::s_texCoordsGame[4] =
{
	ion::Vector2(g_borderLeft, g_borderTop),
	ion::Vector2(g_borderLeft, 1.0f - g_borderBottom),
	ion::Vector2(1.0f - g_borderRight, 1.0f - g_borderBottom),
	ion::Vector2(1.0f - g_borderRight, g_borderTop)
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
	, m_saveManager(saveManager)
	, m_window(window)
	, m_windowSize(windowSize)
	, m_emulatorSize(emulatorSize)
	, m_pauseState(pauseState)
	, m_settings(settings)
{
	for (int i = 0; i < EMU_NUM_RENDER_BUFFERS; i++)
	{
		m_renderTextures[i] = NULL;
		m_renderMaterials[i] = NULL;
	}

#if EMU_USE_2X_BUFFER
	m_pixelScaleBuffer = NULL;
#endif

#if EMU_USE_SCANLINES
	m_scanlineTexture = NULL;
	m_scanlineMaterial = NULL;
#endif

	m_quadPrimitiveEmu = NULL;
	m_currentBufferIdx = 0;

#if defined ION_RENDERER_SHADER
	m_vertexShader = NULL;
	m_pixelShader = NULL;
#endif

	m_emulatorThread = NULL;
	m_emulatorThreadRunning = false;
	m_prevEmulatorState = eState_Running;
	m_emuStartTimer = EMU_START_TIMER;

	m_saveSlotsMenu = NULL;
}

void StateGame::OnEnterState()
{
	ion::Vector2i renderBufferSize = m_emulatorSize;

#if EMU_USE_2X_BUFFER || VDP_SCALE_2X
	renderBufferSize = renderBufferSize * 2;
#endif

#if EMU_USE_SCANLINES
	m_scanlineTexture = ion::render::Texture::Create(renderBufferSize.x, renderBufferSize.y, ion::render::Texture::Format::BGRA, ion::render::Texture::Format::BGRA, ion::render::Texture::BitsPerPixel::BPP24, false, false, NULL);

	m_scanlineMaterial = new ion::render::Material();
	m_scanlineMaterial->AddDiffuseMap(m_scanlineTexture);
	m_scanlineMaterial->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));

	m_scanlineTexture->SetMinifyFilter(ion::render::Texture::Filter::Linear);
	m_scanlineTexture->SetMagnifyFilter(ion::render::Texture::Filter::Linear);
	m_scanlineTexture->SetWrapping(ion::render::Texture::Wrapping::Clamp);
#endif

#if EMU_USE_2X_BUFFER
	m_pixelScaleBuffer = new u32[(m_emulatorSize.x * 2) * (m_emulatorSize.y * 2)];
	ion::memory::MemSet(m_pixelScaleBuffer, 0, (m_emulatorSize.x * 2) * (m_emulatorSize.y * 2));
#endif

	m_quadPrimitiveEmu = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2(((float)m_window.GetClientAreaHeight() * DEFAULT_SCREEN_RATIO) / 2.0f, (float)m_window.GetClientAreaHeight() / 2.0f));

#if defined ION_RENDERER_SHADER
	//Create shaders
	m_vertexShader = ion::render::Shader::Create();
	m_pixelShader = ion::render::Shader::Create();

	//Load shaders
	if(!m_vertexShader->Load("shaders/flattextured_v.ion.shader"))
	{
		ion::debug::Error("Failed to load vertex shader\n");
	}

	if(!m_pixelShader->Load("shaders/flattextured_p.ion.shader"))
	{
		ion::debug::Error("Failed to load pixel shader\n");
	}

	m_materialEmu->SetVertexShader(m_vertexShader);
	m_materialEmu->SetPixelShader(m_pixelShader);
#endif

	//Initialise emulator
	m_quadPrimitiveEmu->SetTexCoords(s_texCoordsGame);

#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_ROMFILE
	if (!InitialiseEmulator(EVIL_EMU_ROM_FILENAME))
	{
		ion::debug::error << "Unable to initialise emulator" << ion::debug::end;
	}
#else
	if (!InitialiseEmulator(snasm68k_binary, snasm68k_binary_size))
	{
		ion::debug::error << "Unable to initialise emulator" << ion::debug::end;
	}
#endif

	//Initialise watches
#if EVIL_EMU_USE_SAVES
	m_memWatcher.AddAddress(snasm68k_symbol_EmuTrap_SaveGame_val, 2, std::bind(&StateGame::OnSaveGame, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_2));
	m_memWatcher.AddAddress(snasm68k_symbol_EmuTrap_GetSaveAvailable_val, 1, std::bind(&StateGame::OnGetSaveAvailable, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_2));
	m_memWatcher.AddAddress(snasm68k_symbol_EmuTrap_GetSaveData_val, 1, std::bind(&StateGame::OnGetSaveData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_2));
#endif

	//Initialise save slots
	InitSaves();

#if EVIL_EMU_ACHEVEMENTS
	//Initialise achievements
	m_achievements.RegisterWatchers(m_memWatcher);
#endif

	//Create emulator thread
	m_emulatorThread = new EmulatorThread();

	//Create GUI
	m_gui = new ion::gui::GUI(m_windowSize);

	//Create debugger UI
#if EMU_INCLUDE_DEBUGGER
	m_debuggerUI = new DebuggerUI(*m_gui, *m_emulatorThread, ion::Vector2i(20, 20), ion::Vector2i());
	m_debuggerUI->RollUp(true);
	m_gui->AddWindow(*m_debuggerUI);
#endif

	//Start timer to allow display change to settle down
	m_emuStartTimer = EMU_START_TIMER;
}

void StateGame::OnLeaveState()
{
	if (m_debuggerUI)
		delete m_debuggerUI;

	if (m_gui)
		delete m_gui;

#if EMU_USE_2X_BUFFER
	if (m_pixelScaleBuffer)
		delete m_pixelScaleBuffer;
#endif

	if(m_quadPrimitiveEmu)
		delete m_quadPrimitiveEmu;

	for (int i = 0; i < EMU_NUM_RENDER_BUFFERS; i++)
	{
		if (m_renderMaterials[i])
			delete m_renderMaterials[i];

		if (m_renderTextures[i])
			delete m_renderTextures[i];
	}
	

#if defined ION_RENDERER_SHADER
	if(m_pixelShader)
		delete m_pixelShader;

	if(m_vertexShader)
		delete m_vertexShader;
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

bool StateGame::Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, ion::input::Gamepad* gamepad)
{
	if (!m_emulatorThreadRunning)
	{
		m_emuStartTimer -= deltaTime;

		if (m_emuStartTimer < 0.0f)
		{
			//Run emulator
#if EMU_THREADED
			m_emulatorThread->Run();
			m_emulatorThread->SetPriority(ion::thread::Thread::Priority::High);
#endif
			m_emulatorThreadRunning = true;
		}
	}
	else
	{
#if !EMU_THREADED
		m_emulatorThread->TickEmulator(deltaTime);
#endif
		m_emulatorThread->m_renderCritSec.Begin();

#if EMU_USE_2X_BUFFER
		//Copy output to scalar buffer (scale x2 without filtering)
		u32 pixel = 0;
		const int bufferWidth = m_emulatorSize.x * 2;
		const int bufferHeight = m_emulatorSize.y * 2;
		
		for (int y = 0; y < m_emulatorSize.y; y++)
		{
			for (int x = 0; x < m_emulatorSize.x; x++)
			{
#if EMU_PIXEL_ALIGN_TEST
				pixel = ((x & 1) && (y & 1)) ? 0xFF0000FF : 0xFF00FF00;
#else
				pixel = ((u32*)VDP::videoMemory)[(y * m_emulatorSize.x) + x];
#endif

				m_pixelScaleBuffer[(((y * 2) + 0) * bufferWidth) + (x * 2) + 0] = pixel;
				m_pixelScaleBuffer[(((y * 2) + 0) * bufferWidth) + (x * 2) + 1] = pixel;
				m_pixelScaleBuffer[(((y * 2) + 1) * bufferWidth) + (x * 2) + 0] = pixel;
				m_pixelScaleBuffer[(((y * 2) + 1) * bufferWidth) + (x * 2) + 1] = pixel;
			}
		}
#else
#if EMU_PIXEL_ALIGN_TEST
		u32 pixel = 0;
		for (int x = 0; x < m_emulatorSize.x; x++)
		{
			for (int y = 0; y < m_emulatorSize.y; y++)
			{
				pixel = ((x & 1) && (y & 1)) ? 0xFF0000FF : 0xFF00FF00;
				((u32*)VDP::videoMemory)[(y * m_emulatorSize.x) + x] = pixel;
			}
		}
#endif
#endif

		//Update FPS display
		if (m_frameCount++ % 30 == 0)
		{
			//Set window title
			std::stringstream text;
			text.setf(std::ios::fixed, std::ios::floatfield);
			text.precision(2);
			text << "Window FPS: " << m_fpsCounter.GetLastFPS()
				<< " :: 68000 FPS: " << m_emulatorThread->m_fpsCounterEmulator.GetLastFPS()
				<< " :: Render FPS: " << m_emulatorThread->m_fpsCounterRender.GetLastFPS()
				<< " :: Audio FPS: " << m_emulatorThread->m_fpsCounterAudio.GetLastFPS();
			m_window.SetTitle(text.str().c_str());
		}

		//Update input if window has focus
		u16 buttonState = 0;

		if (m_window.HasFocus())
		{
			//Update digital input
			for (int i = 0; i < eBtn_MAX; i++)
			{
				if (keyboard->KeyDown(m_settings.keyboardMap[i]) || gamepad->ButtonDown(m_settings.gamepadMap[i]))
				{
					buttonState |= g_emulatorButtonBits[i];
				}
			}

			//Update analogue input
			if (gamepad->GetLeftStick().x < 0.0f)
				buttonState |= g_emulatorButtonBits[eBtn_Left];
			if (gamepad->GetLeftStick().x > 0.0f)
				buttonState |= g_emulatorButtonBits[eBtn_Right];
			if (gamepad->GetLeftStick().y > 0.0f)
				buttonState |= g_emulatorButtonBits[eBtn_Up];
			if (gamepad->GetLeftStick().y < 0.0f)
				buttonState |= g_emulatorButtonBits[eBtn_Down];
		}

		//Apply input
		EmulatorSetButtonState(buttonState);

		m_emulatorThread->m_renderCritSec.End();

		m_memWatcher.Update();
	}

	if (m_window.HasFocus())
	{
		if (keyboard->KeyPressedThisFrame(ion::input::Keycode::ESCAPE) || gamepad->ButtonPressedThisFrame(ion::input::GamepadButtons::SELECT))
		{
			m_stateManager.PushState(m_pauseState);
		}
	}

	m_gui->Update(deltaTime, keyboard, mouse, gamepad);

#if EVIL_EMU_ACHEVEMENTS
	m_achievements.Update();
#endif

	return true;
}

void StateGame::Render(ion::render::Renderer& renderer, const ion::render::Camera& camera, ion::render::Viewport& viewport)
{
#if EMU_USE_2X_BUFFER
	//Copy scale buffer to render texture (scaled to target resolution with filtering)
	m_renderTextures[m_currentBufferIdx]->SetPixels(ion::render::Texture::eBGRA, false, (u8*)m_pixelScaleBuffer);
#else
	m_emulatorThread->m_renderCritSec.Begin();
	m_renderTextures[m_currentBufferIdx]->SetPixels(ion::render::Texture::Format::BGRA, false, VDP::videoMemory);
	m_emulatorThread->m_renderCritSec.End();
#endif

	//Bind material and draw quad
	ion::Matrix4 emuMatrix;
	emuMatrix.SetTranslation(ion::Vector3(0.0f, 0.0f, 1.0f));
	m_renderMaterials[m_currentBufferIdx]->Bind(emuMatrix, camera.GetTransform().GetInverse(), renderer.GetProjectionMatrix());
	renderer.SetAlphaBlending(ion::render::Renderer::eNoBlend);
	renderer.DrawVertexBuffer(m_quadPrimitiveEmu->GetVertexBuffer(), m_quadPrimitiveEmu->GetIndexBuffer());
	m_renderMaterials[m_currentBufferIdx]->Unbind();

	//Next buffer
	m_currentBufferIdx = (m_currentBufferIdx + 1) % EMU_NUM_RENDER_BUFFERS;

#if EMU_USE_SCANLINES
	if (m_settings.scanlineAlpha > 0.0f)
	{
		//Draw scanlines
		emuMatrix.SetTranslation(ion::Vector3(0.0f, 0.0f, 1.1f));
		m_scanlineMaterial->Bind(emuMatrix, camera.GetTransform().GetInverse(), renderer.GetProjectionMatrix());
		renderer.SetAlphaBlending(ion::render::Renderer::eTranslucent);
		renderer.DrawVertexBuffer(m_quadPrimitiveEmu->GetVertexBuffer(), m_quadPrimitiveEmu->GetIndexBuffer());
		m_scanlineMaterial->Unbind();
	}
#endif

	m_gui->Render(renderer, viewport);

	m_fpsCounter.Update();
}

void StateGame::ApplySettings()
{
	ion::Vector2i renderBufferSize = m_emulatorSize;

#if EMU_USE_2X_BUFFER || VDP_SCALE_2X
	renderBufferSize = renderBufferSize * 2;
#endif

	//Recreate textures
	for (int i = 0; i < EMU_NUM_RENDER_BUFFERS; i++)
	{
		if (m_renderTextures[i])
		{
			delete m_renderTextures[i];
			m_renderTextures[i] = nullptr;
		}

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
	{
		delete m_quadPrimitiveEmu;
		m_quadPrimitiveEmu = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2(((float)m_window.GetClientAreaHeight() * DEFAULT_SCREEN_RATIO) / 2.0f, (float)m_window.GetClientAreaHeight() / 2.0f));
		m_quadPrimitiveEmu->SetTexCoords(s_texCoordsGame);
	}

	//Redraw scanlines
	ion::ColourRGB colour = EMU_SCANLINE_DEFAULT_COLOUR;
	DrawScanlineTexture(ion::Colour(colour.r, colour.b, colour.b, m_settings.scanlineAlpha));

	m_windowSize = ion::Vector2i(m_window.GetClientAreaWidth(), m_window.GetClientAreaHeight());

	//Resize GUI
	m_gui->SetSize(m_windowSize);
}

#if EMU_USE_SCANLINES
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

void StateGame::OnSaveGame(u32 watchAddress, int watchSize, u32 watchValue)
{
#if EVIL_EMU_USE_SAVES
	//If data saved with FINAL build, but we're running DEMO, don't overwrite data
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
	if (m_save.m_saveSlots.size() > 0)
	{
		if (m_save.m_saveSlots.back().gameType != EVIL_EMU_GAME_TYPE_DEMO)
		{
			return;
		}
	}
#endif

	//New save slot
	Save::SaveSlot saveSlot;

	saveSlot.serialiseData.clear();
	saveSlot.serialiseData.reserve(snasm68k_symbol_CheckpointSerialiseBlockSize_val);

#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL
	//Copy serialise block
	for (int i = 0; i < snasm68k_symbol_CheckpointSerialiseBlockSize_val; i++)
	{
		saveSlot.serialiseData.push_back(MEM_getByte(snasm68k_symbol_CheckpointSerialiseMemBlock_val + i));
	}

	//Get save version
	saveSlot.saveVersion = MEM_getWord(snasm68k_symbol_LastSaveVersion_val);
#endif

	//Get game type
	saveSlot.gameType = EVIL_EMU_GAME_TYPE;

	//Get password
	saveSlot.password = MEM_getLong(snasm68k_symbol_CurrentSavePassword_val);

	//Get firefly counts
	saveSlot.firefliesAct = MEM_getWord(snasm68k_symbol_FireflyPickupCountAct_val);
	saveSlot.firefliesGame = MEM_getWord(snasm68k_symbol_FireflyPickupCountTotalSave_val);

	//Get level address
	saveSlot.levelAddr = MEM_getLong(snasm68k_symbol_CurrentLevel_val);

	//Get level index
	saveSlot.levelIdx = MEM_getByte(saveSlot.levelAddr + snasm68k_symbol_Level_Index_val);

	//Get save version
	saveSlot.saveVersion = MEM_getWord(snasm68k_symbol_LastSaveVersion_val);

	//If slot differs from last
	if (m_save.m_saveSlots.size() == 0 || !(saveSlot == m_save.m_saveSlots.back()))
	{
		//Time stamp
		saveSlot.timeStamp = ion::time::GetLocalTime();

		//Add slot
		m_save.m_saveSlots.push_back(saveSlot);

		//Save to disk
		m_saveManager.SaveGame(m_save);
	}
#endif // EVIL_EMU_USE_SAVES
}

void StateGame::OnGetSaveAvailable(u32 watchAddress, int watchSize, u32 watchValue)
{
#if EVIL_EMU_USE_SAVES
	//Backwards compatibility with fixed slots - check slot 0 has valid password
	bool available = m_save.m_saveSlots.size() > 0 && m_save.m_saveSlots[0].password != 0;

		//If data saved with FINAL build, but we're running DEMO, data isn't compatible
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
	if (m_save.m_saveSlots.size() > 0 && m_save.m_saveSlots[0].gameType != EVIL_EMU_GAME_TYPE_DEMO)
	{
		available = false;
	}
#endif

	//Set save available
	MEM_setByte(snasm68k_symbol_EmuData_SaveAvailable_val, available ? 0x01 : 0x00);

	//Notify game if waiting
	MEM_setByte(snasm68k_symbol_EmuData_AwaitingResponse_val, 0x01);
#endif // EVIL_EMU_USE_SAVES
}

void StateGame::OnGetSaveData(u32 watchAddress, int watchSize, u32 watchValue)
{
#if EVIL_EMU_USE_SAVES
	if (m_save.m_saveSlots.size() > 1)
	{
		//Open save slot menu
		m_saveSlotsMenu = new MenuSaveSlots(*m_gui, /* font, */ m_window, m_save,
			[&](int index)
		{
			if (index >= 0)
			{
				//Apply save slot data
				ApplySaveData(index);

				//Notify game if waiting
				MEM_setByte(snasm68k_symbol_EmuData_AwaitingResponse_val, 0x01);
			}
			else
			{
				//Canceled
				MEM_setByte(snasm68k_symbol_EmuData_AwaitingResponse_val, 0x00);
			}

			//Destroy save slots menu
			m_gui->DeleteWindow(*m_saveSlotsMenu);
			m_saveSlotsMenu = nullptr;
		}
		);

		m_gui->AddWindow(*m_saveSlotsMenu);
	}
	else
	{
		//Apply save slot 0 data
		ApplySaveData(0);

		//Notify game if waiting
		MEM_setByte(snasm68k_symbol_EmuData_AwaitingResponse_val, 0x01);
	}
#endif // EVIL_EMU_USE_SAVES
}

void StateGame::InitSaves()
{
#if EVIL_EMU_USE_SAVES
	//Try loading game
	if (!m_saveManager.LoadGame(m_save))
	{
		//Could not load, clear save slots
		m_save.m_saveSlots.clear();
	}

	//Backwards compatibility - remove saves with invalid passwords
	for (int i = 0; i < m_save.m_saveSlots.size();)
	{
		if (m_save.m_saveSlots[i].password == 0)
		{
			m_save.m_saveSlots.erase(m_save.m_saveSlots.begin() + i);
		}
		else
		{
			i++;
		}
	}
#endif // EVIL_EMU_USE_SAVES
}

void StateGame::ApplySaveData(int slot)
{
#if EVIL_EMU_USE_SAVES
	//If data saved with FINAL build, but we're running DEMO, data isn't compatible
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
	if (m_save.m_saveSlots[slot].gameType != EVIL_EMU_GAME_TYPE_DEMO)
	{
		MEM_setByte(snasm68k_symbol_EmuData_SaveAvailable_val, 0x00);
		MEM_setLong(snasm68k_symbol_CurrentSavePassword_val, 0x00000000);
		MEM_setByte(snasm68k_symbol_EmuData_AwaitingResponse_val, 0x01);
		return;
	}
#endif

#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL
	//Inject serialise block into game memory
	if (snasm68k_symbol_CheckpointSerialiseBlockSize_val == m_save.m_saveSlots[slot].serialiseData.size())
	{
		for (int i = 0; i < snasm68k_symbol_CheckpointSerialiseBlockSize_val; i++)
		{
			MEM_setByte(snasm68k_symbol_CheckpointSerialiseMemBlock_val + i, m_save.m_saveSlots[slot].serialiseData[i]);
		}
	}

	//Set save version
	MEM_setWord(snasm68k_symbol_LastSaveVersion_val, m_save.m_saveSlots[slot].saveVersion);
#endif

	//Set password
	MEM_setLong(snasm68k_symbol_CurrentSavePassword_val, m_save.m_saveSlots[slot].password);

	//Set firefly counts
	MEM_setWord(snasm68k_symbol_FireflyPickupCountAct_val, m_save.m_saveSlots[slot].firefliesAct);
	MEM_setWord(snasm68k_symbol_FireflyPickupCountTotalSave_val, m_save.m_saveSlots[slot].firefliesGame);

	//Set level address (fetch from index - address may change between builds)
	u32 levelAddrEntry = snasm68k_symbol_LevelList_val + (m_save.m_saveSlots[slot].levelIdx * M68K_SIZE_LONG);
	u32 levelAddr = MEM_getLong(levelAddrEntry);
	MEM_setLong(snasm68k_symbol_CurrentLevel_val, levelAddr);
#endif // EVIL_EMU_USE_SAVES
}
