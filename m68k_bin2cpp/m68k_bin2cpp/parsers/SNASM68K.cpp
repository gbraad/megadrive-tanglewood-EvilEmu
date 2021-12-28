// ============================================================
//   Matt Phillips (c) 2020 BIG EVIL CORPORATION
// ============================================================
//   http://www.bigevilcorporation.co.uk
// ============================================================
//   m68k_bin2cpp - Converts 68K bin/symbol files to
//   C++ binary block, and extracts known memory addresses
// ============================================================

#include <iostream>
#include <algorithm>

#include "SNASM68K.h"

namespace m68k
{
	ParserSNASM68K::ParserSNASM68K()
	{
		m_stringTableRaw = NULL;
	}

	ParserSNASM68K::~ParserSNASM68K()
	{
		for (int i = 0; i < m_fileHeader.numSections; i++)
		{
			if (m_sectionHeaders[i].data)
			{
				delete[] m_sectionHeaders[i].data;
			}
		}

		if (m_stringTableRaw)
		{
			delete[] m_stringTableRaw;
		}
	}

	uint32_t ParserSNASM68K::ReadBinary(const std::vector<uint8_t>& data, std::vector<uint8_t>& binary)
	{
		Serialiser stream(data.data());
		Serialise(stream);
		
		uint8_t* romData = m_sectionHeaders[COFF_SECTION_ROM_DATA].data;
		uint32_t romSize = m_sectionHeaders[COFF_SECTION_ROM_DATA].size;

		if (romSize > 0)
		{
			binary.resize(romSize);
			memcpy(binary.data(), romData, romSize);
		}

		return romSize;
	}

	bool ParserSNASM68K::ReadSymbols(const std::vector<uint8_t>& data)
	{
		Serialiser stream(data.data());
		Serialise(stream);
		return !m_symbols.empty();
	}

	uint32_t ParserSNASM68K::FindSymbolValue(const std::string& name)
	{
		unsigned int value = 0xFFFFFFFF;

		for (int i = 0; i < m_sortedSymbols.size() && value == 0xFFFFFFFF; i++)
		{
			if (stricmp(m_sortedSymbols[i].name, name))
			{
				value = m_sortedSymbols[i].value;
			}
		}

		return value;
	}

