#include "nrpch.h"
#include "FileSystem.h"

#include "mono/jit/jit.h"

namespace NR
{
    template<typename T>
    Buffer<T> FileSystem::ReadFileBinary(const std::filesystem::path& filepath)
    {
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

		if (!stream)
		{
			return {};
		}


		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		uint64_t size = end - stream.tellg();

		if (size == 0)
		{
			return {};
		}

		Buffer<T> buffer(size);
		stream.read((char*)buffer.Data, size);
		stream.close();
		return buffer;
    }

	template Buffer<char> FileSystem::ReadFileBinary(const std::filesystem::path& filepath);
	template Buffer<mono_byte> FileSystem::ReadFileBinary(const std::filesystem::path& filepath);
}