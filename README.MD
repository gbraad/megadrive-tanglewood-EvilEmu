
# EvilEmu

Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator

Uses mega emulation code based on the work of Lee Hammerton and Jake Turner

Matt Phillips
http://www.bigevilcorporation.co.uk

# Getting Started

EvilEmu (and all other ion::engine projects) use the JamPlus build system (see https://github.com/jamplus/jamplus).
JamPlus includes support for XCode and other IDEs, but the .sh files need fetching from JamPlus source and I haven't
used it in a long time so mileage may vary.

- Check out ion::engine, place EvilEmu and ion::engine in the same directory
- Run ion_symlink.bat AS ADMINISTRATOR to link up the engine
- Run tanglewood_emu/GenerateWorkspace_VS20XX.bat to create and open a Visual Studio workspace
