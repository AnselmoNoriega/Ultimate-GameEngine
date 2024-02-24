#include "nrpch.h"
#include "Shader.h"

#include <glad/glad.h>

namespace NR
{
    Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        
        const GLchar* source = vertexSrc.c_str();
        glShaderSource(vertexShader, 1, &source, 0);
        
        glCompileShader(vertexShader);
        
        GLint isCompiled = 0;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);
            
            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);
            
            glDeleteShader(vertexShader);
            
            NR_CORE_ERROR("{0}", infoLog.data());
            NR_CORE_ASSERT(false, "Vertex shader failed to compile!");
            return;
        }
        
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        
        source = fragmentSrc.c_str();
        glShaderSource(fragmentShader, 1, &source, 0);
        
        glCompileShader(fragmentShader);
        
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);
            
            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &infoLog[0]);
            
            glDeleteShader(fragmentShader);
            glDeleteShader(vertexShader);
            
            NR_CORE_ERROR("{0}", infoLog.data());
            NR_CORE_ASSERT(false, "Fragment shader failed to compile!");
            return;
        }
        
        mID = glCreateProgram();
        GLuint program = mID;
        
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        
        glLinkProgram(program);
        
        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, static_cast<int*>(& isLinked));
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
            
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
            
            glDeleteProgram(program);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            
            NR_CORE_ERROR("{0}", infoLog.data());
            NR_CORE_ASSERT(false, "Shader link failure!");
            return;
        }
        
        glDetachShader(program, vertexShader);
        glDetachShader(program, fragmentShader);
    }

    Shader::~Shader()
    {
        glDeleteProgram(mID);
    }

    void Shader::Bind() const
    {
        glUseProgram(mID);
    }

    void Shader::Unbind() const
    {
        glUseProgram(0);
    }
}