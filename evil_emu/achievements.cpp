#include "constants.h"

#if EVIL_EMU_ACHEVEMENTS

#include "achievements.h"
#include "roms/include_vars.h"

ion::online::Achievement Achievements::s_achievements[ACH_COUNT] = 
{
	{ ACH_COLLECT_50_FIREFLIES,		"ACH_COLLECT_50_FIREFLIES",		"Moon Light",				"Collect 50 fireflies" },
	{ ACH_COLLECT_100_FIREFLIES,	"ACH_COLLECT_100_FIREFLIES",	"Night Light",				"Collect 100 fireflies" },
	{ ACH_COLLECT_ALL_FIREFLIES,	"ACH_COLLECT_ALL_FIREFLIES",	"Guiding Light",			"Collect all fireflies" },
	{ ACH_L1A4_KILL_DJAKK,			"ACH_L1A4_KILL_DJAKK",			"I trusted you...",			"Kill the Djakk in Chapter 1, Act 4" },
	{ ACH_L2A3_KILL_DJAKK,			"ACH_L2A3_KILL_DJAKK",			"Take No Prisoners",		"Kill the Djakk in Chapter 2, Act 3" },
	{ ACH_SURVIVE_SCIRUS,			"ACH_SURVIVE_SCIRUS",			"Cool Off",					"Survive an encounter with an angry Scirus" },
	{ ACH_SURVIVE_DJAKK,			"ACH_SURVIVE_DJAKK",			"By the Skin of your Teeth","Evade a Djakk's bite" },
	{ ACH_BORGUS_TWO_BOULDERS,		"ACH_BORGUS_TWO_BOULDERS",		"Two Stones, One Bird",		"Kill Borgus with only two boulder drops" },
	{ ACH_KILL_SWARM,				"ACH_KILL_SWARM",				"Not so Tough Now",			"Kill a Demon Swarm with the Lightning ability" },
	{ ACH_COMPLETE_GAME,			"ACH_COMPLETE_GAME",			"Home...",					"Complete the game" },
	{ ACH_COMPLETE_GAME_ALT,		"ACH_COMPLETE_GAME_ALT",		"...Sweet Home.",			"Complete the game with all fireflies" },
	{ ACH_L1A3_SNEAK_DJAKK,			"ACH_L1A3_SNEAK_DJAKK",			"In and Out Like the Wind",	"Sneak past the Djakk in Chapter 1, Act 3" },
	{ ACH_HOGG_WALL,				"ACH_HOGG_WALL",				"Headache",					"Coerce a Hogg into getting stuck in three walls" },
	{ ACH_ECHO_FIREFLY,				"ACH_ECHO_FIREFLY",				"Teamwork",					"Allow Echo to collect a firefly" },
	{ ACH_CART_RACE,				"ACH_CART_RACE",				"Pole Position",			"Beat Echo in a mine cart race" },
	{ ACH_SURVIVE_WYRM,				"ACH_SURVIVE_WYRM",				"Exo-nope",					"Evade a Wyrm attack" },
	{ ACH_KILL_ALL_DJUBBS,			"ACH_KILL_ALL_DJUBBS",			"So long, and thanks for all the fish",	"Kill all Djubb fish in Chapter 8, Act 1" },
};

Achievements::Achievements()
{
	m_achievementsSystem = ion::online::Achievements::Create();

	for (int i = 0; i < ACH_COUNT; i++)
	{
		m_achievementsSystem->RegisterAchievement(s_achievements[i]);
	}
}

Achievements::~Achievements()
{
	delete m_achievementsSystem;
}

