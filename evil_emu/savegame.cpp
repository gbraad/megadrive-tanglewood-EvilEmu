#include "savegame.h"
#include <ion/core/io/File.h>
#include <ion/engine/Engine.h>

SaveManager::SaveManager(ion::io::FileSystem& filesystem)
	: m_filesystem(filesystem)
{

}

void SaveManager::CreateUserDirectory()
{
	std::string directory = m_filesystem.GetUserDataDirectory() + "/" + EVIL_EMU_SAVE_DIRECTORY;
	std::string filename = directory + "/" + EVIL_EMU_SAVE_FILE;

	if (!ion::io::FileDevice::GetDefault()->GetDirectoryExists(directory))
	{
		ion::io::FileDevice::GetDefault()->CreateDirectory(directory);
	}
}

bool SaveManager::LoadSettings(Settings& settings)
{
	std::string filename = m_filesystem.GetUserDataDirectory() + "/" + EVIL_EMU_SAVE_DIRECTORY + "/" + EVIL_EMU_SETTINGS_FILE;

	ion::io::File file;
	if (file.Open(filename, ion::io::File::OpenMode::Read))
	{
		ion::io::Archive archive(file, ion::io::Archive::Direction::In);
		archive.Serialise(settings, "settings");
		file.Close();
		return true;
	}

	return false;
}

void SaveManager::SaveSettings(Settings& settings)
{
	CreateUserDirectory();

	std::string filename = m_filesystem.GetUserDataDirectory() + "/" + EVIL_EMU_SAVE_DIRECTORY + "/" + EVIL_EMU_SETTINGS_FILE;

	ion::io::File file;
	if (file.Open(filename, ion::io::File::OpenMode::Write))
	{
		ion::io::Archive archive(file, ion::io::Archive::Direction::Out);
		archive.Serialise(settings, "settings");
		file.Close();
	}
}

void SaveManager::InitSave(ion::services::User& user, OnInitCompleted const& onInitCompleted)
{
#if defined ION_PLATFORM_SWITCH
	//Async init
	ion::engine.services.saveManager->InitialiseSaveStorage(user, EVIL_EMU_SAVE_FILE,
		[onInitCompleted](ion::services::SaveManager::InitResult result)
		{
			onInitCompleted(result == ion::services::SaveManager::InitResult::Success);
		});
#else
	onInitCompleted(true);
#endif
}

void SaveManager::LoadGame(ion::services::User& user, OnLoadCompleted const& onLoadCompleted)
{
#if defined ION_PLATFORM_SWITCH
	//Async load
	ion::engine.services.saveManager->RequestLoad(user, EVIL_EMU_SAVE_FILE,
		[onLoadCompleted](ion::services::SaveManager::LoadResult result, ion::io::MemoryStream& data)
		{
			Save saveGame;

			if (result == ion::services::SaveManager::LoadResult::Success)
			{
				//Deserialise data
				ion::io::Archive archive(data, ion::io::Archive::Direction::In);
				archive.Serialise(saveGame.m_saveSlots, "saveSlots");
			}

			onLoadCompleted(saveGame, (result == ion::services::SaveManager::LoadResult::Success));
		});
#else
	//Legacy filesystem implementation
	std::string filename = m_filesystem.GetUserDataDirectory() + "/" + EVIL_EMU_SAVE_DIRECTORY + "/" + EVIL_EMU_SAVE_FILE;

	Save saveGame;

	ion::io::File file;
	if (file.Open(filename, ion::io::File::OpenMode::Read))
	{
		ion::io::Archive archive(file, ion::io::Archive::Direction::In);
		archive.Serialise(saveGame.m_saveSlots, "saveSlots");
		file.Close();
	}

	onLoadCompleted(saveGame, true);
#endif
}

void SaveManager::SaveGame(ion::services::User& user, Save& save, OnSaveCompleted const& onSaveCompleted)
{
#if defined ION_PLATFORM_SWITCH
	//Serialise
	ion::io::MemoryStream dataStream;
	ion::io::Archive archive(dataStream, ion::io::Archive::Direction::Out);
	archive.Serialise(save.m_saveSlots, "saveSlots");

	//Async save
	ion::engine.services.saveManager->RequestSave(user, EVIL_EMU_SAVE_FILE, dataStream,
		[onSaveCompleted](ion::services::SaveManager::SaveResult result)
		{
			onSaveCompleted(result == ion::services::SaveManager::SaveResult::Success);
		});
#else
	//Legacy filesystem implementation
	CreateUserDirectory();

	std::string filename = m_filesystem.GetUserDataDirectory() + "/" + EVIL_EMU_SAVE_DIRECTORY + "/" + EVIL_EMU_SAVE_FILE;

	ion::io::File file;
	if (file.Open(filename, ion::io::File::OpenMode::Write))
	{
		ion::io::Archive archive(file, ion::io::Archive::Direction::Out);
		archive.Serialise(save.m_saveSlots, "saveSlots");
		file.Close();
	}

	onSaveCompleted(true);
#endif
}