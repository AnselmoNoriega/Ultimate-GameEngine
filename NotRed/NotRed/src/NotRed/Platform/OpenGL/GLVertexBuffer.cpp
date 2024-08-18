#include "nrpch.h"
#include "GLVertexBuffer.h"

#include <Glad/glad.h>

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
    static GLenum OpenGLUsage(VertexBufferUsage usage)
    {
        switch (usage)
        {
        case VertexBufferUsage::Static:    return GL_STATIC_DRAW;
        case VertexBufferUsage::Dynamic:   return GL_DYNAMIC_DRAW;
        default:
        {
            NR_CORE_ASSERT(false, "Unknown vertex buffer usage");
            return 0;
        }
        }
    }

    GLVertexBuffer::GLVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage)
        : mSize(size), mUsage(usage)
    {
        mLocalData = Buffer::Copy(data, size);

        Renderer::Submit([=]() {
            glCreateBuffers(1, &mID);
            glNamedBufferData(mID, mSize, mLocalData.Data, OpenGLUsage(mUsage));
            });
    }

    GLVertexBuffer::GLVertexBuffer(uint32_t size, VertexBufferUsage usage)
        : mSize(size), mUsage(usage)
    {
        Renderer::Submit([this]() {
            glCreateBuffers(1, &mID);
            glNamedBufferData(mID, mSize, nullptr, OpenGLUsage(mUsage));
            });
    }

    GLVertexBuffer::~GLVertexBuffer()
    {
        Renderer::Submit([this]() {
            glDeleteBuffers(1, &mID);
            });
    }

    void GLVertexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
    {
        mLocalData = Buffer::Copy(data, size);
        mSize = size;

        Renderer::Submit([this, offset]() {
            glNamedBufferSubData(mID, offset, mSize, mLocalData.Data);
            });
    }

    void GLVertexBuffer::Bind() const
    {
        Renderer::Submit([this]() {
            glBindBuffer(GL_ARRAY_BUFFER, mID);
            });
    }

}