#include "nrpch.h"
#include "GLVertexBuffer.h"

#include <glad/glad.h>

namespace NR
{
    GLVertexBuffer::GLVertexBuffer(float* vertices, uint32_t size)
    {
        glCreateBuffers(1, &mID);
        glNamedBufferData(mID, size, vertices, GL_STATIC_DRAW);
        Bind();
    }

    void GLVertexBuffer::Bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, mID);
    }

    void GLVertexBuffer::Unbind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLVertexBuffer::~GLVertexBuffer()
    {
        glDeleteBuffers(1, &mID);
    }
}