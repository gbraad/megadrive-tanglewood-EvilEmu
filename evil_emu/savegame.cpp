#include "savegame.h"

SaveManager::SaveManager(ion::io::FileSystem& filesystem)
	: m_filesystem(filesystem)
{

}

void SaveManager::CreateUserDirectory()
{
	std::string directory = m_filesystem.GetUserDataDirectory() + "/" + EVIL_EMU_SAVE_DIRECTORY;
	std::string filename = directory + "/" + EVIL_EMU_SAVE_FILE;

	if (!m_filesystem.GetDefaultFileDevice()->GetDirectoryExists(directory))
	{
		m_filesystem.GetDefaultFileDevice()->CreateDirectory(directory);
	}
}

bool SaveManager::LoadSettings(Settings& settings)
{
	std::string filename = m_filesystem.GetUserDataDirectory() + "/" + EVIL_EMU_SAVE_DIRECTORY + "/" + EVIL_EMU_SETTINGS_FILE;

	ion::io::File file;
	if (file.Open(filename, ion::io::File::eOpenRead))
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
	if (file.Open(filename, ion::io::File::eOpenWrite))
	{
		ion::io::Archive archive(file, ion::io::Archive::Direction::Out);
		archive.Serialise(settings, "settings");
		file.Close();
	}
}

bool SaveManager::LoadGame(Save& save)
{
	std::string filename = m_filesystem.GetUserDataDirectory() + "/" + EVIL_EMU_SAVE_DIRECTORY + "/" + EVIL_EMU_SAVE_FILE;

	ion::io::File file;
	if (file.Open(filename, ion::io::File::eOpenRead))
	{
		ion::io::Archive archive(file, ion::io::Archive::Direction::In);
		archive.Serialise(save.m_saveSlots, "saveSlots");
		file.Close();
		return true;
	}

	return false;
}

void SaveManager::SaveGame(Save& save)
{
	CreateUserDirectory();

	std::string filename = m_filesystem.GetUserDataDirectory() + "/" + EVIL_EMU_SAVE_DIRECTORY + "/" + EVIL_EMU_SAVE_FILE;

	ion::io::File file;
	if (file.Open(filename, ion::io::File::eOpenWrite))
	{
		ion::io::Archive archive(file, ion::io::Archive::Direction::Out);
		archive.Serialise(save.m_saveSlots, "saveSlots");
		file.Close();
	}
}