#include "GameStateTanglewood.h"
#include "roms/include_vars.h"
#include "megaex/vdp/vdp.h"

#include <ion/engine/Engine.h>

const std::map<ion::platform::Language, unsigned int> StateTanglewood::s_supportedLanguages =
{
#if EVIL_EMU_USE_DATA_BRIDGE && EVIL_EMU_LANGUAGE_SELECT
	{ ion::platform::Language::AmericanEnglish, m68k_symbol_LANGUAGE_ENGLISH_val },
	{ ion::platform::Language::BritishEnglish, m68k_symbol_LANGUAGE_ENGLISH_val },
	{ ion::platform::Language::Japanese, m68k_symbol_LANGUAGE_ENGLISH_val },
	{ ion::platform::Language::French, m68k_symbol_LANGUAGE_FRENCH_val },
	{ ion::platform::Language::German, m68k_symbol_LANGUAGE_GERMAN_val },
	{ ion::platform::Language::LatinAmericanSpanish, m68k_symbol_LANGUAGE_SPANISH_val },
	{ ion::platform::Language::Spanish, m68k_symbol_LANGUAGE_SPANISH_val },
	{ ion::platform::Language::Italian, m68k_symbol_LANGUAGE_ITALIAN_val },
	{ ion::platform::Language::Dutch, m68k_symbol_LANGUAGE_ENGLISH_val },
	{ ion::platform::Language::CanadianFrench, m68k_symbol_LANGUAGE_FRENCH_val },
	{ ion::platform::Language::Portuguese, m68k_symbol_LANGUAGE_ENGLISH_val },
	{ ion::platform::Language::Russian, m68k_symbol_LANGUAGE_ENGLISH_val },
	{ ion::platform::Language::SimplifiedChinese, m68k_symbol_LANGUAGE_ENGLISH_val },
	{ ion::platform::Language::TraditionalChinese, m68k_symbol_LANGUAGE_ENGLISH_val },
	{ ion::platform::Language::Korean, m68k_symbol_LANGUAGE_ENGLISH_val },

#endif
};

#if EVIL_EMU_HD_PLANE_B
const ion::render::TexCoord StateTanglewood::s_texCoordsPlaneB[4] =
{
	ion::Vector2(0.0f, 0.0f),
	ion::Vector2(0.0f, 1.0f),
	ion::Vector2(1.0f, 1.0f),
	ion::Vector2(1.0f, 0.0f)
};
#endif

const ion::Vector3 StateTanglewood::s_scrollNotificationPos = { 1.0f, -0.8f, 1.0f };
const float StateTanglewood::s_scrollNotificationSpeed = 1000.0f;
const float StateTanglewood::s_scrollNotificationDisplayTime = 5.0f;

StateTanglewood::StateTanglewood(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, Settings& settings, SaveManager& saveManager, const ion::Vector2i& windowSize, const ion::Vector2i& emulatorSize, ion::render::Window& window, StatePause& pauseState)
	: StateGame(stateManager, resourceManager, settings, saveManager, windowSize, emulatorSize, window, pauseState)
	, m_gamepadType (ion::input::GamepadType::Generic)
{
	ion::debug::Log("\nT A N G L E W O O D");
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
	ion::debug::Log("-- DEMO --");
#endif
	ion::debug::Log("Copyright (C) 2020 Big Evil Corporation Ltd");
	ion::debug::Log("http://www.tanglewoodgame.com\n");

	//Set resouce directory for lore text files
	ion::engine.io.resourceManager->SetResourceDirectory<TextFile>("assets/text", ".txt");

	//Create resources for scroll pickup notification
	m_textureScrollNotification = ion::engine.io.resourceManager->GetResource<ion::render::Texture>("story_notification",
		[this](ion::render::Texture& texture)
	{
		m_quadPrimitiveScrollNotification = new ion::render::Quad(ion::render::Quad::Axis::xy, ion::Vector2((float)texture.GetWidth() / 2.0f, (float)texture.GetHeight() / 2.0f));
	});

	m_materialScrollNotification = new ion::render::Material();
	m_materialScrollNotification->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));
	m_materialScrollNotification->AddDiffuseMap(m_textureScrollNotification);
	m_scrollEnabled = false;

