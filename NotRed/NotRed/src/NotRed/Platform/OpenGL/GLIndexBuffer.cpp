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

		Ref<GLIndexBuffer> instance = this;
		Renderer::Submit([instance]() mutable {
			glCreateBuffers(1, &instance->mID);
			glNamedBufferData(instance->mID, instance->mSize, instance->mLocalData.Data, GL_STATIC_DRAW);
			});
	}

	GLIndexBuffer::GLIndexBuffer(uint32_t size)
		: mSize(size)
	{
		Ref<GLIndexBuffer> instance = this;
		Renderer::Submit([instance]() mutable {
			glCreateBuffers(1, &instance->mID);
			glNamedBufferData(instance->mID, instance->mSize, nullptr, GL_DYNAMIC_DRAW);
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

		Ref<GLIndexBuffer> instance = this;
		Renderer::Submit([instance, offset]() {
			glNamedBufferSubData(instance->mID, offset, instance->mSize, instance->mLocalData.Data);
			});
	}

	void GLIndexBuffer::Bind() const
	{
		Renderer::Submit([this]() {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mID);
			});
	}
}