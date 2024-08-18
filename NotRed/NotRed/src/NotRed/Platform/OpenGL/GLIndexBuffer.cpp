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

		Renderer::Submit([this]() {
			glCreateBuffers(1, &mID);
			glNamedBufferData(mID, mSize, mLocalData.Data, GL_STATIC_DRAW);
			});
	}

	GLIndexBuffer::~GLIndexBuffer()
	{
		Renderer::Submit([this]() {
			glDeleteBuffers(1, &mID);
			});
	}

	void GLIndexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
	{
		mLocalData = Buffer::Copy(data, size);
		mSize = size;

		Renderer::Submit([this, offset]() {
			glNamedBufferSubData(mID, offset, mSize, mLocalData.Data);
			});
	}

	void GLIndexBuffer::Bind() const
	{
		Renderer::Submit([this]() {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mID);
			});
	}
}