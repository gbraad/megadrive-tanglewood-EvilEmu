#pragma once

#include "states/StateGame.h"

#include "CDAudio.h"
#include "MarkupText.h"

#if defined ION_SERVICES
#include "Achievements.h"
#endif

class StateTanglewood : public StateGame
{
public:
	StateTanglewood(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, Settings& settings, SaveManager& saveManager, const ion::Vector2i& windowSize, const ion::Vector2i& emulatorSize, ion::render::Window& window, StatePause& pauseState);

	virtual void OnEnterState();
	virtual void OnLeaveState();
	virtual void OnPauseState();
	virtual void OnResumeState();

	virtual bool Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, const std::vector<ion::input::Gamepad*>& gamepads);
	virtual void Render(ion::render::Renderer& renderer, const ion::render::Camera& camera, ion::render::Viewport& viewport);

protected:
#if defined ION_SERVICES
	virtual void OnUserLoggedIn(ion::services::UserManager::LoginResult result, ion::services::User* user);
	virtual void OnUserLoggedOut(ion::services::User& user);
#endif

#if EVIL_EMU_USE_SAVES
	//Inject flag into RAM to notify the game that save data is available
	virtual void OnSaveDataLoaded();

	//Inject save data into RAM
	void InjectSaveData(int slot);
#endif

#if EVIL_EMU_USE_DATA_BRIDGE
	//Memwatch callbacks for controller config
	void OnGetControllerType(u32 watchAddress, int watchSize, u32 watchValue);
#endif

#if EVIL_EMU_USE_DATA_BRIDGE && EVIL_EMU_USE_SAVES
	//Memwatch callbacks for language/save game
	void OnGetLanguage(u32 watchAddress, int watchSize, u32 watchValue);
	void OnNewGame(u32 watchAddress, int watchSize, u32 watchValue);
	void OnSaveGame(u32 watchAddress, int watchSize, u32 watchValue);
	void OnSaveFireflies(u32 watchAddress, int watchSize, u32 watchValue);
	void OnCollectScroll(u32 watchAddress, int watchSize, u32 watchValue);
	void OnGetSaveAvailable(u32 watchAddress, int watchSize, u32 watchValue);
	void OnGetSaveData(u32 watchAddress, int watchSize, u32 watchValue);
#endif

#if EVIL_EMU_USE_DATA_BRIDGE && EVIL_EMU_CD_AUDIO
	//Memwatch callbacks for CD audio playback
	void OnPlayCDTrack(u32 watchAddress, int watchSize, u32 watchValue);
#endif

private:
	void UpdateScrollNotification(float deltaTime);
	void RenderScrollNotification();

	static const std::map<ion::platform::Language, unsigned int> s_supportedLanguages;

	ion::input::GamepadType m_gamepadType;

#if EVIL_EMU_ACHEVEMENTS
	Achievements m_achievements;
#endif

#if EVIL_EMU_CD_AUDIO
	AudioStream m_audioAmbience;
#endif

#if EVIL_EMU_HD_PLANE_B
	static const ion::render::TexCoord s_texCoordsPlaneB[4];
	ion::render::Texture* m_texturePlaneB;
	ion::render::Material* m_materialPlaneB;
	ion::render::Quad* m_quadPrimitivePlaneB;

#if defined ION_RENDERER_SHADER
	ion::render::Shader::ParamHndl<ion::Vector2> m_shadarParamUVScrollPlaneB;
#endif
#endif

#if defined ION_RENDERER_SHADER
	ion::io::ResourceHandle<ion::render::Shader> m_shaderFlatTextured;
#endif

	static const ion::Vector3 s_scrollNotificationPos;
	static const float s_scrollNotificationSpeed;
	static const float s_scrollNotificationDisplayTime;
	bool m_scrollEnabled;
	float m_scrollDisplayTime;
	float m_scrollMoveSpeed;
	float m_scrollOffsetX;
	ion::render::Material* m_materialScrollNotification;
	ion::io::ResourceHandle<ion::render::Texture> m_textureScrollNotification;
	ion::render::Quad* m_quadPrimitiveScrollNotification;
};