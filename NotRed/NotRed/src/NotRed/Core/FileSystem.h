#pragma once

#include "Buffer.h"

namespace NR
{
    class FileSystem
    {
    public:
        template <typename T>
        static Buffer<T> ReadFileBinary(const std::filesystem::path& filepath);
    };
}