#if defined ION_RENDERER_SHADER
	//Load shaders
	m_shaderFlatTextured = ion::engine.io.resourceManager->GetResource<ion::render::Shader>("flattextured",
		[this](ion::render::Shader& shader)
	{
#if EVIL_EMU_HD_PLANE_B
		m_shadarParamUVScrollPlaneB = m_shaderFlatTextured->CreateParamHndl<ion::Vector2>("gUVScroll");
#endif // EVIL_EMU_HD_PLANE_B
	});
#endif // ION_RENDERER_SHADER

	m_materialScrollNotification->SetShader(m_shaderFlatTextured);

#if EVIL_EMU_HD_PLANE_B
	m_quadPrimitivePlaneB = new ion::render::Quad(ion::render::Quad::Axis::xy, ion::Vector2((float)ion::engine.render.window->GetClientAreaWidth() / 2.0f, (float)ion::engine.render.window->GetClientAreaHeight() / 2.0f));
	m_texturePlaneB = ion::render::Texture::Create();
	ion::engine.io.resourceManager->GetResource<ion::render::Texture>("bg_test.png");
	m_texturePlaneB->SetWrapping(ion::render::Texture::Wrapping::Repeat);
	m_materialPlaneB = new ion::render::Material();
	m_materialPlaneB->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));
	m_materialPlaneB->AddDiffuseMap(m_texturePlaneB);
#if defined ION_RENDERER_SHADER
	m_materialPlaneB->SetShader(m_shaderFlatTextured);
#endif // ION_RENDERER_SHADER
#endif // EVIL_EMU_HD_PLANE_B
}

