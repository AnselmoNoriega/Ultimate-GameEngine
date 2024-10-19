#pragma once

namespace NR::Utils 
{
	namespace String
	{
		bool EqualsIgnoreCase(const std::string& a, const std::string& b);
		std::string& ToLower(std::string& string);
		std::string ToLowerCopy(const std::string& string);
	}

	std::string GetFilename(const std::string& filepath);

	std::string GetExtension(const std::string& filename);
	std::string RemoveExtension(const std::string& filename);

	bool StartsWith(const std::string& string, const std::string& start);

	std::vector<std::string> SplitString(const std::string& string, const std::string& delimiters);
	std::vector<std::string> SplitString(const std::string& string, const char delimiter);

	std::string ToLower(const std::string& string);
	std::string BytesToString(uint64_t bytes);
}