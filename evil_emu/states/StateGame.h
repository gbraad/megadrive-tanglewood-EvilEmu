////////////////////////////////////////////////////////////////////////////////
// EvilEmu
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ion/gamekit/StateManager.h>
#include <ion/renderer/Texture.h>
#include <ion/renderer/Shader.h>
#include <ion/renderer/Material.h>
#include <ion/renderer/Primitive.h>
#include <ion/renderer/TexCoord.h>
#include <ion/gui/GUI.h>
#include <ion/io/Serialise.h>

#include "constants.h"
#include "emulator.h"
#include "emuthread.h"
#include "memwatch.h"
#include "debugger/DebuggerUI.h"
#include "StatePause.h"
#include "savegame.h"
#include "settings.h"

#include "menu/MenuSaveSlots.h"

#if defined ION_ONLINE
#include "achievements.h"
#endif

class StateGame : public ion::gamekit::State
{
public:
	StateGame(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, Settings& settings, SaveManager& saveManager, const ion::Vector2i& windowSize, const ion::Vector2i& emulatorSize, ion::render::Window& window, StatePause& pauseState);

	virtual void OnEnterState();
	virtual void OnLeaveState();
	virtual void OnPauseState();
	virtual void OnResumeState();

	virtual bool Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, ion::input::Gamepad* gamepad);
	virtual void Render(ion::render::Renderer& renderer, const ion::render::Camera& camera, ion::render::Viewport& viewport);

	void ApplySettings();

private:
	void OnSaveGame(u32 watchAddress, int watchSize, u32 watchValue);
	void OnGetSaveAvailable(u32 watchAddress, int watchSize, u32 watchValue);
	void OnGetSaveData(u32 watchAddress, int watchSize, u32 watchValue);

	void InitSaves();
	void ApplySaveData(int slot);

#if EMU_USE_SCANLINES
	void DrawScanlineTexture(const ion::Colour& colour);
#endif

	static const ion::render::TexCoord s_texCoordsGame[4];
	static const ion::render::TexCoord s_texCoordsDebugger[4];

	EmulatorThread* m_emulatorThread;
	EmulatorState m_prevEmulatorState;
	bool m_emulatorThreadRunning;

	ion::render::Window& m_window;
	ion::Vector2i m_windowSize;
	ion::Vector2i m_emulatorSize;

#if EMU_USE_2X_BUFFER
	u32* m_pixelScaleBuffer;
#endif

#if EMU_USE_SCANLINES
	ion::render::Texture* m_scanlineTexture;
	ion::render::Material* m_scanlineMaterial;
#endif

	ion::render::Texture* m_renderTextures[EMU_NUM_RENDER_BUFFERS];
	ion::render::Material* m_renderMaterials[EMU_NUM_RENDER_BUFFERS];
	ion::render::Quad* m_quadPrimitiveEmu;

	int m_currentBufferIdx;

#if defined ION_RENDERER_SHADER
	ion::render::Shader* m_vertexShader;
	ion::render::Shader* m_pixelShader;
#endif

	Settings& m_settings;
	StatePause& m_pauseState;

	ion::gui::GUI* m_gui;
	DebuggerUI* m_debuggerUI;

	MemWatcher m_memWatcher;

	SaveManager m_saveManager;
	Save m_save;
	MenuSaveSlots* m_saveSlotsMenu;

#if EVIL_EMU_ACHEVEMENTS
	Achievements m_achievements;
#endif

	//Timing
	u64 m_frameCount;
	u64 m_startTicks;
	float m_emuStartTimer;

	FPSCounter m_fpsCounter;
};
