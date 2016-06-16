////////////////////////////////////////////////////////////////////////////////
// megaEx
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

#include "StateControlsConfig.h"

StateControlsConfig::StateControlsConfig(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, ion::FixedArray<u32, eBtn_MAX>& keymap)
	: ion::gamekit::State(stateManager, resourceManager)
	, m_keymap(keymap)
{
	m_currentKeyIdx = 0;
	m_registeredKeyboard = NULL;
}

void StateControlsConfig::OnEnterState()
{

}

void StateControlsConfig::OnLeaveState()
{
	if(m_registeredKeyboard)
	{
		m_registeredKeyboard->UnregisterHandler(*this);
	}
}

void StateControlsConfig::OnPauseState()
{

}

void StateControlsConfig::OnResumeState()
{

}

void StateControlsConfig::Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, ion::input::Gamepad* gamepad)
{
	//TODO: Pass to constructor/OnEnterState
	if(!m_registeredKeyboard)
	{
		keyboard->RegisterHandler(*this);
		m_registeredKeyboard = keyboard;
	}

	if(m_currentKeyIdx == eBtn_MAX - 1)
	{
		//Finished
		m_stateManager.PopState();
	}
}

void StateControlsConfig::Render(ion::render::Renderer& renderer, ion::render::Camera& camera)
{

}

void StateControlsConfig::OnKeyDown(int key)
{
	m_keymap[m_currentKeyIdx++] = key;
}

void StateControlsConfig::OnKeyUp(int key)
{

}