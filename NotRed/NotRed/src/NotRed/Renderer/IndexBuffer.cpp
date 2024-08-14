#include "nrpch.h"
#include "IndexBuffer.h"

#include "NotRed/Platform/OpenGL/GLIndexBuffer.h"

namespace NR
{
	IndexBuffer* IndexBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    return nullptr;
		case RendererAPIType::OpenGL:  return new GLIndexBuffer(size);
		}
		return nullptr;

	}
}