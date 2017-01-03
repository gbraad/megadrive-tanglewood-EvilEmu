#include <ion/core/time/Time.h>

#include "megaex.h"

int main(char** args, int numargs)
{
	MegaEx app;

	if(app.Initialise())
	{
		float deltaTime = 0.0f;
		bool run = true;
		while(run)
		{
			u64 startTicks = ion::time::GetSystemTicks();

			if(run = app.Update(deltaTime))
			{
				app.Render();
			}

			u64 endTicks = ion::time::GetSystemTicks();

#if defined ION_PLATFORM_DREAMCAST
			deltaTime = 0.003f;
#else
			deltaTime = (float)ion::time::TicksToSeconds(endTicks - startTicks);
#endif
		}

		app.Shutdown();
	}
}