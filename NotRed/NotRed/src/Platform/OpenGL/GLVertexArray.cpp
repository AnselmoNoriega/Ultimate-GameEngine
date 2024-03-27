#include "nrpch.h"
#include "GLVertexArray.h"

#include <glad/glad.h>

namespace NR
{
    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
        case NR::ShaderDataType::Float:   return GL_FLOAT;
        case NR::ShaderDataType::Float2:  return GL_FLOAT;
        case NR::ShaderDataType::Float3:  return GL_FLOAT;
        case NR::ShaderDataType::Float4:  return GL_FLOAT;
        case NR::ShaderDataType::Int:     return GL_INT;
        case NR::ShaderDataType::Mat3:    return GL_FLOAT;
        case NR::ShaderDataType::Mat4:    return GL_FLOAT;
        case NR::ShaderDataType::None:
        default:
        {
            NR_CORE_ASSERT(false, "Unknown ShaderDataType!");
            return 0;
        }
        }
    }

    GLVertexArray::GLVertexArray()
    {
        NR_PROFILE_FUNCTION();

        glCreateVertexArrays(1, &mID);
    }

    void GLVertexArray::Bind() const
    {
        NR_PROFILE_FUNCTION();

        glBindVertexArray(mID);
    }

    void GLVertexArray::Unbind() const
    {
        NR_PROFILE_FUNCTION();

        glBindVertexArray(0);
    }

    void GLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexbuffer)
    {
        NR_PROFILE_FUNCTION();

        NR_CORE_ASSERT(vertexbuffer->GetLayout().GetElements().size(), "VertexBuffer's layout is empty!")

        glBindVertexArray(mID);
        vertexbuffer->Bind();

        for (const auto& element : vertexbuffer->GetLayout())
        {
            switch (element.Type)
            {
            case NR::ShaderDataType::Float: 
            case NR::ShaderDataType::Float2:
            case NR::ShaderDataType::Float3:
            case NR::ShaderDataType::Float4:
            {
                glEnableVertexAttribArray(mVertexBufferIndex);
                glVertexAttribPointer(
                    mVertexBufferIndex,
                    element.GetComponentCount(),
                    ShaderDataTypeToOpenGLBaseType(element.Type),
                    element.Normalized ? GL_TRUE : GL_FALSE,
                    vertexbuffer->GetLayout().GetStride(),
                    (const void*)element.Offset
                );
                ++mVertexBufferIndex;
                break;
            }
            case NR::ShaderDataType::Int:   
            {
                glEnableVertexAttribArray(mVertexBufferIndex);
                glVertexAttribIPointer(
                    mVertexBufferIndex,
                    element.GetComponentCount(),
                    ShaderDataTypeToOpenGLBaseType(element.Type),
                    vertexbuffer->GetLayout().GetStride(),
                    (const void*)element.Offset
                );
                ++mVertexBufferIndex;
                break;
            }
            case NR::ShaderDataType::Mat3:  
            case NR::ShaderDataType::Mat4:
            {
                int count = element.GetComponentCount();
                for (uint8_t i = 0; i < count; ++i)
                {
                    glEnableVertexAttribArray(mVertexBufferIndex);
                    glVertexAttribPointer(
                        mVertexBufferIndex,
                        count,
                        ShaderDataTypeToOpenGLBaseType(element.Type),
                        element.Normalized ? GL_TRUE : GL_FALSE,
                        vertexbuffer->GetLayout().GetStride(),
                        (const void*)(element.Offset + sizeof(float) * count * i)
                    );
                    ++mVertexBufferIndex;
                }
                break;
            }
            default:
            {
                NR_CORE_ASSERT(false, "Unknown ShaderDataType!");
                break;
            }
            }
        }

        mVertexBuffers.push_back(vertexbuffer);
    }

    void GLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
    {
        NR_PROFILE_FUNCTION();

        glBindVertexArray(mID);
        indexBuffer->Bind();

        mIndexBuffer = indexBuffer;
    }

    GLVertexArray::~GLVertexArray()
    {
        NR_PROFILE_FUNCTION();

        glDeleteVertexArrays(1, &mID);
    }
}