void Achievements::RegisterWatchers(MemWatcher& memWatcher)
{
	memWatcher.AddAddress(snasm68k_symbol_FireflyPickupCountTotalUI_val,		2, std::bind(&Achievements::AchievementWatch_Fireflies, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_L1A4_KILL_DJAKK_val,		1, std::bind(&Achievements::AchievementWatch_ACH_L1A4_KILL_DJAKK, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_L2A3_KILL_DJAKK_val,		1, std::bind(&Achievements::AchievementWatch_ACH_L2A3_KILL_DJAKK, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_SURVIVE_SCIRUS_val,		1, std::bind(&Achievements::AchievementWatch_ACH_SURVIVE_SCIRUS, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_SURVIVE_DJAKK_val,		1, std::bind(&Achievements::AchievementWatch_ACH_SURVIVE_DJAKK, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_BORGUS_TWO_BOULDERS_val,	1, std::bind(&Achievements::AchievementWatch_ACH_BORGUS_TWO_BOULDERS, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_KILL_SWARM_val,			1, std::bind(&Achievements::AchievementWatch_ACH_KILL_SWARM, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_COMPLETE_GAME_val,		1, std::bind(&Achievements::AchievementWatch_ACH_COMPLETE_GAME, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_COMPLETE_GAME_ALT_val,	1, std::bind(&Achievements::AchievementWatch_ACH_COMPLETE_GAME_ALT, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_L1A3_SNEAK_DJAKK_val,		1, std::bind(&Achievements::AchievementWatch_ACH_L1A3_SNEAK_DJAKK, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_2));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_HOGG_WALL_val,			1, std::bind(&Achievements::AchievementWatch_ACH_HOGG_WALL, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_ECHO_FIREFLY_val,			1, std::bind(&Achievements::AchievementWatch_ACH_ECHO_FIREFLY, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_CART_RACE_val,			1, std::bind(&Achievements::AchievementWatch_ACH_CART_RACE, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_SURVIVE_WYRM_val,			1, std::bind(&Achievements::AchievementWatch_ACH_SURVIVE_WYRM, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	memWatcher.AddAddress(snasm68k_symbol_EmuTrap_ACH_KILL_ALL_DJUBBS_val,		1, std::bind(&Achievements::AchievementWatch_ACH_KILL_ALL_DJUBBS, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void Achievements::Update()
{
	m_achievementsSystem->Update();
}

void Achievements::AchievementWatch_Fireflies(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue >= 50)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_COLLECT_50_FIREFLIES]);
	}

	if (watchValue >= 100)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_COLLECT_100_FIREFLIES]);
	}

	if (watchValue == snasm68k_symbol_TotalFireflyCount_val)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_COLLECT_ALL_FIREFLIES]);
	}
}

void Achievements::AchievementWatch_ACH_L1A4_KILL_DJAKK(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_L1A4_KILL_DJAKK]);
	}
}

void Achievements::AchievementWatch_ACH_L2A3_KILL_DJAKK(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_L2A3_KILL_DJAKK]);
	}
}

void Achievements::AchievementWatch_ACH_SURVIVE_SCIRUS(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_SURVIVE_SCIRUS]);
	}
}

void Achievements::AchievementWatch_ACH_SURVIVE_DJAKK(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_SURVIVE_DJAKK]);
	}
}

void Achievements::AchievementWatch_ACH_BORGUS_TWO_BOULDERS(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_BORGUS_TWO_BOULDERS]);
	}
}

void Achievements::AchievementWatch_ACH_KILL_SWARM(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_KILL_SWARM]);
	}
}

void Achievements::AchievementWatch_ACH_COMPLETE_GAME(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_COMPLETE_GAME]);
	}
}

void Achievements::AchievementWatch_ACH_COMPLETE_GAME_ALT(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_COMPLETE_GAME_ALT]);
	}
}

void Achievements::AchievementWatch_ACH_L1A3_SNEAK_DJAKK(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_L1A3_SNEAK_DJAKK]);
	}
}

void Achievements::AchievementWatch_ACH_HOGG_WALL(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_HOGG_WALL]);
	}
}

void Achievements::AchievementWatch_ACH_ECHO_FIREFLY(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_ECHO_FIREFLY]);
	}
}

void Achievements::AchievementWatch_ACH_CART_RACE(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_CART_RACE]);
	}
}

void Achievements::AchievementWatch_ACH_SURVIVE_WYRM(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_SURVIVE_WYRM]);
	}
}

void Achievements::AchievementWatch_ACH_KILL_ALL_DJUBBS(u32 watchAddress, int watchSize, u32 watchValue)
{
	if (watchValue == 1)
	{
		m_achievementsSystem->AwardAchievement(s_achievements[ACH_KILL_ALL_DJUBBS]);
	}
}

#endif
