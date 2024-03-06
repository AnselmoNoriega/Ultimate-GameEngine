#include "nrpch.h"
#include "GLShader.h"

#include <fstream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace NR
{
    GLShader::GLShader(const std::string& filepath)
    {
        NR_PROFILE_FUNCTION();

        mName = GetShaderName(filepath);
        const std::string vertSource = ParseFile(filepath + "/" + mName + "Vert.glsl");
        const std::string fragSource = ParseFile(filepath + "/" + mName + "Frag.glsl");

        ShaderInfo shaders[] = {
            {glCreateShader(GL_VERTEX_SHADER), vertSource},
            {glCreateShader(GL_FRAGMENT_SHADER), fragSource}
        };

        Compile(shaders, 2);
    }

    GLShader::GLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
        : mName(name)
    {
        NR_PROFILE_FUNCTION();

        ShaderInfo shaders[] = {
            {glCreateShader(GL_VERTEX_SHADER), vertexSrc},
            {glCreateShader(GL_FRAGMENT_SHADER), fragmentSrc}
        };

        Compile(shaders, 2);
    }

    std::string GLShader::ParseFile(const std::string& filepath)
    {
        NR_PROFILE_FUNCTION();

        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        std::string result;

        if (in)
        {
            in.seekg(0, std::ios::end);
            result.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&result[0], result.size());
            in.close();

        }
        else
        {
            NR_CORE_ERROR("Could not open file \"{0}\"!", filepath);
        }

        return result;
    }

    std::string GLShader::GetShaderName(const std::string& filepath)
    {
        NR_PROFILE_FUNCTION();

        size_t fileName = filepath.find_last_of("/\\");

        if (fileName != std::string::npos)
        {
            return filepath.substr(fileName + 1);
        }
        else
        {
            NR_CORE_WARN("File is in wrong folder!");
            return filepath;
        }
    }

    void GLShader::Compile(const ShaderInfo* shaders, uint32_t count)
    {
        NR_PROFILE_FUNCTION();

        for (int i = 0; i < count; ++i)
        {
            const GLchar* source = shaders[i].second.c_str();
            glShaderSource(shaders[i].first, 1, &source, 0);

            glCompileShader(shaders[i].first);

            GLint isCompiled = 0;
            glGetShaderiv(shaders[i].first, GL_COMPILE_STATUS, &isCompiled);
            if (isCompiled == GL_FALSE)
            {
                GLint maxLength = 0;
                glGetShaderiv(shaders[i].first, GL_INFO_LOG_LENGTH, &maxLength);

                std::vector<GLchar> infoLog(maxLength);
                glGetShaderInfoLog(shaders[i].first, maxLength, &maxLength, &infoLog[0]);

                while (i >= 0)
                {
                    glDeleteShader(shaders[i--].first);
                }

                NR_CORE_ERROR("{0}", infoLog.data());
                NR_CORE_ASSERT(false, "GLShader failed to compile!");
                return;
            }
        }

        mID = glCreateProgram();
        GLuint program = mID;

        for (int i = 0; i < count; ++i)
        {
            glAttachShader(program, shaders[i].first);
        }

        glLinkProgram(program);

        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, static_cast<int*>(&isLinked));
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

            glDeleteProgram(program);
            for (int i = 0; i < count; ++i)
            {
                glDeleteShader(shaders[i].first);
            }

            NR_CORE_ERROR("{0}", infoLog.data());
            NR_CORE_ASSERT(false, "GLShader link failure!");
            return;
        }

        for (int i = 0; i < count; ++i)
        {
            glDetachShader(program, shaders[i].first);
        }
    }

    void GLShader::Bind() const
    {
        NR_PROFILE_FUNCTION();

        glUseProgram(mID);
    }

    void GLShader::Unbind() const
    {
        NR_PROFILE_FUNCTION();

        glUseProgram(0);
    }

    void GLShader::SetInt(const std::string& name, int value)
    {
        SetUniformInt(name, value);
    }

    void GLShader::SetFloat3(const std::string& name, const glm::vec3& value)
    {
        SetUniformFloat3(name, value);
    }

    void GLShader::SetFloat4(const std::string& name, const glm::vec4& value)
    {
        SetUniformFloat4(name, value);
    }

    void GLShader::SetMat4(const std::string& name, const glm::mat4& value)
    {
        SetUniformMat4(name, value);
    }

    void GLShader::SetUniformInt(const std::string& name, const int values)
    {
        GLint location = glGetUniformLocation(mID, name.c_str());
        glUniform1i(location, values);
    }

    void GLShader::SetUniformFloat(const std::string& name, float values)
    {
        GLint location = glGetUniformLocation(mID, name.c_str());
        glUniform1f(location, values);
    }

    void GLShader::SetUniformFloat2(const std::string& name, const glm::vec2& values)
    {
        GLint location = glGetUniformLocation(mID, name.c_str());
        glUniform2f(location, values.x, values.y);
    }

    void GLShader::SetUniformFloat3(const std::string& name, const glm::vec3& values)
    {
        GLint location = glGetUniformLocation(mID, name.c_str());
        glUniform3f(location, values.x, values.y, values.z);
    }

    void GLShader::SetUniformFloat4(const std::string& name, const glm::vec4& values)
    {
        GLint location = glGetUniformLocation(mID, name.c_str());
        glUniform4f(location, values.x, values.y, values.z, values.w);
    }

    void GLShader::SetUniformMat3(const std::string& name, const glm::mat3& matrix)
    {
        GLint location = glGetUniformLocation(mID, name.c_str());
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void GLShader::SetUniformMat4(const std::string& name, const glm::mat4& matrix)
    {
        GLint location = glGetUniformLocation(mID, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }

    GLShader::~GLShader()
    {
        NR_PROFILE_FUNCTION();

        glDeleteProgram(mID);
    }
}