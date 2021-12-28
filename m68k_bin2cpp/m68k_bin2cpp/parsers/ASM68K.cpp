// ============================================================
//   Matt Phillips (c) 2019 BIG EVIL CORPORATION
// ============================================================
//   http://www.bigevilcorporation.co.uk
// ============================================================
//   asm68ksymboldump - An ASM68K.EXE symbol file reader
// ============================================================

#include "ASM68K.h"

namespace m68k
{
	bool SymbolParserASM68K::ReadSymbols(const std::vector<uint8_t>& data)
	{
		if (data.size() > 0)
		{
			Serialiser serialiser(data.data());

			FilenameHeader filenameHeader;
			ChunkHeader chunkHeader;
			FilenameSection filenameSection;
			AddressEntry addressEntry;
			SymbolEntry symbolEntry;
			std::string readString;

			int32_t bytesRead = 0;
			int32_t currentLine = 0;

			//Read file header
			FileHeader fileHeader;
			bytesRead += serialiser.Serialise(fileHeader);

			//Iterate over chunks
			while (bytesRead < data.size())
			{
				//Read chunk header
				bytesRead += serialiser.Serialise(chunkHeader);

				//What is it?
				switch (chunkHeader.chunkId)
				{
				case ChunkId::Filename:
				{
					//Read filename header
					bytesRead += serialiser.Serialise(filenameHeader);
					Serialiser::EndianSwap(filenameHeader.length);

					//Read string
					bytesRead += serialiser.Serialise(readString, filenameHeader.length);

					if (filenameHeader.flags == 0x1)
					{
						//This is the filename passed for assembly
						m_AssembledFile = readString;
					}
					else
					{
						//If filename already exists, continue adding data to it
						std::vector<FilenameSection>::iterator filenameIt = std::find_if(m_Filenames.begin(), m_Filenames.end(), [&](const FilenameSection& lhs) { return ToUpper(lhs.filename) == ToUpper(readString); });
						if (filenameIt != m_Filenames.end())
						{
							//Continue
							filenameSection = *filenameIt;

							//Fetch line counter
							currentLine = filenameSection.addresses[filenameIt->addresses.size() - 1].lineTo;
						}
						else
						{
							//This is the first address in a filename chunk
							filenameSection = FilenameSection();
							filenameSection.filename = readString;

							//Reset line counter
							currentLine = 0;
						}

						//Chunk payload contains address
						addressEntry.address = chunkHeader.payload;
						addressEntry.lineFrom = currentLine;
						addressEntry.lineTo = filenameHeader.firstLine;
						currentLine = filenameHeader.firstLine;
						filenameSection.addresses.push_back(addressEntry);

						//Add to filename list
						m_Filenames.push_back(filenameSection);
					}

					break;
				}

				case ChunkId::Address:
				{
					//Chunk payload contains address
					addressEntry.address = chunkHeader.payload;

					//Set line range
					addressEntry.lineFrom = currentLine;
					currentLine++;
					addressEntry.lineTo = currentLine;

					//Add
					filenameSection.addresses.push_back(addressEntry);

					break;
				}

				case ChunkId::AddressWithCount:
				{
					//Read line count
					int8_t lineCount = 0;
					bytesRead += serialiser.Serialise(lineCount);

					//Chunk payload contains address
					addressEntry.address = chunkHeader.payload;

					//Set line range
					addressEntry.lineFrom = currentLine;
					currentLine += lineCount;
					addressEntry.lineTo = currentLine;

					//Add
					filenameSection.addresses.push_back(addressEntry);

					break;
				}

				case ChunkId::Symbol:
				case ChunkId::Equate:
				{
					//Read symbol string length
					int8_t stringLength = 0;
					bytesRead += serialiser.Serialise(stringLength);

					//Read string
					bytesRead += serialiser.Serialise(symbolEntry.name, stringLength);

					//Payload contains address
					symbolEntry.address = chunkHeader.payload;

					m_Symbols.push_back(symbolEntry);

					break;
				}

				case ChunkId::EndOfSection:
					//Nothing of interest
					break;
				}
			}

			//Build address to file/line map
			for (FilenameSection section : m_Filenames)
			{
				for(AddressEntry address : section.addresses)
				{
					if (m_Addr2FileLine.find(address.address) == m_Addr2FileLine.end())
					{
						m_Addr2FileLine[address.address].first = section.filename;
						m_Addr2FileLine[address.address].second = address.lineTo;
					}
				}
			}

			return true;
		}

		return false;
	}

	uint32_t SymbolParserASM68K::FindSymbolValue(const std::string& name)
	{
		std::vector<SymbolEntry>::iterator symbolIt = std::find_if(m_Symbols.begin(), m_Symbols.end(), [&](const SymbolEntry& lhs) { return ToUpper(lhs.name) == ToUpper(name); });
		if (symbolIt != m_Symbols.end())
		{
			return symbolIt->address;
		}

		return 0;
	}
}
