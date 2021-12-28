// ============================================================
//   Matt Phillips (c) 2020 BIG EVIL CORPORATION
// ============================================================
//   http://www.bigevilcorporation.co.uk
// ============================================================
//   m68k_bin2cpp - Converts 68K bin/symbol files to
//   C++ binary block, and extracts known memory addresses
// ============================================================

#pragma once

#include <utility>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "Parser.h"
#include "Serialiser.h"

namespace m68k
{
	class SymbolParserASM68K : public SymbolParser
	{
	public:
		virtual bool ReadSymbols(const std::vector<uint8_t>& data);
		virtual uint32_t FindSymbolValue(const std::string& name);

	private:
		enum class ChunkId : unsigned char
		{
			Filename = 0x88,            // A filename with start address and line count
			Address = 0x80,             // An address of next line
			AddressWithCount = 0x82,    // An address with line count
			EndOfSection = 0x8A,        // End of section
			Equate = 0x01,              // Symbol table entry (equate)
			Symbol = 0x02               // Symbol table entry
		};

		struct AddressEntry
		{
			uint32_t address;
			int8_t flags;
			int32_t lineFrom;
			int32_t lineTo;
		};

		struct SymbolEntry
		{
			uint32_t address;
			std::string name;
		};

		struct FilenameSection
		{
			std::string filename;
			std::vector<AddressEntry> addresses;
		};

#pragma pack(push)
#pragma pack(1)
		struct FileHeader
		{
			uint32_t unknown1;
			uint32_t unknown2;

			void Serialise(Serialiser& serialiser)
			{
				serialiser.Serialise(unknown1);
				serialiser.Serialise(unknown2);
			}
		};

		struct FilenameHeader
		{
			int8_t firstLine;
			int8_t flags;
			int8_t unknown;
			int16_t length;

			void Serialise(Serialiser& serialiser)
			{
				serialiser.Serialise(firstLine);
				serialiser.Serialise(flags);
				serialiser.Serialise(unknown);
				serialiser.Serialise(length);
			}
		};

		struct ChunkHeader
		{
			uint32_t payload;
			ChunkId chunkId;

			void Serialise(Serialiser& serialiser)
			{
				serialiser.Serialise(payload);
				serialiser.Serialise((unsigned char&)chunkId);
			}
		};

		struct SymbolChunk
		{
			uint32_t address;
			int8_t flags;
			int8_t stringLen;
		};
#pragma pack(pop)

		std::vector<SymbolEntry> m_Symbols;
		std::vector<FilenameSection> m_Filenames;
		std::map<uint32_t, std::pair<std::string, int32_t>> m_Addr2FileLine;
		std::string m_AssembledFile;
	};
}
