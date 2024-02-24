#pragma once

#include <string>

namespace NR
{
    class Shader
    {
    public:
        Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
        ~Shader();

        void Bind() const;
        void Unbind() const;

    private:
        uint32_t mID;
    };
}