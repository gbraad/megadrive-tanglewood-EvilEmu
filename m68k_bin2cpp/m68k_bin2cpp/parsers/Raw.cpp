// ============================================================
//   Matt Phillips (c) 2020 BIG EVIL CORPORATION
// ============================================================
//   http://www.bigevilcorporation.co.uk
// ============================================================
//   m68k_bin2cpp - Converts 68K bin/symbol files to
//   C++ binary block, and extracts known memory addresses
// ============================================================

#include "Raw.h"

namespace m68k
{
	uint32_t BinaryParserRaw::ReadBinary(const std::vector<uint8_t>& data, std::vector<uint8_t>& binary)
	{
		binary = data;
		return binary.size();
	}
}