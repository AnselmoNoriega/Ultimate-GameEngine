#include "nrpch.h"

#include "NotRed/Platform/OpenGL/GLVertexBuffer.h"

namespace NR
{
	Ref<VertexBuffer> VertexBuffer::Create(void* data, uint32_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    return nullptr;
		case RendererAPIType::OpenGL:  return std::make_shared<GLVertexBuffer>(data, size, usage);
		default:
		{
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
		}
		}
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    return nullptr;
		case RendererAPIType::OpenGL:  return std::make_shared<GLVertexBuffer>(size, usage);
		default:
		{
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
		}
		}
	}
}
