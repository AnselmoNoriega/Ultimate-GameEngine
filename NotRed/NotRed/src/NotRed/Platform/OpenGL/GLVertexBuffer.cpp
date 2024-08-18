#include "nrpch.h"
#include "GLVertexBuffer.h"

#include <Glad/glad.h>

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

        NR_RENDER_S({
            glCreateBuffers(1, &self->mID);
            glNamedBufferData(self->mID, self->mSize, self->mLocalData.Data, OpenGLUsage(self->mUsage));
            });
    }

    GLVertexBuffer::GLVertexBuffer(uint32_t size, VertexBufferUsage usage)
        : mSize(size), mUsage(usage)
    {
        NR_RENDER_S({
            glCreateBuffers(1, &self->mID);
            glNamedBufferData(self->mID, self->mSize, nullptr, OpenGLUsage(self->mUsage));
            });
    }

    GLVertexBuffer::~GLVertexBuffer()
    {
        NR_RENDER_S({
            glDeleteBuffers(1, &self->mID);
            });
    }

    void GLVertexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
    {
        mLocalData = Buffer::Copy(data, size);
        mSize = size;

        NR_RENDER_S1(offset, {
            glNamedBufferSubData(self->mID, offset, self->mSize, self->mLocalData.Data);
            });
    }

    void GLVertexBuffer::Bind() const
    {
        NR_RENDER_S({
            glBindBuffer(GL_ARRAY_BUFFER, self->mID);
            });
    }

}