#include "nrpch.h"
#include "GLShader.h"

#include <string>
#include <sstream>
#include <limits>

#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
#define UNIFORMLOGGING 0

#if UNIFORMLOGGING
#define NR_LOG_UNIFORM(...) NR_CORE_WARN(__VA_ARGS__)
#else
#define NR_LOG_UNIFORM
#endif

#pragma region Utils

    const char* FindToken(const char* str, const std::string& token)
    {
        const char* t = str;
        while (t = strstr(t, token.c_str()))
        {
            bool left = str == t || isspace(t[-1]);
            bool right = !t[token.size()] || isspace(t[token.size()]);
            if (left && right)
            {
                return t;
            }

            t += token.size();
        }
        return nullptr;
    }

    const char* FindToken(const std::string& string, const std::string& token)
    {
        return FindToken(string.c_str(), token);
    }

    std::vector<std::string> SplitString(const std::string& string, const std::string& delimiters)
    {
        size_t start = 0;
        size_t end = string.find_first_of(delimiters);

        std::vector<std::string> result;

        while (end <= std::string::npos)
        {
            std::string token = string.substr(start, end - start);
            if (!token.empty())
            {
                result.push_back(token);
            }

            if (end == std::string::npos)
            {
                break;
            }

            start = end + 1;
            end = string.find_first_of(delimiters, start);
        }

        return result;
    }

    std::vector<std::string> SplitString(const std::string& string, const char delimiter)
    {
        return SplitString(string, std::string(1, delimiter));
    }

    std::vector<std::string> Tokenize(const std::string& string)
    {
        return SplitString(string, " \t\n");
    }

    std::vector<std::string> GetLines(const std::string& string)
    {
        return SplitString(string, "\n");
    }

    std::string GetBlock(const char* str, const char** outPosition)
    {
        const char* end = strstr(str, "}");
        if (!end)
        {
            return str;
        }

        if (outPosition)
        {
            *outPosition = end;
        }
        uint32_t length = end - str + 1;
        return std::string(str, length);
    }

    std::string GetStatement(const char* str, const char** outPosition)
    {
        const char* end = strstr(str, ";");
        if (!end)
        {
            return str;
        }

        if (outPosition)
        {
            *outPosition = end;
        }
        uint32_t length = end - str + 1;
        return std::string(str, length);
    }

    bool StartsWith(const std::string& string, const std::string& start)
    {
        return string.find(start) == 0;
    }

    static bool IsTypeStringResource(const std::string& type)
    {
        if (type == "sampler2D")		return true;
        if (type == "sampler2DMS")		return true;
        if (type == "samplerCube")		return true;
        if (type == "sampler2DShadow")	return true;
        return false;
    }

