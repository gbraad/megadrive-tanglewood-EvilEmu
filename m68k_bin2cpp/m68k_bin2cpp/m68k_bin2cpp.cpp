// ============================================================
//   Matt Phillips (c) 2016 BIG EVIL CORPORATION
// ============================================================
//   http://www.bigevilcorporation.co.uk
// ============================================================
//   snasm68k_bin2cpp - Converts SNASM68K COFF files to
//   C++ binary block, and extracts known memory addresses
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>

#include "parsers/Parser.h"
#include "parsers/ASM68K.h"
#include "parsers/SNASM68K.h"
#include "parsers/ELF32.h"
#include "parsers/Raw.h"

#define HEX2(val) std::hex << std::setfill('0') << std::setw(2) << std::uppercase << (int)(val&0xFF)
#define HEX4(val) std::hex << std::setfill('0') << std::setw(4) << std::uppercase << (int)(val&0xFFFF)
#define HEX8(val) std::hex << std::setfill('0') << std::setw(8) << std::uppercase << (int)(val&0xFFFFFFFF)

static const int MAX_BYTES_PER_LINE = 128;

enum class CmdArgs : int
{
	Executable,
	Operation,
	InputFile,

	Count
};

enum class CmdArgsBinary : int
{
	Executable,
	Operation,
	InputFile,
	OutputROM,

	Count
};

enum class CmdArgsELF32 : int
{
	Executable,
	Operation,
	InputFile,
	OutputROM,
	OutputVars,
	FirstSymbol,

	Count
};

enum class CmdArgsCOFF : int
{
	Executable,
	Operation,
	InputFile,
	OutputROM,
	OutputVars,
	FirstSymbol,

	Count
};

enum class CmdArgsSymb : int
{
	Executable,
	Operation,
	InputBinary,
	InputSymb,
	OutputROM,
	OutputVars,
	FirstSymbol,

	Count
};

enum class FileType
{
	Unknown,
	Raw,
	ASM68K,
	SNASM68K,
	ELF32
};

enum class SymbolSize : int
{
	byte,
	word,
	longword
};

struct Symbol
{
	std::string name;
	unsigned int value;
};

bool stricmp(const std::string& a, const std::string& b)
{
	return std::equal(a.begin(), a.end(),
		b.begin(), b.end(),
		[](char a, char b) {
		return tolower(a) == tolower(b);
	});
}

void PrintUsage()
{
	std::cout << "Usage:" << std::endl
		<< "  Raw binary .bin:   m68k_bin2cpp.exe -b [binary.bin] [binary.h]" << std::endl
		<< "  ASM68K     .symb:  m68k_bin2cpp.exe -s [binary.bin] [symbols.symb] [binary.h] [symbols.h] <symbolname> <symbolname>..." << std::endl
		<< "  ELF32      .elf:   m68k_bin2cpp.exe -e [symbols.elf] [binary.h] [symbols.h] <symbolname> <symbolname>..." << std::endl
		<< "  SNASM68K   .cof:   m68k_bin2cpp.exe -c [symbols.coff] [binary.h] [symbols.h] <symbolname> <symbolname>..." << std::endl;
}

void WriteHeader(std::stringstream& stream)
{
	stream << "//------------------------------------------------------------------" << std::endl;
	stream << "// m68k_bin2cpp" << std::endl;
	stream << "// BIG EVIL CORPORATION LTD" << std::endl;
	stream << "// M68K binary/symbols to C++ tool" << std::endl;
	stream << "// Matt Phillips 2020" << std::endl;
	stream << "//------------------------------------------------------------------" << std::endl;
	stream << std::endl;
}

