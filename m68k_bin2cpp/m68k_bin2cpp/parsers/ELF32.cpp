// ============================================================
//   Matt Phillips (c) 2020 BIG EVIL CORPORATION
// ============================================================
//   http://www.bigevilcorporation.co.uk
// ============================================================
//   m68k_bin2cpp - Converts 68K bin/symbol files to
//   C++ binary block, and extracts known memory addresses
// ============================================================

#include "ELF32.h"

namespace m68k
{
	const std::string ParserELF32::ELFHeader::elfMagic = "\177ELF";

	ParserELF32::ParserELF32()
	{
		m_sectionStringTableOffset = 0;
		m_symbolsStringTableOffset = 0;
	}

	ParserELF32::~ParserELF32()
	{

	}

	uint32_t ParserELF32::ReadBinary(const std::vector<uint8_t>& data, std::vector<uint8_t>& binary)
	{
		Serialiser stream(data.data(), Serialiser::Endian::Big);
		Serialise(stream);

		//Read executable
		for(int i = 0; i < m_exeSections.size(); i++)
		{
			uint32_t originalSize = binary.size();
			stream.Seek(m_exeSections[i].sectionOffset);
			binary.resize(originalSize + m_exeSections[i].sectionSize);
			stream.Serialise(binary.data() + originalSize, m_exeSections[i].sectionSize);
		}

		return binary.size();
	}

	bool ParserELF32::ReadSymbols(const std::vector<uint8_t>& data)
	{
		Serialiser stream(data.data(), Serialiser::Endian::Big);
		Serialise(stream);
		return m_symbols.size() > 0;
	}

	uint32_t ParserELF32::FindSymbolValue(const std::string& name)
	{
		unsigned int value = 0xFFFFFFFF;

		for (int i = 0; i < m_symbols.size() && value == 0xFFFFFFFF; i++)
		{
			if (stricmp(m_symbols[i].name, name))
			{
				value = m_symbols[i].value;
			}
		}

		return value;
	}

	void ParserELF32::Serialise(Serialiser& stream)
	{
		//Read header
		stream.Serialise(m_elfHeader);

		if (memcmp(m_elfHeader.ident, &ELFHeader::elfMagic[0], ELFHeader::elfMagic.size()) == 0)
		{
			//Read all section headers
			stream.Seek(m_elfHeader.sectionHeaderOffset);
			for (int i = 0; i < m_elfHeader.sectionHeaderEntryCount; i++)
			{
				SectionHeader sectionHeader;
				sectionHeader.Serialise(stream);
				m_sectionHeaders.push_back(sectionHeader);
			}

			//Find section name string table
			if (m_elfHeader.stringTableSectionIdx > 0)
			{
				m_sectionStringTableOffset = m_sectionHeaders[m_elfHeader.stringTableSectionIdx].sectionOffset;

				//Read all section names
				for (int i = 0; i < m_sectionHeaders.size(); i++)
				{
					SerialiseString(stream, m_sectionStringTableOffset, m_sectionHeaders[i].nameStringOffset, m_sectionHeaders[i].name);
				}
			}

			//Read all symbol sections, find symbol string table and executable sections
			for (int i = 0; i < m_sectionHeaders.size(); i++)
			{
				switch (m_sectionHeaders[i].sectionType)
				{
					case SectionType::SymbolTable:
					case SectionType::DynamicSymbolTable:
					{
						stream.Seek(m_sectionHeaders[i].sectionOffset);

						for (int j = 0; j < m_sectionHeaders[i].sectionSize / m_sectionHeaders[i].entrySize; j++)
						{
							ELFSymbol symbol;
							symbol.Serialise(stream);
							m_symbols.push_back(symbol);
						}

						break;
					}

					case SectionType::StringTable:
					{
						m_symbolsStringTableOffset = m_sectionHeaders[i].sectionOffset;
						break;
					}

					case SectionType::ProgramBits:
					{
						m_exeSections.push_back(m_sectionHeaders[i]);
						break;
					}
				}
			}

			//Read all symbol names
			if (m_symbolsStringTableOffset > 0)
			{
				for (int i = 0; i < m_symbols.size(); i++)
				{
					SerialiseString(stream, m_symbolsStringTableOffset, m_symbols[i].nameStringOffset, m_symbols[i].name);
				}
			}
		}
	}

	void ParserELF32::SerialiseString(Serialiser& stream, uint32_t tableOffset, uint32_t stringOffset, std::string& string)
	{
		if (stringOffset > 0)
		{
			stream.Seek((int64_t)tableOffset + stringOffset);
			char str[s_maxStringSize] = { 0 };
			int index = 0;

			do
			{
				stream.Serialise((uint8_t&)str[index]);
			} while (str[index++] != 0);

			string = str;
		}
	}

	void ParserELF32::ELFHeader::Serialise(Serialiser& stream)
	{
		stream.Serialise(ident, identSize);
		stream.Serialise(fileType);
		stream.Serialise(machineType);
		stream.Serialise(objectFileVersion);
		stream.Serialise(entryPoint);
		stream.Serialise(programHeaderOffset);
		stream.Serialise(sectionHeaderOffset);
		stream.Serialise(processorFlags);
		stream.Serialise(headerSize);
		stream.Serialise(programHeaderEntrySize);
		stream.Serialise(programHeaderEntryCount);
		stream.Serialise(sectionHeaderEntrySize);
		stream.Serialise(sectionHeaderEntryCount);
		stream.Serialise(stringTableSectionIdx);
	}

	void ParserELF32::SectionHeader::Serialise(Serialiser& stream)
	{
		stream.Serialise(nameStringOffset);
		stream.Serialise(sectionType);
		stream.Serialise(flags);
		stream.Serialise(imageAddress);
		stream.Serialise(sectionOffset);
		stream.Serialise(sectionSize);
		stream.Serialise(linkTableIndex);
		stream.Serialise(info);
		stream.Serialise(addressAlignment);
		stream.Serialise(entrySize);
	}

	void ParserELF32::ELFSymbol::Serialise(Serialiser& stream)
	{
		stream.Serialise(nameStringOffset);
		stream.Serialise(value);
		stream.Serialise(size);
		stream.Serialise(info);
		stream.Serialise(visibility);
		stream.Serialise(sectionIndex);
	}
}