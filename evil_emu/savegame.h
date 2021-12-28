#pragma once

#include <ion/core/time/Time.h>
#include <ion/core/io/FileSystem.h>
#include <ion/core/io/Archive.h>
#include <ion/services/User.h>

#include "settings.h"
#include "constants.h"

#include <functional>

struct Save
{
	struct SaveSlot
	{
		SaveSlot()
		{
			gameType = EVIL_EMU_GAME_TYPE;
			password = 0;
			levelIdx = 0;
			levelAddr = 0;
			firefliesAct = 0;
			firefliesGame = 0;
			saveVersion = 0;
		}

		bool operator == (const SaveSlot& rhs) const
		{
			return gameType == rhs.gameType
				&& password == rhs.password
				&& levelIdx == rhs.levelIdx
				&& firefliesAct == rhs.firefliesAct
				&& firefliesGame == rhs.firefliesGame
				&& saveVersion == rhs.saveVersion
				&& fireflyData == rhs.fireflyData;
		}

		void Serialise(ion::io::Archive& archive)
		{
			archive.Serialise(gameType, "gameType");
			archive.Serialise(password, "password");
			archive.Serialise(levelIdx, "levelIdx");
			archive.Serialise(levelAddr, "levelAddr");
			archive.Serialise(firefliesAct, "firefliesAct");
			archive.Serialise(firefliesGame, "firefliesGame");
			archive.Serialise(saveVersion, "saveVersion");
			archive.Serialise(timeStamp.time, "timeStamp");
			archive.Serialise(serialiseData, "serialiseData");
			archive.Serialise(fireflyData, "fireflyData");
		}

		u32 gameType;
		u32 password;
		u8 levelIdx;
		u32 levelAddr;
		u16 firefliesAct;
		u16 firefliesGame;
		u16 saveVersion;
		ion::time::TimeStamp timeStamp;
		std::vector<u8> serialiseData;
		std::vector<u8> fireflyData;
	};

	std::vector<SaveSlot> m_saveSlots;
};

class SaveManager
{
public:
	typedef std::function<void(bool success)> OnInitCompleted;
	typedef std::function<void(bool success)> OnSaveCompleted;
	typedef std::function<void(Save& saveData, bool success)> OnLoadCompleted;

	SaveManager(ion::io::FileSystem& filesystem);

	bool LoadSettings(Settings& settings);
	void SaveSettings(Settings& settings);

	void InitSave(ion::services::User& user, OnInitCompleted const& onInitCompleted);
	void LoadGame(ion::services::User& user, OnLoadCompleted const& onLoadCompleted);
	void SaveGame(ion::services::User& user, Save& saveGame, OnSaveCompleted const& onSaveCompleted);

private:
	void CreateUserDirectory();

	ion::io::FileSystem& m_filesystem;
};