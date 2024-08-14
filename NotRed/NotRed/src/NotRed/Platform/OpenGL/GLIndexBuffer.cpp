#include "nrpch.h"
#include "GLIndexBuffer.h"

#include <Glad/glad.h>

namespace NR
{
	GLIndexBuffer::GLIndexBuffer(uint32_t size)
		: mID(0), mSize(size)
	{
		NR_RENDER_S({
			glGenBuffers(1, &self->mID);
			});
	}

	GLIndexBuffer::~GLIndexBuffer()
	{
		NR_RENDER_S({
			glDeleteBuffers(1, &self->mID);
			});
	}

	void GLIndexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset)
	{
		NR_RENDER_S3(buffer, size, offset, {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->mID);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, buffer, GL_STATIC_DRAW);
			});
	}

	void GLIndexBuffer::Bind() const
	{
		NR_RENDER_S({
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->mID);
			});
	}
}