void StateTanglewood::OnEnterState()
{
	//Initialise databridgees
#if EVIL_EMU_USE_DATA_BRIDGE && EVIL_EMU_LANGUAGE_SELECT
	m_dataBridge.AddAddress(m68k_symbol_EmuTrap_GetLanguage_val, M68K_SIZE_BYTE, std::bind(&StateTanglewood::OnGetLanguage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#endif

#if EVIL_EMU_USE_DATA_BRIDGE
	m_dataBridge.AddAddress(m68k_symbol_EmuTrap_GetControllerType_val, M68K_SIZE_BYTE, std::bind(&StateTanglewood::OnGetControllerType, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#endif

#if EVIL_EMU_USE_DATA_BRIDGE && EVIL_EMU_USE_SAVES
	m_dataBridge.AddAddress(m68k_symbol_EmuTrap_NewGame_val, M68K_SIZE_BYTE, std::bind(&StateTanglewood::OnNewGame, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_dataBridge.AddAddress(m68k_symbol_EmuTrap_SaveGame_val, M68K_SIZE_BYTE, std::bind(&StateTanglewood::OnSaveGame, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_dataBridge.AddAddress(m68k_symbol_EmuTrap_SaveFireflies_val, M68K_SIZE_BYTE, std::bind(&StateTanglewood::OnSaveFireflies, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_dataBridge.AddAddress(m68k_symbol_EmuTrap_CollectScroll_val, M68K_SIZE_BYTE, std::bind(&StateTanglewood::OnCollectScroll, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_dataBridge.AddAddress(m68k_symbol_EmuTrap_GetSaveAvailable_val, M68K_SIZE_BYTE, std::bind(&StateTanglewood::OnGetSaveAvailable, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_dataBridge.AddAddress(m68k_symbol_EmuTrap_GetSaveData_val, M68K_SIZE_BYTE, std::bind(&StateTanglewood::OnGetSaveData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#endif

#if EVIL_EMU_USE_DATA_BRIDGE && EVIL_EMU_CD_AUDIO
	m_dataBridge.AddAddress(m68k_symbol_EmuData_CurrentCDTrack_val, M68K_SIZE_BYTE, std::bind(&StateTanglewood::OnPlayCDTrack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#endif

#if EVIL_EMU_USE_DATA_BRIDGE && EVIL_EMU_ACHEVEMENTS
	//Initialise achievements
	m_achievements.RegisterWatchers(m_dataBridge);
#endif

	StateGame::OnEnterState();

#if EVIL_EMU_HD_PLANE_B
	VDP_SetDrawPlaneB(false);
	VDP_SetTransparentBG(true);
#endif

	//Begin async save query
	QueryLoginSaves();
}

void StateTanglewood::OnLeaveState()
{
	StateGame::OnLeaveState();
}

void StateTanglewood::OnPauseState()
{
	StateGame::OnPauseState();
}

void StateTanglewood::OnResumeState()
{
	StateGame::OnResumeState();
}

bool StateTanglewood::Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, const std::vector<ion::input::Gamepad*>& gamepads)
{
#if EVIL_EMU_ACHEVEMENTS
	m_achievements.Update();
#endif

	if (gamepads.size() > 0 && gamepads[0])
	{
		m_gamepadType = gamepads[0]->GetGamepadType();
	}

	UpdateScrollNotification(deltaTime);

	return StateGame::Update(deltaTime, keyboard, mouse, gamepads);
}

void StateTanglewood::Render(ion::render::Renderer& renderer, const ion::render::Camera& camera, ion::render::Viewport& viewport)
{
#if EVIL_EMU_HD_PLANE_B
	//Draw replacement plane B
	if (m_materialPlaneB && m_materialPlaneB->GetShader())
	{
		float scrollX = (float)VDP_GetScrollX_PlaneB(0) / (float)VDP_GetPlaneWidthPx();
		//float scrollY = VDP_GetScrollY_PlaneB();

		ion::Matrix4 emuMatrix;
		emuMatrix.SetTranslation(ion::Vector3(0.0f, 0.0f, 1.0f));
		ion::engine.render.renderer->BindMaterial(*m_materialPlaneB, emuMatrix, ion::engine.render.camera->GetTransform().GetInverse(), ion::engine.render.renderer->GetProjectionMatrix());

#if defined ION_RENDERER_SHADER
		m_shadarParamUVScrollPlaneB.SetValue(ion::Vector2(-scrollX, 0.0f));
#else
		ion::render::TexCoord texCoords[4];
		
		for (int i = 0; i < 4; i++)
			texCoords[i] = s_texCoordsPlaneB[i] + ion::Vector2(scrollX, 0.0f);

		m_quadPrimitivePlaneB->SetTexCoords(texCoords);
#endif
		ion::engine.render.renderer->SetAlphaBlending(ion::render::Renderer::AlphaBlendType::None);
		ion::engine.render.renderer->DrawVertexBuffer(m_quadPrimitivePlaneB->GetVertexBuffer(), m_quadPrimitivePlaneB->GetIndexBuffer());

#if defined ION_RENDERER_SHADER
		m_shadarParamUVScrollPlaneB.SetValue(ion::Vector2(0.0f, 0.0f));
#endif

		ion::engine.render.renderer->UnbindMaterial(*m_materialPlaneB);
	}
#endif

	StateGame::Render(renderer, camera, viewport);
	RenderScrollNotification();
}

void StateTanglewood::UpdateScrollNotification(float deltaTime)
{
	m_scrollOffsetX += m_scrollMoveSpeed * deltaTime;

	if (m_scrollDisplayTime > 0.0f)
	{
		m_scrollDisplayTime -= deltaTime;
		if (m_scrollDisplayTime <= 0.0f)
		{
			m_scrollMoveSpeed = s_scrollNotificationSpeed;
			m_scrollDisplayTime = 0.0f;
		}
	}
	else if (m_scrollOffsetX <= -(float)m_textureScrollNotification->GetWidth())
	{
		m_scrollOffsetX = -(float)m_textureScrollNotification->GetWidth();
		m_scrollMoveSpeed = 0.0f;
		m_scrollDisplayTime = s_scrollNotificationDisplayTime;
	}
	else if (m_scrollOffsetX >= (float)m_textureScrollNotification->GetWidth())
	{
		m_scrollOffsetX = (float)m_textureScrollNotification->GetWidth();
		m_scrollMoveSpeed = 0.0f;
	}
}

void StateTanglewood::RenderScrollNotification()
{
	if (m_scrollEnabled && m_quadPrimitiveScrollNotification && m_textureScrollNotification && m_materialScrollNotification->GetShader())
	{
		ion::Matrix4 emuMatrix;
		emuMatrix.SetTranslation(ion::Vector3(
			((m_window.GetWindowWidth() / 2) + (m_textureScrollNotification->GetWidth() / 2) + m_scrollOffsetX) * s_scrollNotificationPos.x,
			((m_window.GetWindowHeight() / 2) - (m_textureScrollNotification->GetHeight() / 2)) * s_scrollNotificationPos.y,
				s_scrollNotificationPos.z));

		ion::engine.render.renderer->BindMaterial(*m_materialScrollNotification, emuMatrix, ion::engine.render.camera->GetTransform().GetInverse(), ion::engine.render.renderer->GetProjectionMatrix());
		ion::engine.render.renderer->SetAlphaBlending(ion::render::Renderer::AlphaBlendType::None);
		ion::engine.render.renderer->DrawVertexBuffer(m_quadPrimitiveScrollNotification->GetVertexBuffer(), m_quadPrimitiveScrollNotification->GetIndexBuffer());
		ion::engine.render.renderer->UnbindMaterial(*m_materialScrollNotification);
	}
}

#if EVIL_EMU_USE_DATA_BRIDGE
void StateTanglewood::OnGetControllerType(u32 watchAddress, int watchSize, u32 watchValue)
{
	//Set controller type
	MEM_setByte(m68k_symbol_GamepadType_val, (u8)m_gamepadType);

	//Notify game if waiting
	MEM_setByte(m68k_symbol_EmuData_AwaitingResponse_val, 0x01);
}
#endif

#if EVIL_EMU_USE_DATA_BRIDGE && EVIL_EMU_LANGUAGE_SELECT
void StateTanglewood::OnGetLanguage(u32 watchAddress, int watchSize, u32 watchValue)
{
	//Get language byte
	u8 languageByte = (u8)s_supportedLanguages.find(ion::platform::Language::BritishEnglish)->second;

	ion::platform::Language language = ion::platform::GetSystemLanguage();

	std::map<ion::platform::Language, unsigned int>::const_iterator it = s_supportedLanguages.find(language);
	if (it != s_supportedLanguages.end())
	{
		languageByte = it->second;
	}

	//Set language
	MEM_setByte(m68k_symbol_GameLanguage_val, languageByte);

	//Notify game if waiting
	MEM_setByte(m68k_symbol_EmuData_AwaitingResponse_val, 0x01);
}
#endif

#if defined ION_SERVICES
void StateTanglewood::OnUserLoggedIn(ion::services::UserManager::LoginResult result, ion::services::User* user)
{
	StateGame::OnUserLoggedIn(result, user);

#if EVIL_EMU_ACHEVEMENTS
	if (result == ion::services::UserManager::LoginResult::Success)
	{
		m_achievements.OnUserLoggedIn(*user);
	}
#endif
}

void StateTanglewood::OnUserLoggedOut(ion::services::User& user)
{
	StateGame::OnUserLoggedOut(user);
}
#endif

#if EVIL_EMU_USE_DATA_BRIDGE && EVIL_EMU_USE_SAVES
void StateTanglewood::OnSaveDataLoaded()
{
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
	MEM_setByte(m68k_symbol_EmuData_SaveAvailable_val, available ? 0x01 : 0x00);

	//Notify game if waiting
	MEM_setByte(m68k_symbol_EmuData_AwaitingResponse_val, 0x01);
}

void StateTanglewood::OnNewGame(u32 watchAddress, int watchSize, u32 watchValue)
{
	//Clear save
#if !EVIL_EMU_MULTIPLE_SAVESLOTS
	Save::SaveSlot saveSlot;
	saveSlot.gameType = EVIL_EMU_GAME_TYPE;
	saveSlot.password = 0;
	m_save.m_saveSlots.resize(1);
	m_save.m_saveSlots[0] = saveSlot;

	//Save to disk
	m_saveManager.SaveGame(*m_currentUser, m_save,
		[](bool success)
	{
		//TODO: Respond to failure
	});
#endif
}

void StateTanglewood::OnSaveGame(u32 watchAddress, int watchSize, u32 watchValue)
{
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

	ion::debug::Assert(m_currentUser, "StateGame::OnSaveGame() - No current user");

	//New save slot
	Save::SaveSlot saveSlot;

	saveSlot.serialiseData.clear();
	saveSlot.serialiseData.reserve(m68k_symbol_CheckpointSerialiseBlockSize_val);

	saveSlot.fireflyData.clear();
	saveSlot.fireflyData.reserve(m68k_symbol_FireflyBitmaskSize_val);

#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL
	//Copy serialise block
	for (int i = 0; i < m68k_symbol_CheckpointSerialiseBlockSize_val; i++)
	{
		saveSlot.serialiseData.push_back(MEM_getByte(m68k_symbol_CheckpointSerialiseMemBlock_val + i));
	}

	//Copy firefly block
	for (int i = 0; i < m68k_symbol_FireflyBitmaskSize_val; i++)
	{
		saveSlot.fireflyData.push_back(MEM_getByte(m68k_symbol_FireflyCollectedMask_val + i));
	}

	//Get save version
	saveSlot.saveVersion = MEM_getWord(m68k_symbol_LastSaveVersion_val);
#endif

	//Get game type
	saveSlot.gameType = EVIL_EMU_GAME_TYPE;

	//Get password
	saveSlot.password = MEM_getLong(m68k_symbol_CurrentSavePassword_val);

	//Get firefly counts
	saveSlot.firefliesAct = MEM_getWord(m68k_symbol_FireflyPickupCountAct_val);
	saveSlot.firefliesGame = MEM_getWord(m68k_symbol_FireflyPickupCountTotalSave_val);

	//Get last saved level address
	u32 levelAddress = MEM_getLong(m68k_symbol_LastSaveLevel_val);

	//If not saved yet, get current level address
	if(!levelAddress)
		levelAddress = MEM_getLong(m68k_symbol_CurrentLevel_val);

	saveSlot.levelAddr = levelAddress;

	//Get level index
	saveSlot.levelIdx = MEM_getByte(saveSlot.levelAddr + m68k_symbol_Level_Index_val);

	//Get save version
	saveSlot.saveVersion = MEM_getWord(m68k_symbol_LastSaveVersion_val);

	//If slot differs from last
	if (m_save.m_saveSlots.size() == 0 || !(saveSlot == m_save.m_saveSlots.back()))
	{
		//Time stamp
		saveSlot.timeStamp = ion::time::GetLocalTime();

		//Add slot
#if EVIL_EMU_MULTIPLE_SAVESLOTS
		m_save.m_saveSlots.push_back(saveSlot);
#else
		m_save.m_saveSlots.resize(1);
		m_save.m_saveSlots[0] = saveSlot;
#endif

		//Save to disk
		m_saveManager.SaveGame(*m_currentUser, m_save,
			[](bool success)
		{
			//TODO: Respond to failure
		});
	}
}

void StateTanglewood::OnSaveFireflies(u32 watchAddress, int watchSize, u32 watchValue)
{
	//Update firefly data for current slot
	if (m_save.m_saveSlots.size() > 0)
	{
		Save::SaveSlot& saveSlot = m_save.m_saveSlots.back();
		
		saveSlot.fireflyData.clear();
		saveSlot.fireflyData.reserve(m68k_symbol_FireflyBitmaskSize_val);

		//Copy firefly block
		for (int i = 0; i < m68k_symbol_FireflyBitmaskSize_val; i++)
		{
			saveSlot.fireflyData.push_back(MEM_getByte(m68k_symbol_FireflyCollectedMask_val + i));
		}

		//Update time stamp
		saveSlot.timeStamp = ion::time::GetLocalTime();

		//Save to disk
		m_saveManager.SaveGame(*m_currentUser, m_save,
			[](bool success)
		{
			//TODO: Respond to failure
		});
	}
	else
	{
		//No slot, create new save
		OnSaveGame(watchAddress, watchSize, watchValue);
	}
}

void StateTanglewood::OnCollectScroll(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue != 0xFF)
	{
		//Save scrolls

		//Open scroll page
		//m_stateReadLore->SetLorePage(watchValue);
		//m_stateManager.PushState(*m_stateReadLore);

		m_scrollDisplayTime = 0.0f;
		m_scrollMoveSpeed = -s_scrollNotificationSpeed;
	}
}

void StateTanglewood::OnGetSaveAvailable(u32 watchAddress, int watchSize, u32 watchValue)
{
	//Begin async save query
	QueryLoginSaves();
}

void StateTanglewood::OnGetSaveData(u32 watchAddress, int watchSize, u32 watchValue)
{
#if EVIL_EMU_MULTIPLE_SAVESLOTS
	if (m_save.m_saveSlots.size() > 1)
	{
		//Open save slot menu
		m_saveSlotsMenu = new MenuSaveSlots(*m_gui, /* font, */ m_window, m_save,
			[&](int index)
		{
			if (index >= 0)
			{
				//Apply save slot data
				InjectSaveData(index);

				//Notify game if waiting
				MEM_setByte(m68k_symbol_EmuData_AwaitingResponse_val, 0x01);
			}
			else
			{
				//Canceled
				MEM_setByte(m68k_symbol_EmuData_AwaitingResponse_val, 0x00);
			}

			//Destroy save slots menu
			m_gui->DeleteWindow(*m_saveSlotsMenu);
			m_saveSlotsMenu = nullptr;
		}
		);

		m_gui->AddWindow(*m_saveSlotsMenu);
	}
	else
#endif
	{
		//Apply save slot 0 data
		InjectSaveData(0);

		//Notify game if waiting
		MEM_setByte(m68k_symbol_EmuData_AwaitingResponse_val, 0x01);
	}
}

void StateTanglewood::InjectSaveData(int slot)
{
	//If data saved with FINAL build, but we're running DEMO, data isn't compatible
#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_DEMO
	if (m_save.m_saveSlots[slot].gameType != EVIL_EMU_GAME_TYPE_DEMO)
	{
		MEM_setByte(m68k_symbol_EmuData_SaveAvailable_val, 0x00);
		MEM_setLong(m68k_symbol_CurrentSavePassword_val, 0x00000000);
		MEM_setByte(m68k_symbol_EmuData_AwaitingResponse_val, 0x01);
		return;
	}
#endif

#if EVIL_EMU_GAME_TYPE==EVIL_EMU_GAME_TYPE_FINAL
	//Inject serialise block into game memory
	if (m68k_symbol_CheckpointSerialiseBlockSize_val == m_save.m_saveSlots[slot].serialiseData.size())
	{
		for (int i = 0; i < m68k_symbol_CheckpointSerialiseBlockSize_val; i++)
		{
			MEM_setByte(m68k_symbol_CheckpointSerialiseMemBlock_val + i, m_save.m_saveSlots[slot].serialiseData[i]);
		}
	}

	//Inject firefly block into game memory
	if (m68k_symbol_FireflyBitmaskSize_val == m_save.m_saveSlots[slot].fireflyData.size())
	{
		for (int i = 0; i < m68k_symbol_FireflyBitmaskSize_val; i++)
		{
			MEM_setByte(m68k_symbol_FireflyCollectedMask_val + i, m_save.m_saveSlots[slot].fireflyData[i]);
		}
	}

	//Set save version
	MEM_setWord(m68k_symbol_LastSaveVersion_val, m_save.m_saveSlots[slot].saveVersion);
#endif

	//Set password
	MEM_setLong(m68k_symbol_CurrentSavePassword_val, m_save.m_saveSlots[slot].password);

	//Set firefly counts
	MEM_setWord(m68k_symbol_FireflyPickupCountAct_val, m_save.m_saveSlots[slot].firefliesAct);
	MEM_setWord(m68k_symbol_FireflyPickupCountTotalSave_val, m_save.m_saveSlots[slot].firefliesGame);

	//Set level address (fetch from index - address may change between builds)
	u32 levelAddrEntry = m68k_symbol_LevelList_val + (m_save.m_saveSlots[slot].levelIdx * M68K_SIZE_LONG);
	u32 levelAddr = MEM_getLong(levelAddrEntry);
	MEM_setLong(m68k_symbol_CurrentLevel_val, levelAddr);
}
#endif

#if EVIL_EMU_USE_DATA_BRIDGE && EVIL_EMU_CD_AUDIO
void StateTanglewood::OnPlayCDTrack(u32 watchAddress, int watchSize, u32 watchValue)
{
	m_audioAmbience.PlayTrack(watchValue);
}
#endif