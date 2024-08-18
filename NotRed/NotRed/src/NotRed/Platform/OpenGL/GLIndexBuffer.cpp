#include "nrpch.h"
#include "GLIndexBuffer.h"

#include <Glad/glad.h>

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
	GLIndexBuffer::GLIndexBuffer(void* data, uint32_t size)
		: mID(0), mSize(size)
	{
		mLocalData = Buffer::Copy(data, size);

		NR_RENDER_S({
			glCreateBuffers(1, &self->mID);
			glNamedBufferData(self->mID, self->mSize, self->mLocalData.Data, GL_STATIC_DRAW);
			});
	}

	GLIndexBuffer::~GLIndexBuffer()
	{
		NR_RENDER_S({
			glDeleteBuffers(1, &self->mID);
			});
	}

	void GLIndexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
	{
		mLocalData = Buffer::Copy(data, size);
		mSize = size;

		NR_RENDER_S1(offset, {
			glNamedBufferSubData(self->mID, offset, self->mSize, self->mLocalData.Data);
			});
	}

	void GLIndexBuffer::Bind() const
	{
		NR_RENDER_S({
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->mID);
			});
	}
}