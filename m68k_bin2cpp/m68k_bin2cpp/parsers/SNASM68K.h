// ============================================================
//   Matt Phillips (c) 2020 BIG EVIL CORPORATION
// ============================================================
//   http://www.bigevilcorporation.co.uk
// ============================================================
//   m68k_bin2cpp - Converts 68K bin/symbol files to
//   C++ binary block, and extracts known memory addresses
// ============================================================

#pragma once

#include <sstream>
#include <vector>
#include <map>

#include "Parser.h"
#include "Serialiser.h"

#define COFF_MACHINE_68000		0x150
#define COFF_SECTION_NAME_SIZE	8

//SNASM2 hard coded section idxs
#define COFF_SECTION_FILENAMES	0
#define COFF_SECTION_DBG_DATA	1
#define COFF_SECTION_ROM_DATA	2
#define COFF_SECTION_COUNT		3

//Section header flags
#define COFF_SECTION_FLAG_DUMMY	0x00000001
#define COFF_SECTION_FLAG_GROUP	0x00000004
#define COFF_SECTION_FLAG_TEXT	0x00000020
#define COFF_SECTION_FLAG_DATA	0x00000040
#define COFF_SECTION_FLAG_BSS	0x00000080
#define COFF_SECTION_FLAG_WRITE	0x80000000

namespace m68k
{
	class ParserSNASM68K : public BinaryParser, public SymbolParser
	{
	public:
		ParserSNASM68K();
		virtual ~ParserSNASM68K();

		virtual uint32_t ReadBinary(const std::vector<uint8_t>& data, std::vector<uint8_t>& binary);
		virtual bool ReadSymbols(const std::vector<uint8_t>& data);
		virtual uint32_t FindSymbolValue(const std::string& name);

		void Serialise(Serialiser& stream);
		void Dump(std::stringstream& stream);

		struct FileHeader
		{
			void Serialise(Serialiser& stream);
			void Dump(std::stringstream& stream);

			uint16_t machineType;
			uint16_t numSections;
			uint32_t timeDate;
			uint32_t symbolTableOffset;
			uint32_t numSymbols;
			uint16_t exHeaderSize;
			uint16_t flags;
		};

		struct ExecutableHeader
		{
			void Serialise(Serialiser& stream);
			void Dump(std::stringstream& stream);

			uint16_t exHeaderMagic;
			uint16_t exHeaderVersion;
			uint32_t textDataSize;
			uint32_t initialisedDataSize;
			uint32_t uninitialisedDataSize;
			uint32_t entryPointAddr;
			uint32_t textDataAddr;
			uint32_t dataAddr;
		};

		struct SectionHeader
		{
			SectionHeader()
			{
				data = NULL;
			}

			void Serialise(Serialiser& stream);
			void Dump(std::stringstream& stream);

			std::string name;
			uint32_t physicalAddr;
			uint32_t virtualAddr;
			uint32_t size;
			uint32_t sectiondataOffset;
			uint32_t relocationTableOffset;
			uint32_t lineNumberTableOffset;
			uint16_t numRelocationEntries;
			uint16_t numLineNumberTableEntries;
			uint32_t flags;

			uint8_t* data;
		};

		struct Symbol
		{
			void Serialise(Serialiser& stream);
			bool operator < (const Symbol& rhs) const { return value < rhs.value; }

			std::string name;
			uint32_t stringTableOffset;
			uint32_t value;
			int16_t sectionIndex;
			uint16_t symbolType;
			int8_t storageClass;
			int8_t auxCount;
		};

		union SymbolNameStringDef
		{
			char name[COFF_SECTION_NAME_SIZE + 1];

			struct
			{
				uint32_t freeStringSpace;
				uint32_t stringTableOffset;
			};
		};

		struct LineNumberEntry
		{
			LineNumberEntry()
			{
				physicalAddress = 0;
				sectionMarker = 0;
				lineNumberSectionIdx = 0;
				filename = NULL;
			}

			void Serialise(Serialiser& stream);

			union
			{
				uint32_t physicalAddress;
				uint32_t filenameIndex;
			};

			union
			{
				int16_t sectionMarker;
				int16_t lineNumber;
			};

			uint32_t lineNumberSectionIdx;
			std::string* filename;
		};

		FileHeader m_fileHeader;
		ExecutableHeader m_executableHeader;
		std::vector<SectionHeader> m_sectionHeaders;
		std::vector<LineNumberEntry> m_lineNumberSectionHeaders;
		std::map<uint32_t, LineNumberEntry> m_lineNumberAddressMap;
		std::vector<Symbol> m_symbols;
		std::vector<Symbol> m_sortedSymbols;
		std::vector<std::string> m_filenameTable;
		uint8_t* m_stringTableRaw;
	};
}