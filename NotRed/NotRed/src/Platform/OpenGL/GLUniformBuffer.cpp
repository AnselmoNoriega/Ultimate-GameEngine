#include "nrpch.h"
#include "GLUniformBuffer.h"

#include <glad/glad.h>

namespace NR
{
	GLUniformBuffer::GLUniformBuffer(uint32_t size, uint32_t binding)
	{
		glCreateBuffers(1, &mRendererID);
		glNamedBufferData(mRendererID, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, mRendererID);
	}

	GLUniformBuffer::~GLUniformBuffer()
	{
		glDeleteBuffers(1, &mRendererID);
	}

	void GLUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(mRendererID, offset, size, data);
	}
}