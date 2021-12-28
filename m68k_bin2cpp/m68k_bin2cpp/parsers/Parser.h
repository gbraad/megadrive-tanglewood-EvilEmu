// ============================================================
//   Matt Phillips (c) 2020 BIG EVIL CORPORATION
// ============================================================
//   http://www.bigevilcorporation.co.uk
// ============================================================
//   m68k_bin2cpp - Converts 68K bin/symbol files to
//   C++ binary block, and extracts known memory addresses
// ============================================================

#pragma once

#include <string>
#include <vector>

namespace m68k
{
	class BinaryParser
	{
	public:
		virtual ~BinaryParser() {}

		virtual uint32_t ReadBinary(const std::vector<uint8_t>& data, std::vector<uint8_t>& binary) = 0;
	};

	class SymbolParser
	{
	public:
		virtual ~SymbolParser() {}

		virtual bool ReadSymbols(const std::vector<uint8_t>& data) = 0;
		virtual uint32_t FindSymbolValue(const std::string& name) = 0;
	};

	std::string ToUpper(const std::string& string);
	bool stricmp(const std::string& a, const std::string& b);
}
