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

		Ref<GLUniformBuffer> instance(this);
		Renderer::Submit([instance, size, binding]() mutable
			{
				glCreateBuffers(1, &instance->mRendererID);
				glNamedBufferData(instance->mRendererID, size, nullptr, GL_DYNAMIC_DRAW);
				glBindBufferBase(GL_UNIFORM_BUFFER, binding, instance->mRendererID);
			});
	}

	GLUniformBuffer::~GLUniformBuffer()
	{
		delete[] mLocalStorage;

		uint32_t rendererID = mRendererID;
		Renderer::Submit([rendererID]()
			{
				glDeleteBuffers(1, &rendererID);
			});
	}

	void GLUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		memcpy(mLocalStorage, data, size);
		Ref<GLUniformBuffer> instance(this);
		Renderer::Submit([instance, size, offset]() mutable
			{
				instance->RT_SetData(instance->mLocalStorage, size, offset);
			});
	}

	void GLUniformBuffer::RT_SetData(const void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(mRendererID, offset, size, mLocalStorage);
	}

}