#pragma endregion

    GLShader::GLShader(const std::string& filepath)
    {
        mAssetPath = filepath;
        Reload();
    }

    Ref<GLShader> GLShader::CreateFromString(const std::string& vertSrc, const std::string& fragSrc)
    {
        Ref<GLShader> shader = Ref<GLShader>::Create();
        shader->Load(vertSrc, fragSrc);
        return shader;
    }

    void GLShader::Reload()
    {
        if (mAssetPath.empty())
        {
            NR_WARN("This shader can't be reloaded");
            return;
        }

        mName = GetShaderName(mAssetPath);

        std::string compute = "";
        ParseFile(mAssetPath + "/" + mName + ".comp", compute, true);

        std::string vert;
        std::string frag;

        if (compute == "")
        {
            ParseFile(mAssetPath + "/" + mName + ".vert", vert);
            ParseFile(mAssetPath + "/" + mName + ".frag", frag);
        }

        Load(vert, frag, compute);
    }

    void GLShader::Load(const std::string& vertSrc, const std::string& fragSrc, const std::string& computeSrc)
    {
        mShaderSource.clear();

        mResources.clear();
        mStructs.clear();
        mVSMaterialUniformBuffer.Reset();
        mPSMaterialUniformBuffer.Reset();

        if (computeSrc == "")
        {
            Parse(vertSrc, ShaderDomain::Vertex);
            mShaderSource.insert({ glCreateShader(GL_VERTEX_SHADER), vertSrc });

            Parse(fragSrc, ShaderDomain::Pixel);
            mShaderSource.insert({ glCreateShader(GL_FRAGMENT_SHADER), fragSrc });
        }
        else
        {
            mIsCompute = true;
            mShaderSource.insert({ glCreateShader(GL_COMPUTE_SHADER), computeSrc });
        }

        Renderer::Submit([=]() {
            if (mID)
            {
                glDeleteProgram(mID);
            }

            Compile();
            if (!mIsCompute)
            {
                ResolveUniforms();
                ValidateUniforms();
            }

            if (mLoaded)
            {
                for (auto& callback : mShaderReloadedCallbacks)
                {
                    callback();
                }
            }

            mLoaded = true;
            });
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

    void GLShader::ParseFile(const std::string& filepath, std::string& output, bool isCompute)
    {
        std::ifstream in(filepath, std::ios::in | std::ios::binary);

        if (in)
        {
            in.seekg(0, std::ios::end);
            output.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&output[0], output.size());
            in.close();
        }
        else if (!isCompute)
        {
            NR_CORE_ASSERT(false, "Could not open file");
        }
    }

    void GLShader::Parse(const std::string& source, ShaderDomain type)
    {
        const char* token;
        const char* str;

        str = source.c_str();
        while (token = FindToken(str, "struct"))
        {
            ParseUniformStruct(GetBlock(token, &str), type);
        }

        str = source.c_str();
        while (token = FindToken(str, "uniform"))
        {
            ParseUniform(GetStatement(token, &str), type);
        }
    }

    void GLShader::ParseUniformStruct(const std::string& block, ShaderDomain domain)
    {
        std::vector<std::string> tokens = Tokenize(block);

        uint32_t index = 0;
        ++index; // struct
        std::string name = tokens[index++];
        ShaderStruct* uniformStruct = new ShaderStruct(name);
        ++index; // {
        while (index < tokens.size())
        {
            if (tokens[index] == "}")
            {
                break;
            }

            std::string type = tokens[index++];
            std::string name = tokens[index++];

            // Strip ; from name if present
            if (const char* s = strstr(name.c_str(), ";"))
            {
                name = std::string(name.c_str(), s - name.c_str());
            }

            uint32_t count = 1;
            const char* namestr = name.c_str();
            if (const char* s = strstr(namestr, "["))
            {
                name = std::string(namestr, s - namestr);

                const char* end = strstr(namestr, "]");
                std::string c(s + 1, end - s);
                count = atoi(c.c_str());
            }
            ShaderUniformDeclaration* field = new GLShaderUniformDeclaration(domain, GLShaderUniformDeclaration::StringToType(type), name, count);
            uniformStruct->AddField(field);
        }
        mStructs.push_back(uniformStruct);
    }

    void GLShader::Compile()
    {
        for (const auto& [shaderID, shaderSource] : mShaderSource)
        {
            const GLchar* source = shaderSource.c_str();
            glShaderSource(shaderID, 1, &source, 0);

            glCompileShader(shaderID);

            GLint isCompiled = 0;
            glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);
            if (isCompiled == GL_FALSE)
            {
                GLint maxLength = 0;
                glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);

                std::vector<GLchar> infoLog(maxLength);
                glGetShaderInfoLog(shaderID, maxLength, &maxLength, &infoLog[0]);

                for (const auto& [prevShaderID, prevShaderSource] : mShaderSource)
                {
                    glDeleteShader(prevShaderID);
                    if (prevShaderID == shaderID)
                    {
                        break;
                    }
                }

                NR_CORE_ERROR("{0}", infoLog.data());
                NR_CORE_ASSERT(false, "GLShader failed to compile!");
                return;
            }
        }

        mID = glCreateProgram();
        GLuint program = mID;

        for (const auto& [shaderID, shaderSource] : mShaderSource)
        {
            glAttachShader(program, shaderID);
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
            for (const auto& [shaderID, shaderSource] : mShaderSource)
            {
                glDeleteShader(shaderID);
            }

            NR_CORE_ERROR("{0}", infoLog.data());
            NR_CORE_ASSERT(false, "GLShader link failure!");
            return;
        }

        for (const auto& [shaderID, shaderSource] : mShaderSource)
        {
            glDetachShader(program, shaderID);
        }
    }

    void GLShader::ResolveUniforms()
    {
        glUseProgram(mID);

        for (size_t i = 0; i < mVSRendererUniformBuffers.size(); ++i)
        {
            GLShaderUniformBufferDeclaration* decl = (GLShaderUniformBufferDeclaration*)mVSRendererUniformBuffers[i];
            const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
            for (size_t j = 0; j < uniforms.size(); ++j)
            {
                GLShaderUniformDeclaration* uniform = (GLShaderUniformDeclaration*)uniforms[j];
                if (uniform->GetType() == GLShaderUniformDeclaration::Type::STRUCT)
                {
                    const ShaderStruct& s = uniform->GetShaderUniformStruct();
                    const auto& fields = s.GetFields();
                    for (size_t k = 0; k < fields.size(); ++k)
                    {
                        GLShaderUniformDeclaration* field = (GLShaderUniformDeclaration*)fields[k];
                        field->mLocation = GetUniformLocation(uniform->mName + "." + field->mName);
                    }
                }
                else
                {
                    uniform->mLocation = GetUniformLocation(uniform->mName);
                }
            }
        }

        for (size_t i = 0; i < mPSRendererUniformBuffers.size(); ++i)
        {
            GLShaderUniformBufferDeclaration* decl = (GLShaderUniformBufferDeclaration*)mPSRendererUniformBuffers[i];
            const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
            for (size_t j = 0; j < uniforms.size(); ++j)
            {
                GLShaderUniformDeclaration* uniform = (GLShaderUniformDeclaration*)uniforms[j];
                if (uniform->GetType() == GLShaderUniformDeclaration::Type::STRUCT)
                {
                    const ShaderStruct& s = uniform->GetShaderUniformStruct();
                    const auto& fields = s.GetFields();
                    for (size_t k = 0; k < fields.size(); ++k)
                    {
                        GLShaderUniformDeclaration* field = (GLShaderUniformDeclaration*)fields[k];
                        field->mLocation = GetUniformLocation(uniform->mName + "." + field->mName);
                    }
                }
                else
                {
                    uniform->mLocation = GetUniformLocation(uniform->mName);
                }
            }
        }

        {
            const auto& decl = mVSMaterialUniformBuffer;
            if (decl)
            {
                const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
                for (size_t j = 0; j < uniforms.size(); ++j)
                {
                    GLShaderUniformDeclaration* uniform = (GLShaderUniformDeclaration*)uniforms[j];
                    if (uniform->GetType() == GLShaderUniformDeclaration::Type::STRUCT)
                    {
                        const ShaderStruct& s = uniform->GetShaderUniformStruct();
                        const auto& fields = s.GetFields();
                        for (size_t k = 0; k < fields.size(); ++k)
                        {
                            GLShaderUniformDeclaration* field = (GLShaderUniformDeclaration*)fields[k];
                            field->mLocation = GetUniformLocation(uniform->mName + "." + field->mName);
                        }
                    }
                    else
                    {
                        uniform->mLocation = GetUniformLocation(uniform->mName);
                    }
                }
            }
        }

        {
            const auto& decl = mPSMaterialUniformBuffer;
            if (decl)
            {
                const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
                for (size_t j = 0; j < uniforms.size(); ++j)
                {
                    GLShaderUniformDeclaration* uniform = (GLShaderUniformDeclaration*)uniforms[j];
                    if (uniform->GetType() == GLShaderUniformDeclaration::Type::STRUCT)
                    {
                        const ShaderStruct& s = uniform->GetShaderUniformStruct();
                        const auto& fields = s.GetFields();
                        for (size_t k = 0; k < fields.size(); ++k)
                        {
                            GLShaderUniformDeclaration* field = (GLShaderUniformDeclaration*)fields[k];
                            field->mLocation = GetUniformLocation(uniform->mName + "." + field->mName);
                        }
                    }
                    else
                    {
                        uniform->mLocation = GetUniformLocation(uniform->mName);
                    }
                }
            }
        }

        uint32_t sampler = 0;
        for (size_t i = 0; i < mResources.size(); ++i)
        {
            GLShaderResourceDeclaration* resource = (GLShaderResourceDeclaration*)mResources[i];
            int32_t location = GetUniformLocation(resource->mName);

            if (resource->GetCount() == 1)
            {
                resource->mRegister = sampler;
                if (location != -1)
                    UploadUniformInt(location, sampler);

                sampler++;
            }
            else if (resource->GetCount() > 1)
            {
                resource->mRegister = 0;
                uint32_t count = resource->GetCount();
                int* samplers = new int[count];
                for (uint32_t s = 0; s < count; s++)
                    samplers[s] = s;
                UploadUniformIntArray(resource->GetName(), samplers, count);
                delete[] samplers;
            }
        }
    }

    void GLShader::ValidateUniforms()
    {

    }

    void GLShader::Bind()
    {
        Ref<const GLShader> instance = this;
        Renderer::Submit([instance]() {
            glUseProgram(instance->mID);
            });
    }

    void GLShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
    {
        mShaderReloadedCallbacks.push_back(callback);
    }

    ShaderStruct* GLShader::FindStruct(const std::string& name)
    {
        for (ShaderStruct* s : mStructs)
        {
            if (s->GetName() == name)
            {
                return s;
            }
        }
        return nullptr;
    }

    void GLShader::ParseUniform(const std::string& statement, ShaderDomain domain)
    {
        std::vector<std::string> tokens = Tokenize(statement);
        uint32_t index = 0;

        index++; // "uniform"
        std::string typeString = tokens[index++];
        std::string name = tokens[index++];
        // Strip ; from name if present
        if (const char* s = strstr(name.c_str(), ";"))
        {
            name = std::string(name.c_str(), s - name.c_str());
        }

        std::string n(name);
        int32_t count = 1;
        const char* namestr = n.c_str();
        if (const char* s = strstr(namestr, "["))
        {
            name = std::string(namestr, s - namestr);

            const char* end = strstr(namestr, "]");
            std::string c(s + 1, end - s);
            count = atoi(c.c_str());
        }

        if (IsTypeStringResource(typeString))
        {
            ShaderResourceDeclaration* declaration = new GLShaderResourceDeclaration(GLShaderResourceDeclaration::StringToType(typeString), name, count);
            mResources.push_back(declaration);
        }
        else
        {
            GLShaderUniformDeclaration::Type t = GLShaderUniformDeclaration::StringToType(typeString);
            GLShaderUniformDeclaration* declaration = nullptr;

            if (t == GLShaderUniformDeclaration::Type::NONE)
            {
                // Find struct
                ShaderStruct* s = FindStruct(typeString);
                NR_CORE_ASSERT(s, "");
                declaration = new GLShaderUniformDeclaration(domain, s, name, count);
            }
            else
            {
                declaration = new GLShaderUniformDeclaration(domain, t, name, count);
            }

            if (StartsWith(name, "r_"))
            {
                if (domain == ShaderDomain::Vertex)
                {
                    ((GLShaderUniformBufferDeclaration*)mVSRendererUniformBuffers.front())->PushUniform(declaration);
                }
                else if (domain == ShaderDomain::Pixel)
                {
                    ((GLShaderUniformBufferDeclaration*)mPSRendererUniformBuffers.front())->PushUniform(declaration);
                }
            }
            else
            {
                if (domain == ShaderDomain::Vertex)
                {
                    if (!mVSMaterialUniformBuffer)
                    {
                        mVSMaterialUniformBuffer.Reset(new GLShaderUniformBufferDeclaration("", domain));
                    }

                    mVSMaterialUniformBuffer->PushUniform(declaration);
                }
                else if (domain == ShaderDomain::Pixel)
                {
                    if (!mPSMaterialUniformBuffer)
                    {
                        mPSMaterialUniformBuffer.Reset(new GLShaderUniformBufferDeclaration("", domain));
                    }

                    mPSMaterialUniformBuffer->PushUniform(declaration);
                }
            }
        }
    }

    int32_t GLShader::GetUniformLocation(const std::string& name) const
    {
        int32_t result = glGetUniformLocation(mID, name.c_str());
        if (result == -1)
        {
            NR_CORE_WARN("Could not find uniform '{0}' in the shader {1}", name, mName);
        }

        return result;
    }

    void GLShader::SetVSMaterialUniformBuffer(Buffer buffer)
    {
        Renderer::Submit([this, buffer]() {
            glUseProgram(mID);
            ResolveAndSetUniforms(mVSMaterialUniformBuffer, buffer);
            });
    }

    void GLShader::SetPSMaterialUniformBuffer(Buffer buffer)
    {
        Renderer::Submit([this, buffer]() {
            glUseProgram(mID);
            ResolveAndSetUniforms(mPSMaterialUniformBuffer, buffer);
            });
    }

    void GLShader::ResolveAndSetUniforms(const Ref<GLShaderUniformBufferDeclaration>& decl, Buffer buffer)
    {
        const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
        for (size_t i = 0; i < uniforms.size(); ++i)
        {
            GLShaderUniformDeclaration* uniform = (GLShaderUniformDeclaration*)uniforms[i];
            if (uniform->IsArray())
            {
                ResolveAndSetUniformArray(uniform, buffer);
            }
            else
            {
                ResolveAndSetUniform(uniform, buffer);
            }
        }
    }

    void GLShader::ResolveAndSetUniform(GLShaderUniformDeclaration* uniform, Buffer buffer)
    {
        if (uniform->GetLocation() == -1)
        {
            return;
        }

        NR_CORE_ASSERT(uniform->GetLocation() != -1, "Uniform has invalid location!");

        uint32_t offset = uniform->GetOffset();
        switch (uniform->GetType())
        {
        case GLShaderUniformDeclaration::Type::FLOAT32:
            UploadUniformFloat(uniform->GetLocation(), *(float*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::INT32:
            UploadUniformInt(uniform->GetLocation(), *(int32_t*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::VEC2:
            UploadUniformFloat2(uniform->GetLocation(), *(glm::vec2*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::VEC3:
            UploadUniformFloat3(uniform->GetLocation(), *(glm::vec3*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::VEC4:
            UploadUniformFloat4(uniform->GetLocation(), *(glm::vec4*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::MAT3:
            UploadUniformMat3(uniform->GetLocation(), *(glm::mat3*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::MAT4:
            UploadUniformMat4(uniform->GetLocation(), *(glm::mat4*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::STRUCT:
            UploadUniformStruct(uniform, buffer.Data, offset);
            break;
        default:
            NR_CORE_ASSERT(false, "Unknown uniform type!");
        }
    }

    void GLShader::ResolveAndSetUniformArray(GLShaderUniformDeclaration* uniform, Buffer buffer)
    {
        //NR_CORE_ASSERT(uniform->GetLocation() != -1, "Uniform has invalid location!");

        uint32_t offset = uniform->GetOffset();
        switch (uniform->GetType())
        {
        case GLShaderUniformDeclaration::Type::FLOAT32:
            UploadUniformFloat(uniform->GetLocation(), *(float*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::INT32:
            UploadUniformInt(uniform->GetLocation(), *(int32_t*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::VEC2:
            UploadUniformFloat2(uniform->GetLocation(), *(glm::vec2*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::VEC3:
            UploadUniformFloat3(uniform->GetLocation(), *(glm::vec3*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::VEC4:
            UploadUniformFloat4(uniform->GetLocation(), *(glm::vec4*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::MAT3:
            UploadUniformMat3(uniform->GetLocation(), *(glm::mat3*)&buffer.Data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::MAT4:
            UploadUniformMat4Array(uniform->GetLocation(), *(glm::mat4*)&buffer.Data[offset], uniform->GetCount());
            break;
        case GLShaderUniformDeclaration::Type::STRUCT:
            UploadUniformStruct(uniform, buffer.Data, offset);
            break;
        default:
            NR_CORE_ASSERT(false, "Unknown uniform type!");
        }
    }

    void GLShader::ResolveAndSetUniformField(const GLShaderUniformDeclaration& field, byte* data, int32_t offset)
    {
        switch (field.GetType())
        {
        case GLShaderUniformDeclaration::Type::FLOAT32:
            UploadUniformFloat(field.GetLocation(), *(float*)&data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::INT32:
            UploadUniformInt(field.GetLocation(), *(int32_t*)&data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::VEC2:
            UploadUniformFloat2(field.GetLocation(), *(glm::vec2*)&data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::VEC3:
            UploadUniformFloat3(field.GetLocation(), *(glm::vec3*)&data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::VEC4:
            UploadUniformFloat4(field.GetLocation(), *(glm::vec4*)&data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::MAT3:
            UploadUniformMat3(field.GetLocation(), *(glm::mat3*)&data[offset]);
            break;
        case GLShaderUniformDeclaration::Type::MAT4:
            UploadUniformMat4(field.GetLocation(), *(glm::mat4*)&data[offset]);
            break;
        default:
            NR_CORE_ASSERT(false, "Unknown uniform type!");
        }
    }

    void GLShader::UploadUniformBuffer(const UniformBufferBase& uniformBuffer)
    {
        for (unsigned int i = 0; i < uniformBuffer.GetUniformCount(); ++i)
        {
            const UniformDecl& decl = uniformBuffer.GetUniforms()[i];
            switch (decl.Type)
            {
            case UniformType::Float:
            {
                const std::string& name = decl.Name;
                float value = *(float*)(uniformBuffer.GetBuffer() + decl.Offset);
                Renderer::Submit([=]() {
                    UploadUniformFloat(name, value);
                    });
            }
            case UniformType::Float3:
            {
                const std::string& name = decl.Name;
                glm::vec3& values = *(glm::vec3*)(uniformBuffer.GetBuffer() + decl.Offset);
                Renderer::Submit([=]() {
                    UploadUniformFloat3(name, values);
                    });
            }
            case UniformType::Float4:
            {
                const std::string& name = decl.Name;
                glm::vec4& values = *(glm::vec4*)(uniformBuffer.GetBuffer() + decl.Offset);
                Renderer::Submit([=]() {
                    UploadUniformFloat4(name, values);
                    });
            }
            case UniformType::Matrix4x4:
            {
                const std::string& name = decl.Name;
                glm::mat4& values = *(glm::mat4*)(uniformBuffer.GetBuffer() + decl.Offset);
                Renderer::Submit([=]() {
                    UploadUniformMat4(name, values);
                    });
            }
            }
        }
    }

    void GLShader::SetInt(const std::string& name, int value)
    {
        Renderer::Submit([=]() {
            UploadUniformInt(name, value);
            });
    }

    void GLShader::SetFloat(const std::string& name, float value)
    {
        Renderer::Submit([=]() {
            UploadUniformFloat(name, value);
            });
    }

    void GLShader::SetFloat3(const std::string& name, const glm::vec3& value)
    {
        Renderer::Submit([=]() {
            UploadUniformFloat3(name, value);
            });
    }

    void GLShader::SetMat4(const std::string& name, const glm::mat4& value)
    {
        Renderer::Submit([=]() {
            UploadUniformMat4(name, value);
            });
    }

    void GLShader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind)
    {
        if (bind)
        {
            UploadUniformMat4(name, value);
        }
        else
        {
            int location = glGetUniformLocation(mID, name.c_str());
            if (location != -1)
            {
                UploadUniformMat4(location, value);
            }
        }
    }

    void GLShader::SetIntArray(const std::string& name, int* values, uint32_t size)
    {
        Renderer::Submit([=]() {
            UploadUniformIntArray(name, values, size);
            });
    }

    void GLShader::UploadUniformInt(uint32_t location, int32_t value)
    {
        glUniform1i(location, value);
    }

    void GLShader::UploadUniformIntArray(uint32_t location, int32_t* values, int32_t count)
    {
        glUniform1iv(location, count, values);
    }

    void GLShader::UploadUniformFloat(uint32_t location, float value)
    {
        glUniform1f(location, value);
    }

    void GLShader::UploadUniformFloat2(uint32_t location, const glm::vec2& value)
    {
        glUniform2f(location, value.x, value.y);
    }

    void GLShader::UploadUniformFloat3(uint32_t location, const glm::vec3& value)
    {
        glUniform3f(location, value.x, value.y, value.z);
    }

    void GLShader::UploadUniformFloat4(uint32_t location, const glm::vec4& value)
    {
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }

    void GLShader::UploadUniformMat3(uint32_t location, const glm::mat3& value)
    {
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }

    void GLShader::UploadUniformMat4(uint32_t location, const glm::mat4& value)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }

    void GLShader::UploadUniformMat4Array(uint32_t location, const glm::mat4& values, uint32_t count)
    {
        glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(values));
    }

    void GLShader::UploadUniformStruct(GLShaderUniformDeclaration* uniform, byte* buffer, uint32_t offset)
    {
        const ShaderStruct& s = uniform->GetShaderUniformStruct();
        const auto& fields = s.GetFields();
        for (size_t k = 0; k < fields.size(); ++k)
        {
            GLShaderUniformDeclaration* field = (GLShaderUniformDeclaration*)fields[k];
            ResolveAndSetUniformField(*field, buffer, offset);
            offset += field->mSize;
        }
    }

    void GLShader::UploadUniformInt(const std::string& name, int32_t value)
    {
        int32_t location = GetUniformLocation(name);
        glUniform1i(location, value);
    }

    void GLShader::UploadUniformIntArray(const std::string& name, int32_t* values, uint32_t count)
    {
        int32_t location = GetUniformLocation(name);
        glUniform1iv(location, count, values);
    }

    void GLShader::UploadUniformFloat(const std::string& name, float value)
    {
        glUseProgram(mID);

        auto location = glGetUniformLocation(mID, name.c_str());
        if (location != -1)
        {
            glUniform1f(location, value);
        }
        else
        {
            NR_LOG_UNIFORM("Uniform '{0}' not found!", name);
        }
    }

    void GLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& values)
    {
        glUseProgram(mID);
        auto location = glGetUniformLocation(mID, name.c_str());
        if (location != -1)
        {
            glUniform2f(location, values.x, values.y);
        }
        else
        {
            NR_LOG_UNIFORM("Uniform '{0}' not found!", name);
        }
    }

    void GLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& values)
    {
        glUseProgram(mID);
        auto location = glGetUniformLocation(mID, name.c_str());
        if (location != -1)
        {
            glUniform3f(location, values.x, values.y, values.z);
        }
        else
        {
            NR_LOG_UNIFORM("Uniform '{0}' not found!", name);
        }
    }

    void GLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
    {
        glUseProgram(mID);

        auto location = glGetUniformLocation(mID, name.c_str());
        if (location != -1)
        {
            glUniform4f(location, values.x, values.y, values.z, values.w);
        }
        else
        {
            NR_LOG_UNIFORM("Uniform '{0}' not found!", name);
        }
    }

    void GLShader::UploadUniformMat4(const std::string& name, const glm::mat4& values)
    {
        glUseProgram(mID);
        auto location = glGetUniformLocation(mID, name.c_str());
        if (location != -1)
        {
            glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)&values);
        }
        else
        {
            NR_LOG_UNIFORM("Uniform '{0}' not found!", name);
        }
    }
}