FileType ParseArgs(int argc, char* argv[], std::string& binaryFilenameIn, std::string& binaryFilenameOut, std::string& symbolsFilenameIn, std::string& symbolsFilenameOut, std::vector<std::string>& symbolNames)
{
	FileType fileType = FileType::Unknown;
	int firstSymbolArgIdx = 0;

	if (stricmp(std::string(argv[(int)CmdArgs::Operation]), "-b"))
	{
		fileType = FileType::Raw;
		binaryFilenameIn = argv[(int)CmdArgsBinary::InputFile];
		binaryFilenameOut = argv[(int)CmdArgsBinary::OutputROM];
	}
	else if (stricmp(std::string(argv[(int)CmdArgs::Operation]), "-c"))
	{
		fileType = FileType::SNASM68K;
		binaryFilenameIn = argv[(int)CmdArgsCOFF::InputFile];
		binaryFilenameOut = argv[(int)CmdArgsCOFF::OutputROM];
		symbolsFilenameIn = argv[(int)CmdArgsCOFF::InputFile];
		symbolsFilenameOut = argv[(int)CmdArgsCOFF::OutputVars];
		firstSymbolArgIdx = (int)CmdArgsCOFF::FirstSymbol;
	}
	else if (stricmp(std::string(argv[(int)CmdArgs::Operation]), "-s"))
	{
		fileType = FileType::ASM68K;
		binaryFilenameIn = argv[(int)CmdArgsSymb::InputBinary];
		binaryFilenameOut = argv[(int)CmdArgsSymb::OutputROM];
		symbolsFilenameIn = argv[(int)CmdArgsSymb::InputSymb];
		symbolsFilenameOut = argv[(int)CmdArgsSymb::OutputVars];
		firstSymbolArgIdx = (int)CmdArgsSymb::FirstSymbol;
	}
	else if (stricmp(std::string(argv[(int)CmdArgs::Operation]), "-e"))
	{
		fileType = FileType::ELF32;
		binaryFilenameIn = argv[(int)CmdArgsELF32::InputFile];
		binaryFilenameOut = argv[(int)CmdArgsELF32::OutputROM];
		symbolsFilenameIn = argv[(int)CmdArgsELF32::InputFile];
		symbolsFilenameOut = argv[(int)CmdArgsELF32::OutputVars];
		firstSymbolArgIdx = (int)CmdArgsELF32::FirstSymbol;
	}

	if (firstSymbolArgIdx > 0)
	{
		for (int i = firstSymbolArgIdx; i < argc; i++)
		{
			symbolNames.push_back(argv[i]);
		}
	}

	return fileType;
}

m68k::SymbolParser* CreateSymbolParser(FileType fileType)
{
	m68k::SymbolParser* parser = nullptr;

	switch (fileType)
	{
	case FileType::ASM68K:
		parser = new m68k::SymbolParserASM68K();
		break;
	case FileType::SNASM68K:
		parser = new m68k::ParserSNASM68K();
		break;
	case FileType::ELF32:
		parser = new m68k::ParserELF32();
		break;
	}

	return parser;
}

m68k::BinaryParser* CreateBinaryParser(FileType fileType)
{
	m68k::BinaryParser* parser = nullptr;

	switch (fileType)
	{
	case FileType::ASM68K:
		parser = new m68k::BinaryParserRaw();
		break;
	case FileType::SNASM68K:
		parser = new m68k::ParserSNASM68K();
		break;
	case FileType::ELF32:
		parser = new m68k::ParserELF32();
		break;
	}

	return parser;
}

int ReadBinaryFile(std::string filename, std::vector<uint8_t>& data)
{
	std::fstream file;
	file.open(filename, std::ios::in | std::ios::binary);
	if (file.is_open())
	{
		//Get filesize
		file.seekg(0, std::ios::end);
		std::streampos size = file.tellg();
		file.seekg(0, std::ios::beg);

		//Read data
		data.resize(size);
		file.read((char*)data.data(), size);
		file.close();

		return (int)size;
	}

	return 0;
}

bool WriteTextFile(std::string filename, std::stringstream& stream)
{
	std::fstream file;
	file.open(filename, std::ios::out | std::ios::binary);
	if (file.is_open())
	{
		file.write(stream.str().data(), stream.str().size());
		file.close();
		return true;
	}

	return false;
}

