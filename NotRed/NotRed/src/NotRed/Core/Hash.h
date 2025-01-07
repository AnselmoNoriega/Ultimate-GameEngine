#pragma once

#include <string>

namespace NR
{
	class Hash
	{
	public:
		static uint32_t GenerateFNVHash(const char* str);
		static uint32_t GenerateFNVHash(const std::string& string);

		static uint32_t CRC32(const char* str);
		static uint32_t CRC32(const std::string& string);
	};
}