#pragma once

#include <ion/core/containers/FixedArray.h>
#include <ion/input/Keycodes.h>
#include <ion/input/Gamepad.h>
#include <ion/core/io/Archive.h>

#include "emulator.h"
#include "constants.h"

struct Settings
{
	Settings()
	{
		resolution.x = 1280;
		resolution.y = 720;
		fullscreen = true;
		displayIdx = 0;
		vsync = true;
#if defined ION_PLATFORM_WINDOWS || defined ION_PLATFORM_SWITCH
		pixelBuffer = true;
#else
		pixelBuffer = false;
#endif
		videoBorder = VideoBorder::VDPColour;
		scanlineAlpha = EVIL_EMU_SCANLINE_DEFAULT_ALPHA;

		analogueDeadzone = 0.5f;

		keyboardMap[eBtn_Up] = ion::input::Keycode::UP;
		keyboardMap[eBtn_Down] = ion::input::Keycode::DOWN;
		keyboardMap[eBtn_Left] = ion::input::Keycode::LEFT;
		keyboardMap[eBtn_Right] = ion::input::Keycode::RIGHT;
		keyboardMap[eBtn_A] = ion::input::Keycode::A;
		keyboardMap[eBtn_B] = ion::input::Keycode::S;
		keyboardMap[eBtn_C] = ion::input::Keycode::D;
		keyboardMap[eBtn_Start] = ion::input::Keycode::RETURN;
		keyboardMap[eBtn_X] = ion::input::Keycode::Q;
		keyboardMap[eBtn_Y] = ion::input::Keycode::W;
		keyboardMap[eBtn_Z] = ion::input::Keycode::E;
		keyboardMap[eBtn_Mode] = ion::input::Keycode::X;

		gamepadMap[eBtn_Up].push_back(ion::input::GamepadButtons::DPAD_UP);
		gamepadMap[eBtn_Down].push_back(ion::input::GamepadButtons::DPAD_DOWN);
		gamepadMap[eBtn_Left].push_back(ion::input::GamepadButtons::DPAD_LEFT);
		gamepadMap[eBtn_Right].push_back(ion::input::GamepadButtons::DPAD_RIGHT);
		gamepadMap[eBtn_A].push_back(ion::input::GamepadButtons::BUTTON_X);
		gamepadMap[eBtn_B].push_back(ion::input::GamepadButtons::BUTTON_A);
		gamepadMap[eBtn_C].push_back(ion::input::GamepadButtons::BUTTON_B);
		gamepadMap[eBtn_X].push_back(ion::input::GamepadButtons::LEFT_BUMPER);
		gamepadMap[eBtn_X].push_back(ion::input::GamepadButtons::LEFT_TRIGGER);
		gamepadMap[eBtn_Y].push_back(ion::input::GamepadButtons::RIGHT_BUMPER);
		gamepadMap[eBtn_Y].push_back(ion::input::GamepadButtons::RIGHT_TRIGGER);
		gamepadMap[eBtn_Z].push_back(ion::input::GamepadButtons::BUTTON_Y);
		gamepadMap[eBtn_Mode].push_back(ion::input::GamepadButtons::SELECT);
		gamepadMap[eBtn_Start].push_back(ion::input::GamepadButtons::START);
	}

	void Serialise(ion::io::Archive& archive)
	{
		archive.Serialise(resolution, "resolution");
		archive.Serialise(fullscreen, "fullscreen");
		archive.Serialise(displayIdx, "displayIdx");
		archive.Serialise(vsync, "vsync");
		archive.Serialise(pixelBuffer, "pixelBuffer");
		archive.Serialise(scanlineAlpha, "scanlineAlpha");
		archive.Serialise(analogueDeadzone, "analogueDeadzone");
		archive.Serialise((ion::FixedArray<int, eBtn_MAX>&)keyboardMap, "keyboardMap");
		archive.Serialise((ion::FixedArray< std::vector<int>, eBtn_MAX>&)gamepadMap, "gamepadMap");
	}

	ion::Vector2i resolution;
	bool fullscreen;
	int displayIdx;
	bool vsync;
	bool pixelBuffer;
	float scanlineAlpha;
	VideoBorder videoBorder;
	float analogueDeadzone;
	ion::FixedArray<ion::input::Keycode, eBtn_MAX> keyboardMap;
	ion::FixedArray<std::vector<ion::input::GamepadButtons>, eBtn_MAX> gamepadMap;
};
