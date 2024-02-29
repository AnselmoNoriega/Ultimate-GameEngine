#include "nrpch.h"
#include "GLVertexArray.h"

#include <glad/glad.h>

namespace NR
{
    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
        case NR::ShaderDataType::None:    return GL_FLOAT;
        case NR::ShaderDataType::Float:   return GL_FLOAT;
        case NR::ShaderDataType::Float2:  return GL_FLOAT;
        case NR::ShaderDataType::Float3:  return GL_FLOAT;
        case NR::ShaderDataType::Float4:  return GL_FLOAT;
        case NR::ShaderDataType::Mat3:    return GL_FLOAT;
        case NR::ShaderDataType::Mat4:    return GL_FLOAT;
        default:
        {
            NR_CORE_ASSERT(false, "Unknown ShaderDataType!");
            return 0;
        }
        }
    }

    GLVertexArray::GLVertexArray()
    {
        glCreateVertexArrays(1, &mID);
    }

    void GLVertexArray::Bind() const
    {
        glBindVertexArray(mID);
    }

    void GLVertexArray::Unbind() const
    {
        glBindVertexArray(0);
    }

    void GLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexbuffer)
    {
        NR_CORE_ASSERT(vertexbuffer->GetLayout().GetElements().size(), "VertexBuffer's layout is empty!")

        glBindVertexArray(mID);
        vertexbuffer->Bind();

        uint32_t index = 0;
        for (const auto& element : vertexbuffer->GetLayout())
        {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(
                index,
                element.GetComponentCount(),
                ShaderDataTypeToOpenGLBaseType(element.Type),
                element.Normalized ? GL_TRUE : GL_FALSE,
                vertexbuffer->GetLayout().GetStride(),
                (const void*)element.Offset
            );
            ++index;
        }

        mVertexBuffers.push_back(vertexbuffer);
    }

    void GLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
    {
        glBindVertexArray(mID);
        indexBuffer->Bind();

        mIndexBuffer = indexBuffer;
    }

    GLVertexArray::~GLVertexArray()
    {
        glDeleteVertexArrays(1, &mID);
    }
}