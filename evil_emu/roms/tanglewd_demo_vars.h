//------------------------------------------------------------------
// m68k_bin2cpp
// BIG EVIL CORPORATION LTD
// M68K binary/symbols to C++ tool
// Matt Phillips 2020
//------------------------------------------------------------------

// Symbol addresses
static const unsigned int m68k_symbol_EmuData_AwaitingResponse_val = 0x00FF3086;
static const unsigned int m68k_symbol_EmuData_SaveAvailable_val = 0x00FF3087;
static const unsigned int m68k_symbol_EmuData_CurrentCDTrack_val = 0x00FF3088;
static const unsigned int m68k_symbol_EmuTrap_GetSaveAvailable_val = 0x00FF3089;
static const unsigned int m68k_symbol_EmuTrap_GetSaveData_val = 0x00FF308A;
static const unsigned int m68k_symbol_EmuTrap_NewGame_val = 0x00FF308B;
static const unsigned int m68k_symbol_EmuTrap_SaveGame_val = 0x00FF308C;
static const unsigned int m68k_symbol_EmuTrap_SaveFireflies_val = 0x00FF308D;
static const unsigned int m68k_symbol_EmuTrap_GetLanguage_val = 0x00FF308E;
static const unsigned int m68k_symbol_EmuTrap_GetControllerType_val = 0x00FF308F;
static const unsigned int m68k_symbol_EmuTrap_CollectScroll_val = 0x00FF3090;
static const unsigned int m68k_symbol_CheckpointSerialiseMemBlock_val = 0x00FF2C08;
static const unsigned int m68k_symbol_CheckpointSerialiseBlockSize_val = 0x00000380;
static const unsigned int m68k_symbol_FireflyBitmaskSize_val = 0x0000003B;
static const unsigned int m68k_symbol_FireflyCollectedMask_val = 0x00FF3016;
static const unsigned int m68k_symbol_FireflyPickupCountAct_val = 0x00FF3010;
static const unsigned int m68k_symbol_FireflyPickupCountTotalUI_val = 0x00FF3012;
static const unsigned int m68k_symbol_FireflyPickupCountTotalSave_val = 0x00FF3014;
static const unsigned int m68k_symbol_TotalFireflyCount_val = 0x00000160;
static const unsigned int m68k_symbol_CurrentSavePassword_val = 0x00FF2BF6;
static const unsigned int m68k_symbol_LastSaveVersion_val = 0x00FF2C02;
static const unsigned int m68k_symbol_CurrentLevel_val = 0x00FF115C;
static const unsigned int m68k_symbol_LastSaveLevel_val = 0x00FF2BFE;
static const unsigned int m68k_symbol_Level_Index_val = 0x00000065;
static const unsigned int m68k_symbol_LevelList_val = 0x00026178;
static const unsigned int m68k_symbol_EmuTrap_ACH_L1A4_KILL_DJAKK_val = 0x00FF3091;
static const unsigned int m68k_symbol_EmuTrap_ACH_L2A3_KILL_DJAKK_val = 0x00FF3092;
static const unsigned int m68k_symbol_EmuTrap_ACH_SURVIVE_SCIRUS_val = 0x00FF3093;
static const unsigned int m68k_symbol_EmuTrap_ACH_SURVIVE_DJAKK_val = 0x00FF3094;
static const unsigned int m68k_symbol_EmuTrap_ACH_BORGUS_TWO_BOULDERS_val = 0x00FF3095;
static const unsigned int m68k_symbol_EmuTrap_ACH_KILL_SWARM_val = 0x00FF3096;
static const unsigned int m68k_symbol_EmuTrap_ACH_COMPLETE_GAME_val = 0x00FF3097;
static const unsigned int m68k_symbol_EmuTrap_ACH_COMPLETE_GAME_ALT_val = 0x00FF3098;
static const unsigned int m68k_symbol_EmuTrap_ACH_HOGG_WALL_val = 0x00FF3099;
static const unsigned int m68k_symbol_EmuTrap_ACH_ECHO_FIREFLY_val = 0x00FF309A;
static const unsigned int m68k_symbol_EmuTrap_ACH_CART_RACE_val = 0x00FF309B;
static const unsigned int m68k_symbol_EmuTrap_ACH_SURVIVE_WYRM_val = 0x00FF309C;
static const unsigned int m68k_symbol_EmuTrap_ACH_L1A3_SNEAK_DJAKK_val = 0x00FF309D;
static const unsigned int m68k_symbol_EmuTrap_ACH_KILL_ALL_DJUBBS_val = 0x00FF309E;
static const unsigned int m68k_symbol_Ach_Data_BoulderSmashCount_val = 0x00FF309F;
static const unsigned int m68k_symbol_GameLanguage_val = 0x00FF1130;
static const unsigned int m68k_symbol_GamepadType_val = 0x00FF1131;
static const unsigned int m68k_symbol_LANGUAGE_ENGLISH_val = 0x00000000;
static const unsigned int m68k_symbol_LANGUAGE_FRENCH_val = 0x00000001;
static const unsigned int m68k_symbol_LANGUAGE_ITALIAN_val = 0x00000002;
static const unsigned int m68k_symbol_LANGUAGE_GERMAN_val = 0x00000003;
static const unsigned int m68k_symbol_LANGUAGE_SPANISH_val = 0x00000004;
