#include "nrpch.h"
#include "GLShader.h"

#include <sstream>
#include <limits>

namespace NR
{
    GLShader::GLShader(const std::string& filepath)
    {
        mName = GetShaderName(filepath);
        const std::string vertSource = ParseFile(filepath + "/" + mName + "Vert.glsl");
        const std::string fragSource = ParseFile(filepath + "/" + mName + "Frag.glsl");

        ShaderInfo shaders[] = {
            {glCreateShader(GL_VERTEX_SHADER), vertSource},
            {glCreateShader(GL_FRAGMENT_SHADER), fragSource}
        };

            Compile(shaders, 2);
    }

    void GLShader::Bind()
    {
        NR_RENDER_S({
            glUseProgram(self->mID);
            });
    }

    std::string GLShader::ParseFile(const std::string& filepath)
    {
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

    void GLShader::UploadUniformBuffer(const UniformBufferBase& uniformBuffer)
    {
        for (unsigned int i = 0; i < uniformBuffer.GetUniformCount(); i++)
        {
            const UniformDecl& decl = uniformBuffer.GetUniforms()[i];
            switch (decl.Type)
            {
            case UniformType::Float:
            {
                const std::string& name = decl.Name;
                float value = *(float*)(uniformBuffer.GetBuffer() + decl.Offset);
                NR_RENDER_S2(name, value, {
                    self->UploadUniformFloat(name, value);
                    });
            }
            case UniformType::Float4:
            {
                const std::string& name = decl.Name;
                glm::vec4& values = *(glm::vec4*)(uniformBuffer.GetBuffer() + decl.Offset);
                NR_RENDER_S2(name, values, {
                    self->UploadUniformFloat4(name, values);
                    });
            }
            }
        }
    }

    void GLShader::UploadUniformFloat(const std::string& name, float value)
    {
        glUseProgram(mID);
        glUniform1f(glGetUniformLocation(mID, name.c_str()), value);
    }

    void GLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
    {
        glUseProgram(mID);
        glUniform4f(glGetUniformLocation(mID, name.c_str()), values.x, values.y, values.z, values.w);
    }
}