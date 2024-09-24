#include "nrpch.h"
#include "GLUniformBuffer.h"

#include <glad/glad.h>

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
	GLUniformBuffer::GLUniformBuffer(uint32_t size, uint32_t binding)
		: mSize(size), mBinding(binding)
	{
		mLocalStorage = new uint8_t[size];

		Ref<GLUniformBuffer> instance = this;
		Renderer::Submit([instance, size, binding]() mutable
			{
				glCreateBuffers(1, &instance->mID);
				glNamedBufferData(instance->mID, size, nullptr, GL_DYNAMIC_DRAW);
				glBindBufferBase(GL_UNIFORM_BUFFER, binding, instance->mID);
			});
	}

	GLUniformBuffer::~GLUniformBuffer()
	{
		delete[] mLocalStorage;

		RendererID rendererID = mID;
		Renderer::Submit([rendererID]()
			{
				glDeleteBuffers(1, &rendererID);
			});
	}

	void GLUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		memcpy(mLocalStorage, data, size);
		Ref<GLUniformBuffer> instance = this;
		Renderer::Submit([instance, size, offset]() mutable
			{
				glNamedBufferSubData(instance->mID, offset, size, instance->mLocalStorage);
			});
	}

}