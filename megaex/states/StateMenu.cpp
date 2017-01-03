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

#include "StateMenu.h"

StateMenu::StateMenu(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, ion::gamekit::State* stateControlsConfig, ion::gamekit::State* stateGame)
	: ion::gamekit::State(stateManager, resourceManager)
	, m_stateControlsConfig(stateControlsConfig)
	, m_stateGame(stateGame)
{

}

void StateMenu::OnEnterState()
{

}

void StateMenu::OnLeaveState()
{

}

void StateMenu::OnPauseState()
{

}

void StateMenu::OnResumeState()
{

}

void StateMenu::Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, ion::input::Gamepad* gamepad)
{
#if defined ION_PLATFORM_WINDOWS
	if(keyboard->KeyPressedThisFrame(DIK_K))
	{
		m_stateManager.PushState(*m_stateControlsConfig);
	}
#endif

#if defined ION_PLATFORM_WINDOWS
	if(keyboard->KeyPressedThisFrame(DIK_G))
#endif
	{
		m_stateManager.PushState(*m_stateGame);
	}
}

void StateMenu::Render(ion::render::Renderer& renderer, ion::render::Camera& camera)
{

}