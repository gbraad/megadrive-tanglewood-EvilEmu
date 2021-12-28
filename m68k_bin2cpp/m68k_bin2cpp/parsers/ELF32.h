// ============================================================
//   Matt Phillips (c) 2020 BIG EVIL CORPORATION
// ============================================================
//   http://www.bigevilcorporation.co.uk
// ============================================================
//   m68k_bin2cpp - Converts 68K bin/symbol files to
//   C++ binary block, and extracts known memory addresses
// ============================================================

#pragma once

#include "Parser.h"
#include "Serialiser.h"

namespace m68k
{
	class ParserELF32 : public BinaryParser, public SymbolParser
	{
	public:
		ParserELF32();
		virtual ~ParserELF32();

		virtual uint32_t ReadBinary(const std::vector<uint8_t>& data, std::vector<uint8_t>& binary);
		virtual bool ReadSymbols(const std::vector<uint8_t>& data);
		virtual uint32_t FindSymbolValue(const std::string& name);

	private:
		void Serialise(Serialiser& stream);
		void SerialiseString(Serialiser& stream, uint32_t tableOffset, uint32_t stringOffset, std::string& string);

		static const int s_maxStringSize = 1024;

		enum SectionType
		{
			Empty =					0,
			ProgramBits =			1,
			SymbolTable =			2,
			StringTable = 			3,
			RelocationWithAddends =	4,
			HashTable =				5,
			DynamicLinking =		6,
			Note =					7,
			NoBits =				8,
			Relocation =			9,
			Reserved =				10,
			DynamicSymbolTable =	11,
			LoProcessor =			0x70000000,
			HiProcessor =			0x7fffffff,
			LoUser =				0x80000000,
			HiUser =				0xffffffff,
		};

		struct ELFHeader
		{
			void Serialise(Serialiser& stream);

			static const int identSize = 16;
			static const std::string elfMagic;

			uint8_t ident[identSize];
			uint16_t fileType;
			uint16_t machineType;
			uint32_t objectFileVersion;
			uint32_t entryPoint;
			uint32_t programHeaderOffset;
			uint32_t sectionHeaderOffset;
			uint32_t processorFlags;
			uint16_t headerSize;
			uint16_t programHeaderEntrySize;
			uint16_t programHeaderEntryCount;
			uint16_t sectionHeaderEntrySize;
			uint16_t sectionHeaderEntryCount;
			uint16_t stringTableSectionIdx;
		};

		struct SectionHeader
		{
			void Serialise(Serialiser& stream);

			std::string name;
			uint32_t nameStringOffset;
			uint32_t sectionType;
			uint32_t flags;
			uint32_t imageAddress;
			uint32_t sectionOffset;
			uint32_t sectionSize;
			uint32_t linkTableIndex;
			uint32_t info;
			uint32_t addressAlignment;
			uint32_t entrySize;
		};

		struct ELFSymbol
		{
			void Serialise(Serialiser& stream);

			std::string name;
			uint32_t nameStringOffset;
			uint32_t value;
			uint32_t size;
			uint8_t info;
			uint8_t visibility;
			uint16_t sectionIndex;
		};

		ELFHeader m_elfHeader;
		std::vector<SectionHeader> m_sectionHeaders;
		std::vector<ELFSymbol> m_symbols;
		std::vector<SectionHeader> m_exeSections;
		uint32_t m_sectionStringTableOffset;
		uint32_t m_symbolsStringTableOffset;
	};
}