#include "nrpch.h"

#include "NotRed/Platform/OpenGL/GLVertexBuffer.h"

namespace NR
{
	VertexBuffer* VertexBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    return nullptr;
		case RendererAPIType::OpenGL:  return new GLVertexBuffer(size);
		default:
		{
			return nullptr;
		}
		}
	}
}