	void ParserSNASM68K::Serialise(Serialiser& stream)
	{
		//Serialise file header
		stream.Serialise(m_fileHeader);

		//Serialise executable header
		if (m_fileHeader.exHeaderSize > 0)
		{
			stream.Serialise(m_executableHeader);
		}

		//Allocate section headers
		m_sectionHeaders.resize(m_fileHeader.numSections);

		//Serialise section headers
		for (int i = 0; i < m_fileHeader.numSections; i++)
		{
			stream.Serialise(m_sectionHeaders[i]);
		}

		//Allocate symbol table
		m_symbols.resize(m_fileHeader.numSymbols);

		//Seek to symbol table
		stream.Seek(m_fileHeader.symbolTableOffset);

		//Serialise symbol table
		for (int i = 0; i < m_fileHeader.numSymbols; i++)
		{
			stream.Serialise(m_symbols[i]);
		}

		//Serialise string table size
		uint32_t stringTableSizeBytes;
		stream.Serialise(stringTableSizeBytes);

		//Allocate string table
		m_stringTableRaw = new uint8_t[stringTableSizeBytes];

		//Serialise string table
		stream.Serialise(m_stringTableRaw, stringTableSizeBytes);

		//Resolve symbol strings
		for (int i = 0; i < m_fileHeader.numSymbols; i++)
		{
			if (m_symbols[i].stringTableOffset != -1)
			{
				m_symbols[i].name = (char*)m_stringTableRaw + (m_symbols[i].stringTableOffset - sizeof(uint32_t));
			}
		}

		//Copy to sorted table
		m_sortedSymbols = m_symbols;
		std::sort(m_sortedSymbols.begin(), m_sortedSymbols.end());

		//Read section data
		for (int i = 0; i < m_fileHeader.numSections; i++)
		{
			if (m_sectionHeaders[i].sectiondataOffset > 0 && m_sectionHeaders[i].size > 0)
			{
				//Alloc data
				m_sectionHeaders[i].data = new uint8_t[m_sectionHeaders[i].size];

				//Seek to data start
				stream.Seek(m_sectionHeaders[i].sectiondataOffset);

				//Read data
				stream.Serialise(m_sectionHeaders[i].data, m_sectionHeaders[i].size);
			}
		}

		//Build filename table
		uint32_t lastStringPos = 0;
		const char* filenameData = (const char*)m_sectionHeaders[COFF_SECTION_FILENAMES].data;

		for (int i = 0; i < m_sectionHeaders[COFF_SECTION_FILENAMES].size; i++)
		{
			if (filenameData[i] == 0)
			{
				m_filenameTable.push_back((const char*)(filenameData + lastStringPos));
				lastStringPos = i + 1;
			}
		}

		//Serialise line number sections
		for (int i = 0; i < m_fileHeader.numSections; i++)
		{
			if (m_sectionHeaders[i].numLineNumberTableEntries > 0)
			{
				//Seek to line number section start
				stream.Seek(m_sectionHeaders[i].lineNumberTableOffset);

				//Read all entries
				for (int j = 0; j < m_sectionHeaders[i].numLineNumberTableEntries; j++)
				{
					//Serialise entry
					LineNumberEntry lineNumberEntry;
					stream.Serialise(lineNumberEntry);

					//If a line number section header
					if (lineNumberEntry.sectionMarker <= 0)
					{
						//Filename table is 1-based
						lineNumberEntry.filenameIndex -= 1;

						//Add new section
						m_lineNumberSectionHeaders.push_back(lineNumberEntry);
					}
					else
					{
						//Set line number section
						lineNumberEntry.lineNumberSectionIdx = m_lineNumberSectionHeaders.size() - 1;

						//Get filename
						LineNumberEntry& lineNumberSectionHeader = m_lineNumberSectionHeaders[lineNumberEntry.lineNumberSectionIdx];
						lineNumberEntry.filename = &m_filenameTable[lineNumberSectionHeader.filenameIndex];

						//Insert into address map
						m_lineNumberAddressMap[lineNumberEntry.physicalAddress] = lineNumberEntry;
					}
				}
			}
		}
	}

	void ParserSNASM68K::Dump(std::stringstream& stream)
	{
		m_fileHeader.Dump(stream);

		if (m_fileHeader.exHeaderSize > 0)
		{
			m_executableHeader.Dump(stream);
		}

		for (int i = 0; i < m_fileHeader.numSections; i++)
		{
			m_sectionHeaders[i].Dump(stream);
			stream << std::endl;
		}
	}

	void ParserSNASM68K::FileHeader::Serialise(Serialiser& stream)
	{
		stream.Serialise(machineType);
		stream.Serialise(numSections);
		stream.Serialise(timeDate);
		stream.Serialise(symbolTableOffset);
		stream.Serialise(numSymbols);
		stream.Serialise(exHeaderSize);
		stream.Serialise(flags);
	}

	void ParserSNASM68K::FileHeader::Dump(std::stringstream& stream)
	{
		stream << "-------------------------------------" << std::endl;
		stream << "HEADER" << std::endl;
		stream << "-------------------------------------" << std::endl;
		stream << "COFF machine type: 0x" << std::hex << machineType << std::dec << std::endl;
		stream << "Num sections: " << numSections << std::endl;
		stream << "Symbol table offset: " << symbolTableOffset << std::endl;
		stream << "Num symbols: " << numSymbols << std::endl;
		stream << "Executable header size: " << exHeaderSize << std::endl;
		stream << "Flags: 0x" << std::hex << flags << std::dec << std::endl;

		stream << std::endl;
	}

	void ParserSNASM68K::ExecutableHeader::Serialise(Serialiser& stream)
	{
		stream.Serialise(exHeaderMagic);
		stream.Serialise(exHeaderVersion);
		stream.Serialise(textDataSize);
		stream.Serialise(initialisedDataSize);
		stream.Serialise(uninitialisedDataSize);
		stream.Serialise(entryPointAddr);
		stream.Serialise(textDataAddr);
		stream.Serialise(dataAddr);
	}

