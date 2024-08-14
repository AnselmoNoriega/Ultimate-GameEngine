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
		NR_RENDER_S3(buffer, size, offset, {
			glBindBuffer(GL_ARRAY_BUFFER, self->mID);
			glBufferData(GL_ARRAY_BUFFER, size, buffer, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
			});
	}

	void GLVertexBuffer::Bind() const
	{
		NR_RENDER_S({
			glBindBuffer(GL_ARRAY_BUFFER, self->mID);
			});
	}

}