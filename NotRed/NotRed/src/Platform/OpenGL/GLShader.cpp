#include "nrpch.h"
#include "GLShader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace NR
{
    GLShader::GLShader(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        GLuint vertexGLShader = glCreateShader(GL_VERTEX_SHADER);

        const GLchar* source = vertexSrc.c_str();
        glShaderSource(vertexGLShader, 1, &source, 0);

        glCompileShader(vertexGLShader);

        GLint isCompiled = 0;
        glGetShaderiv(vertexGLShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(vertexGLShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(vertexGLShader, maxLength, &maxLength, &infoLog[0]);

            glDeleteShader(vertexGLShader);

            NR_CORE_ERROR("{0}", infoLog.data());
            NR_CORE_ASSERT(false, "Vertex GLShader failed to compile!");
            return;
        }

        GLuint fragmentGLShader = glCreateShader(GL_FRAGMENT_SHADER);

        source = fragmentSrc.c_str();
        glShaderSource(fragmentGLShader, 1, &source, 0);

        glCompileShader(fragmentGLShader);

        glGetShaderiv(fragmentGLShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(fragmentGLShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(fragmentGLShader, maxLength, &maxLength, &infoLog[0]);

            glDeleteShader(fragmentGLShader);
            glDeleteShader(vertexGLShader);

            NR_CORE_ERROR("{0}", infoLog.data());
            NR_CORE_ASSERT(false, "Fragment GLShader failed to compile!");
            return;
        }

        mID = glCreateProgram();
        GLuint program = mID;

        glAttachShader(program, vertexGLShader);
        glAttachShader(program, fragmentGLShader);

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
            glDeleteShader(vertexGLShader);
            glDeleteShader(fragmentGLShader);

            NR_CORE_ERROR("{0}", infoLog.data());
            NR_CORE_ASSERT(false, "GLShader link failure!");
            return;
        }

        glDetachShader(program, vertexGLShader);
        glDetachShader(program, fragmentGLShader);
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

    void GLShader::Bind() const
    {
        glUseProgram(mID);
    }

    void GLShader::Unbind() const
    {
        glUseProgram(0);
    }

    GLShader::~GLShader()
    {
        glDeleteProgram(mID);
    }
}