int main(int argc, char* argv[])
{
	if (argc < (int)CmdArgs::InputFile)
	{
		PrintUsage();
		return 0;
	}

	std::string binaryFilenameIn;
	std::string binaryFilenameOut;
	std::string symbolsFilenameIn;
	std::string symbolsFilenameOut;
	std::vector<std::string> symbolNames;
	FileType fileType = ParseArgs(argc, argv, binaryFilenameIn, binaryFilenameOut, symbolsFilenameIn, symbolsFilenameOut, symbolNames);

	if(fileType == FileType::Unknown)
	{
		PrintUsage();
	}
	else
	{
		//Parse binary
		std::vector<uint8_t> binaryData;
		if (ReadBinaryFile(binaryFilenameIn, binaryData) > 0)
		{
			if (m68k::BinaryParser* parser = CreateBinaryParser(fileType))
			{
				std::vector<uint8_t> binaryOutput;
				uint32_t binarySize = parser->ReadBinary(binaryData, binaryOutput);

				if (binarySize > 0)
				{
					//Write binary to CPP header
					std::stringstream outStreamROM;
					WriteHeader(outStreamROM);

					outStreamROM << "// Binary data" << std::endl;
					outStreamROM << "static const int m68k_binary_size = 0x" << HEX8(binarySize) << ";" << std::endl;
					outStreamROM << "static const unsigned char m68k_binary[] = " << std::endl;
					outStreamROM << "{" << std::endl;

					for (int i = 0; i < binarySize; i++)
					{
						if (i > 0 && (i % MAX_BYTES_PER_LINE) == 0)
							outStreamROM << std::endl;

						outStreamROM << "0x" << HEX2(binaryOutput[i]) << ",";
					}

					outStreamROM << std::endl;
					outStreamROM << "};" << std::endl;
					outStreamROM << std::endl;

					//Write output file
					if (WriteTextFile(binaryFilenameOut, outStreamROM))
					{
						std::cout << "Wrote ROM file (" << binarySize << " bytes) to " << binaryFilenameOut << std::endl;
					}
					else
					{
						std::cout << "Failed to write ROM file " << binaryFilenameOut << std::endl;
					}
				}
				else
				{
					std::cout << "Error reading binary from " << binaryFilenameIn << std::endl;
				}

				delete parser;
			}
		}
		else
		{
			std::cout << "Error reading binary from " << binaryFilenameIn << std::endl;
		}

		//Parse symbols
		std::vector<uint8_t> symbolsData;
		if (ReadBinaryFile(symbolsFilenameIn, symbolsData) > 0)
		{
			if (m68k::SymbolParser* parser = CreateSymbolParser(fileType))
			{
				if (parser->ReadSymbols(symbolsData))
				{
					//Find symbols
					std::vector<Symbol> symbols;

					for (int i = 0; i < symbolNames.size(); i++)
					{
						Symbol symbol;
						symbol.name = symbolNames[i];
						symbol.value = parser->FindSymbolValue(symbol.name);
						symbols.push_back(symbol);
						std::cout << "Found symbol '" << symbol.name << "' : value = 0x" << HEX8(symbol.value) << std::endl;
					}

					//Write symbols to CPP header
					std::stringstream outStreamVars;
					WriteHeader(outStreamVars);

					outStreamVars << "// Symbol addresses" << std::endl;

					for (int i = 0; i < symbols.size(); i++)
					{
						outStreamVars << "static const unsigned int m68k_symbol_" << symbols[i].name << "_val = 0x" << HEX8(symbols[i].value) << ";" << std::endl;
					}

					//Write output file
					if (WriteTextFile(symbolsFilenameOut, outStreamVars))
					{
						std::cout << "Wrote vars to " << symbolsFilenameOut << std::endl;
					}
					else
					{
						std::cout << "Failed to write vars file " << symbolsFilenameOut << std::endl;
					}
				}
				else
				{
					std::cout << "Error reading symbols from " << symbolsFilenameIn << std::endl;
				}

				delete parser;
			}
		}
		else
		{
			std::cout << "Error reading symbols from " << symbolsFilenameIn << std::endl;
		}
	}

	return 0;
}
