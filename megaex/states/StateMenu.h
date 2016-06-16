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

class StateMenu : public ion::gamekit::State
{
public:
	StateMenu(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, ion::gamekit::State* stateControlsConfig, ion::gamekit::State* stateGame);

	virtual void OnEnterState();
	virtual void OnLeaveState();
	virtual void OnPauseState();
	virtual void OnResumeState();

	virtual void Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, ion::input::Gamepad* gamepad);
	virtual void Render(ion::render::Renderer& renderer, ion::render::Camera& camera);

private:
	ion::gamekit::State* m_stateControlsConfig;
	ion::gamekit::State* m_stateGame;
};