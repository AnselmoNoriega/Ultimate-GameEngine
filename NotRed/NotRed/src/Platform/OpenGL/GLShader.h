#pragma once

#include "NotRed/Renderer/Shader.h"

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

        void SetInt(const std::string& name, int value) override;
        void SetIntArray(const std::string& name, int* values, uint32_t count) override;
        void SetFloat3(const std::string& name, const glm::vec3& value) override;
        void SetFloat4(const std::string& name, const glm::vec4& value) override;
        void SetMat4(const std::string& name, const glm::mat4& value) override;

        void SetUniformInt(const std::string& name, const int values);
        void SetUniformIntArray(const std::string& name, int* values, uint32_t count);

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

