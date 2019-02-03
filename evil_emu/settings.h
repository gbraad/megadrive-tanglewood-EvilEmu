#pragma once

#include <ion/core/containers/FixedArray.h>
#include <ion/input/Keycodes.h>
#include <ion/input/Gamepad.h>
#include <ion/io/Archive.h>

#include "emulator.h"
#include "constants.h"

struct Settings
{
	Settings()
	{
		resolution.x = 800;
		resolution.y = 600;
		fullscreen = true;
		displayIdx = 0;
		vsync = true;
#if defined ION_PLATFORM_WINDOWS
		pixelBuffer = true;
#else
		pixelBuffer = false;
#endif
		videoBorder = VideoBorder::VDPColour;
		scanlineAlpha = EMU_SCANLINE_DEFAULT_ALPHA;

		keyboardMap[eBtn_Up] = ion::input::Keycode::UP;
		keyboardMap[eBtn_Down] = ion::input::Keycode::DOWN;
		keyboardMap[eBtn_Left] = ion::input::Keycode::LEFT;
		keyboardMap[eBtn_Right] = ion::input::Keycode::RIGHT;
		keyboardMap[eBtn_A] = ion::input::Keycode::A;
		keyboardMap[eBtn_B] = ion::input::Keycode::S;
		keyboardMap[eBtn_C] = ion::input::Keycode::D;
		keyboardMap[eBtn_Start] = ion::input::Keycode::RETURN;

		gamepadMap[eBtn_Up] = ion::input::GamepadButtons::DPAD_UP;
		gamepadMap[eBtn_Down] = ion::input::GamepadButtons::DPAD_DOWN;
		gamepadMap[eBtn_Left] = ion::input::GamepadButtons::DPAD_LEFT;
		gamepadMap[eBtn_Right] = ion::input::GamepadButtons::DPAD_RIGHT;
		gamepadMap[eBtn_A] = ion::input::GamepadButtons::BUTTON_X;
		gamepadMap[eBtn_B] = ion::input::GamepadButtons::BUTTON_A;
		gamepadMap[eBtn_C] = ion::input::GamepadButtons::BUTTON_B;
		gamepadMap[eBtn_Start] = ion::input::GamepadButtons::START;
	}

	void Serialise(ion::io::Archive& archive)
	{
		archive.Serialise(resolution, "resolution");
		archive.Serialise(fullscreen, "fullscreen");
		archive.Serialise(displayIdx, "displayIdx");
		archive.Serialise(vsync, "vsync");
		archive.Serialise(pixelBuffer, "pixelBuffer");
		archive.Serialise(scanlineAlpha, "scanlineAlpha");
		archive.Serialise((ion::FixedArray<int, eBtn_MAX>&)keyboardMap, "keyboardMap");
		archive.Serialise((ion::FixedArray<int, eBtn_MAX>&)gamepadMap, "gamepadMap");
	}

	ion::Vector2i resolution;
	bool fullscreen;
	int displayIdx;
	bool vsync;
	bool pixelBuffer;
	float scanlineAlpha;
	VideoBorder videoBorder;
	ion::FixedArray<ion::input::Keycode, eBtn_MAX> keyboardMap;
	ion::FixedArray<ion::input::GamepadButtons, eBtn_MAX> gamepadMap;
};