	void ParserSNASM68K::ExecutableHeader::Dump(std::stringstream& stream)
	{
		stream << "-------------------------------------" << std::endl;
		stream << "EXECUTABLE HEADER" << std::endl;
		stream << "-------------------------------------" << std::endl;
		stream << "Magic: 0x" << std::hex << exHeaderMagic << std::dec << std::endl;
		stream << "Version: 0x" << std::hex << exHeaderVersion << std::dec << std::endl;
		stream << "Text data size: " << textDataSize << std::endl;
		stream << "Initialised data size: " << initialisedDataSize << std::endl;
		stream << "Uninitialised data size: " << uninitialisedDataSize << std::endl;
		stream << "Entry point address: 0x" << std::hex << entryPointAddr << std::dec << std::endl;
		stream << "Text data address: 0x" << std::hex << textDataAddr << std::dec << std::endl;
		stream << "Data address: 0x" << std::hex << dataAddr << std::dec << std::endl;

		stream << std::endl;
	}

	void ParserSNASM68K::SectionHeader::Serialise(Serialiser& stream)
	{
		stream.Serialise(name, COFF_SECTION_NAME_SIZE);
		stream.Serialise(physicalAddr);
		stream.Serialise(virtualAddr);
		stream.Serialise(size);
		stream.Serialise(sectiondataOffset);
		stream.Serialise(relocationTableOffset);
		stream.Serialise(lineNumberTableOffset);
		stream.Serialise(numRelocationEntries);
		stream.Serialise(numLineNumberTableEntries);
		stream.Serialise(flags);
	}

	void ParserSNASM68K::SectionHeader::Dump(std::stringstream& stream)
	{
		std::string flagsString;

		if (flags & COFF_SECTION_FLAG_DUMMY)
			flagsString += "DUMMY SECTION";
		if (flags & COFF_SECTION_FLAG_GROUP)
			flagsString += "GROUP SECTION";
		if (flags & COFF_SECTION_FLAG_TEXT)
			flagsString += "TEXT SECTION";
		if (flags & COFF_SECTION_FLAG_DATA)
			flagsString += "DATA SECTION";
		if (flags & COFF_SECTION_FLAG_BSS)
			flagsString += "BSS SECTION";
		if (flags & COFF_SECTION_FLAG_WRITE)
			flagsString += " + WRITEABLE";

		stream << "-------------------------------------" << std::endl;
		stream << "SECTION HEADER: " << name.c_str() << std::endl;
		stream << "-------------------------------------" << std::endl;
		stream << "Physical address: 0x" << std::hex << physicalAddr << std::dec << std::endl;
		stream << "Virtual address: 0x" << std::hex << virtualAddr << std::dec << std::endl;
		stream << "Size: " << size << std::endl;
		stream << "Section data offset: " << sectiondataOffset << std::endl;
		stream << "Relocation table offset: " << relocationTableOffset << std::endl;
		stream << "Line number table offset: " << lineNumberTableOffset << std::endl;
		stream << "Num relocation table entries: " << numRelocationEntries << std::endl;
		stream << "Num line number table entries: " << numLineNumberTableEntries << std::endl;
		stream << "Flags: 0x" << std::hex << flags << std::dec << " (" << flagsString.c_str() << ")" << std::endl;

		stream << std::endl;
	}

	void ParserSNASM68K::LineNumberEntry::Serialise(Serialiser& stream)
	{
		stream.Serialise(physicalAddress);
		stream.Serialise(lineNumber);
	}

	void ParserSNASM68K::Symbol::Serialise(Serialiser& stream)
	{
		SymbolNameStringDef symbolStringDef;
		stream.Serialise((uint8_t*)symbolStringDef.name, COFF_SECTION_NAME_SIZE);

		//NULL terminate string
		symbolStringDef.name[COFF_SECTION_NAME_SIZE] = 0;

		if (symbolStringDef.freeStringSpace == 0)
		{
			//Name doesn't fit, get string table offset
			stringTableOffset = symbolStringDef.stringTableOffset;
		}
		else
		{
			//Name fits here
			name = symbolStringDef.name;
			stringTableOffset = (uint32_t)-1;
		}

		stream.Serialise(value);
		stream.Serialise(sectionIndex);
		stream.Serialise(symbolType);
		stream.Serialise(storageClass);
		stream.Serialise(auxCount);
	}
}