#include "nrpch.h"
#include "GLVertexBuffer.h"

#include <glad/glad.h>

namespace NR
{
    GLVertexBuffer::GLVertexBuffer(float* vertices, uint32_t size)
    {
        NR_PROFILE_FUNCTION();

        glCreateBuffers(1, &mID);
        glNamedBufferData(mID, size, vertices, GL_STATIC_DRAW);
        Bind();
    }

    void GLVertexBuffer::Bind() const
    {
        NR_PROFILE_FUNCTION();

        glBindBuffer(GL_ARRAY_BUFFER, mID);
    }

    void GLVertexBuffer::Unbind() const
    {
        NR_PROFILE_FUNCTION();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLVertexBuffer::~GLVertexBuffer()
    {
        NR_PROFILE_FUNCTION();

        glDeleteBuffers(1, &mID);
    }
}