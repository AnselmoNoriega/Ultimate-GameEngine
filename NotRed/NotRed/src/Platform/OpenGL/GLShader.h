#pragma once

#include <glm/glm.hpp>

#include "NotRed/Renderer/Shader.h"

//Should be removed
typedef unsigned int GLenum;

namespace NR
{
    using ShaderInfo = std::pair<uint32_t, std::string>;

    class GLShader : public Shader 
    {
    public:
        GLShader(const std::string& filepath);
        GLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
        ~GLShader() override;

        void Bind() const override;
        void Unbind() const override; 
        
        const std::string& GetName() const override { return mName; }

        void SetUniformInt(const std::string& name, const int values);

        void SetUniformFloat(const std::string& name, float values);
        void SetUniformFloat2(const std::string& name, const glm::vec2& values);
        void SetUniformFloat3(const std::string& name, const glm::vec3& values);
        void SetUniformFloat4(const std::string& name, const glm::vec4& values);

        void SetUniformMat3(const std::string& name, const glm::mat3& matrix);
        void SetUniformMat4(const std::string& name, const glm::mat4& matrix);

    private:
        std::string ParseFile(const std::string& filepath);
        std::string GetShaderName(const std::string& filepath);
        void Compile(const ShaderInfo* shaders, uint32_t count);

    private:
        uint32_t mID;
        std::string mName;
    };
}

