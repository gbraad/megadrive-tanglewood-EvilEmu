#include <ion/engine/Engine.h>
#include <ion/core/time/Time.h>
#include <ion/core/debug/CrashHandler.h>
#include <ion/core/Platform.h>
#include "evil_emu.h"
#include "constants.h"
#include "settings.h"

#include <ion/core/thread/Sleep.h>

namespace ion
{
	int EntryPoint(int numargs, char** args)
	{
		Settings defaultSettings;

		//Create engine
		ion::Engine::Config ionConfig;

		ionConfig.window.createWindow = true;
		ionConfig.window.title = EVIL_EMU_APP_TITLE;
		ionConfig.window.clientAreaWidth = defaultSettings.resolution.x;
		ionConfig.window.clientAreaHeight = defaultSettings.resolution.y;
		ionConfig.window.fullscreen = defaultSettings.fullscreen;

		ionConfig.render.createRenderer = true;
		ionConfig.render.createCamera = true;
		ionConfig.render.perspectiveMode = ion::render::Viewport::PerspectiveMode::Ortho2DAbsolute;

		ionConfig.audio.supportAudio = true;

		ionConfig.input.supportKeyboard = true;
		ionConfig.input.supportMouse = true;
		ionConfig.input.maxSupportedControllers = EVIL_EMU_MAX_GAMEPADS;

		ionConfig.services.supportPlatformServices = true;
		ionConfig.services.appId = EVIL_EMU_APP_ID;
		ionConfig.services.appEncryptionKey = EVIL_EMU_APP_KEY;

		ion::engine.Initialise(ionConfig);

		//Create app
		EvilEmu app;

		if (app.Initialise())
		{
			float deltaTime = 0.0f;
			bool run = true;
			while (run)
			{
				u64 startTicks = ion::time::GetSystemTicks();
				run = app.Update(deltaTime);

				if (run)
				{
					app.Render();
				}

				u64 endTicks = ion::time::GetSystemTicks();
				deltaTime = (float)ion::time::TicksToSeconds(endTicks - startTicks);
			}

			app.Shutdown();
		}

		//Shutdown platform
		ion::engine.Shutdown();

		return 0;
	}
}