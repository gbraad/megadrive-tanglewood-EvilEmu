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

#include <ion/core/containers/FixedArray.h>
#include <ion/gamekit/StateManager.h>

#include "emulator.h"

class StateControlsConfig : public ion::gamekit::State, public ion::input::KeyboardHandler
{
public:
	StateControlsConfig(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, ion::FixedArray<u32, eBtn_MAX>& keymap);

	virtual void OnEnterState();
	virtual void OnLeaveState();
	virtual void OnPauseState();
	virtual void OnResumeState();

	virtual void Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, ion::input::Gamepad* gamepad);
	virtual void Render(ion::render::Renderer& renderer, ion::render::Camera& camera);

	virtual void OnKeyDown(int key);
	virtual void OnKeyUp(int key);

private:
	ion::FixedArray<u32, eBtn_MAX>& m_keymap;
	int m_currentKeyIdx;
	ion::input::Keyboard* m_registeredKeyboard;
};