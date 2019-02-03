..\BuildTools\JamPlus\Bin\Win64\Jam.exe --workspace --gen=vs2017 -compiler=vs2017 --config=jamplus.config evil_emu.jam _Build

CALL _Build\_workspaces_\vs2017\evil_emu.sln
