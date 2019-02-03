#pragma once

#include <ion/core/time/Time.h>
#include <ion/io/FileSystem.h>
#include <ion/io/Archive.h>

#include "settings.h"
#include "constants.h"

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
				&& saveVersion == rhs.saveVersion;
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
	};

	std::vector<SaveSlot> m_saveSlots;
};

class SaveManager
{
public:
	SaveManager(ion::io::FileSystem& filesystem);

	bool LoadSettings(Settings& settings);
	void SaveSettings(Settings& settings);

	bool LoadGame(Save& saveGame);
	void SaveGame(Save& saveGame);

private:
	void CreateUserDirectory();

	ion::io::FileSystem& m_filesystem;
};