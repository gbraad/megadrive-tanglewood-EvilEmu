pushd C:\dev\tanglewood_code\bigevilframework\TANGLEWD
C:\dev\MDStudio\3rdparty\assemblers\asm68k.exe /p /c /zd /w /k /e RELEASE=1;FINAL=1;EMUBUILD=1  C:\dev\tanglewood_code\bigevilframework\TANGLEWD\TANGLEWD.ASM,TANGLEWD.bin,TANGLEWD.symb,TANGLEWD.list
popd

if %ERRORLEVEL% NEQ 0 goto err

..\..\m68k_bin2cpp\x64\release\m68k_bin2cpp.exe ^
-s ^
C:\dev\tanglewood_code\bigevilframework\TANGLEWD\TANGLEWD.BIN ^
C:\dev\tanglewood_code\bigevilframework\TANGLEWD\TANGLEWD.SYMB ^
..\roms\tanglewd_final_rom.h ^
..\roms\tanglewd_final_vars.h ^
EmuData_AwaitingResponse ^
EmuData_SaveAvailable ^
EmuData_CurrentCDTrack ^
EmuTrap_GetSaveAvailable ^
EmuTrap_GetSaveData ^
EmuTrap_NewGame ^
EmuTrap_SaveGame ^
EmuTrap_SaveFireflies ^
EmuTrap_GetLanguage ^
EmuTrap_GetControllerType ^
EmuTrap_CollectScroll ^
CheckpointSerialiseMemBlock ^
CheckpointSerialiseBlockSize ^
FireflyBitmaskSize ^
FireflyCollectedMask ^
FireflyPickupCountAct ^
FireflyPickupCountTotalUI ^
FireflyPickupCountTotalSave ^
TotalFireflyCount ^
CurrentSavePassword ^
LastSaveVersion ^
CurrentLevel ^
LastSaveLevel ^
Level_Index ^
LevelList ^
EmuTrap_ACH_L1A4_KILL_DJAKK ^
EmuTrap_ACH_L2A3_KILL_DJAKK ^
EmuTrap_ACH_SURVIVE_SCIRUS ^
EmuTrap_ACH_SURVIVE_DJAKK ^
EmuTrap_ACH_BORGUS_TWO_BOULDERS ^
EmuTrap_ACH_KILL_SWARM ^
EmuTrap_ACH_COMPLETE_GAME ^
EmuTrap_ACH_COMPLETE_GAME_ALT ^
EmuTrap_ACH_HOGG_WALL ^
EmuTrap_ACH_ECHO_FIREFLY ^
EmuTrap_ACH_CART_RACE ^
EmuTrap_ACH_SURVIVE_WYRM ^
EmuTrap_ACH_L1A3_SNEAK_DJAKK ^
EmuTrap_ACH_KILL_ALL_DJUBBS ^
Ach_Data_BoulderSmashCount ^
GameLanguage ^
GamepadType ^
LANGUAGE_ENGLISH ^
LANGUAGE_FRENCH ^
LANGUAGE_ITALIAN ^
LANGUAGE_GERMAN ^
LANGUAGE_SPANISH

:err