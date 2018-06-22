////////////////////////////////////////////////////////////////////////////////
// megaEx
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

#include "emulator.h"
#include "emuthread.h"

class StateGame : public ion::gamekit::State
{
public:
	StateGame(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, const ion::Vector2i& windowSize, const ion::Vector2i& emulatorSize, ion::render::Window& window);

	virtual void OnEnterState();
	virtual void OnLeaveState();
	virtual void OnPauseState();
	virtual void OnResumeState();

	virtual void Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, ion::input::Gamepad* gamepad);
	virtual void Render(ion::render::Renderer& renderer, ion::render::Camera& camera);

	void ChangeWindowSize(const ion::Vector2i& size);

private:
	static const ion::render::TexCoord s_texCoordsGame[4];
	static const ion::render::TexCoord s_texCoordsDebugger[4];

	EmulatorThread* m_emulatorThread;
	EmulatorState m_prevEmulatorState;

	ion::render::Window& m_window;
	ion::Vector2i m_windowSize;
	ion::Vector2i m_emulatorSize;

	ion::render::Texture* m_renderTexture;
	ion::render::Material* m_materialEmu;
	ion::render::Quad* m_quadPrimitiveEmu;

#if defined ION_RENDERER_SHADER
	ion::render::Shader* m_vertexShader;
	ion::render::Shader* m_pixelShader;
#endif

	//Timing
	u64 m_frameCount;
	u64 m_startTicks;
};
