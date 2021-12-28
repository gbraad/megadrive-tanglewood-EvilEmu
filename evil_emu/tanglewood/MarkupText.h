#pragma once

#include <ion/core/io/Archive.h>

struct TextFile
{
	static void RegisterSerialiseType(ion::io::Archive& archive)
	{
		archive.RegisterPointerType<TextFile>("TextFile");
	}

	void Serialise(ion::io::Archive& archive)
	{
		if (archive.GetDirection() == ion::io::Archive::Direction::In)
		{
			text.resize(archive.GetStreamSize());
			archive.Serialise(&text[0], archive.GetStreamSize());
		}
	}

	std::string text;
};