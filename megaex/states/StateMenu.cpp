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

StateMenu::StateMenu(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager)
	: ion::gamekit::State(stateManager, resourceManager)
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

}

void StateMenu::Render(ion::render::Renderer& renderer, ion::render::Camera& camera)

{

}