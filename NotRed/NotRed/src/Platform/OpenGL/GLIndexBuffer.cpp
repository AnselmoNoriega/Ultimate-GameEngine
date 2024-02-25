#include "nrpch.h"
#include "GLIndexBuffer.h"

#include <glad/glad.h>

namespace NR
{
    GLIndexBuffer::GLIndexBuffer(uint32_t* indices, uint32_t count)
        : mCount(count)
    {
        glCreateBuffers(1, &mID);
        glNamedBufferData(mID, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
        Bind();
    }

    void GLIndexBuffer::Bind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mID);
    }

    void GLIndexBuffer::Unbind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    GLIndexBuffer::~GLIndexBuffer()
    {
        glDeleteBuffers(1, &mID);
    }
}