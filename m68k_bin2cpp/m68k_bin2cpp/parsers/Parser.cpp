// ============================================================
//   Matt Phillips (c) 2020 BIG EVIL CORPORATION
// ============================================================
//   http://www.bigevilcorporation.co.uk
// ============================================================
//   m68k_bin2cpp - Converts 68K bin/symbol files to
//   C++ binary block, and extracts known memory addresses
// ============================================================

#include "Parser.h"

#include <algorithm>

namespace m68k
{
	std::string ToUpper(const std::string& string)
	{
		std::string stringUpper = string;
		std::transform(stringUpper.begin(), stringUpper.end(), stringUpper.begin(), ::toupper);
		return stringUpper;
	}

	bool stricmp(const std::string& a, const std::string& b)
	{
		return std::equal(a.begin(), a.end(),
			b.begin(), b.end(),
			[](char a, char b) {
			return tolower(a) == tolower(b);
		});
	}
}