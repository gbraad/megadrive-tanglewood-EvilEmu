#include "constants.h"

#if EVIL_EMU_ACHEVEMENTS

#include <ion/online/Achievements.h>

#include "memwatch.h"

class Achievements
{
public:

	Achievements();
	~Achievements();

	void RegisterWatchers(MemWatcher& memWatcher);

	void Update();

	enum AchievementType
	{
		ACH_COLLECT_50_FIREFLIES,
		ACH_COLLECT_100_FIREFLIES,
		ACH_COLLECT_ALL_FIREFLIES,
		ACH_L1A4_KILL_DJAKK,
		ACH_L2A3_KILL_DJAKK,
		ACH_SURVIVE_SCIRUS,
		ACH_SURVIVE_DJAKK,
		ACH_BORGUS_TWO_BOULDERS,
		ACH_KILL_SWARM,
		ACH_COMPLETE_GAME,
		ACH_COMPLETE_GAME_ALT,
		ACH_L1A3_SNEAK_DJAKK,
		ACH_HOGG_WALL,
		ACH_ECHO_FIREFLY,
		ACH_CART_RACE,
		ACH_SURVIVE_WYRM,
		ACH_KILL_ALL_DJUBBS,

		ACH_COUNT
	};

	static ion::online::Achievement s_achievements[ACH_COUNT];

	void AchievementWatch_Fireflies(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_L1A4_KILL_DJAKK(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_L2A3_KILL_DJAKK(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_SURVIVE_SCIRUS(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_SURVIVE_DJAKK(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_BORGUS_TWO_BOULDERS(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_KILL_SWARM(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_COMPLETE_GAME(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_COMPLETE_GAME_ALT(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_L1A3_SNEAK_DJAKK(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_HOGG_WALL(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_ECHO_FIREFLY(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_CART_RACE(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_SURVIVE_WYRM(u32 watchAddress, int watchSize, u32 watchValue);
	void AchievementWatch_ACH_KILL_ALL_DJUBBS(u32 watchAddress, int watchSize, u32 watchValue);

private:
	ion::online::Achievements* m_achievementsSystem;
};

#endif
