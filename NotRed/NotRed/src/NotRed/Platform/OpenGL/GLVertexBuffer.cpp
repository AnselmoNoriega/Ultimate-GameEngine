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

        Ref<GLVertexBuffer> instance = this;
        Renderer::Submit([instance]() mutable
            {
                glCreateBuffers(1, &instance->mID);
                glNamedBufferData(instance->mID, instance->mSize, instance->mLocalData.Data, OpenGLUsage(instance->mUsage));
            });
    }

    GLVertexBuffer::GLVertexBuffer(uint32_t size, VertexBufferUsage usage)
        : mSize(size), mUsage(usage)
    {
        Ref<GLVertexBuffer> instance = this;
        Renderer::Submit([instance]() mutable
            {
                glCreateBuffers(1, &instance->mID);
                glNamedBufferData(instance->mID, instance->mSize, nullptr, OpenGLUsage(instance->mUsage));
            });
    }

    GLVertexBuffer::~GLVertexBuffer()
    {
        GLuint rendererID = mID;
        Renderer::Submit([rendererID]() {
            glDeleteBuffers(1, &rendererID);
            });
    }

    void GLVertexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
    {
        mLocalData = Buffer::Copy(data, size);
        mSize = size;

        Ref<GLVertexBuffer> instance = this;
        Renderer::Submit([instance, offset]() {
            glNamedBufferSubData(instance->mID, offset, instance->mSize, instance->mLocalData.Data);
            });
    }

    void GLVertexBuffer::Bind() const
    {
        Ref<const GLVertexBuffer> instance = this;
        Renderer::Submit([instance]() {
            glBindBuffer(GL_ARRAY_BUFFER, instance->mID);
            });
    }

}