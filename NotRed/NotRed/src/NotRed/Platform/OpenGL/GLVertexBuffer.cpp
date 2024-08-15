#include "nrpch.h"
#include "GLVertexBuffer.h"

#include <Glad/glad.h>

namespace NR
{
    GLVertexBuffer::GLVertexBuffer(uint32_t size)
        : mID(0), mSize(size)
    {
        NR_RENDER_S({
            glGenBuffers(1, &self->mID);
            });
    }

    GLVertexBuffer::~GLVertexBuffer()
    {
        NR_RENDER_S({
            glDeleteBuffers(1, &self->mID);
            });
    }

    void GLVertexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset)
    {
        mSize = size;

        NR_RENDER_S3(buffer, size, offset, {
            glBindBuffer(GL_ARRAY_BUFFER, self->mID);
            glBufferData(GL_ARRAY_BUFFER, size, buffer, GL_STATIC_DRAW);
            });
    }

    void GLVertexBuffer::Bind() const
    {
        NR_RENDER_S({
            glBindBuffer(GL_ARRAY_BUFFER, self->mID);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (const void*)(3 * sizeof(float)));
            });
    }

}