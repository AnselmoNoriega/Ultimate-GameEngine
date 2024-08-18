#include "nrpch.h"
#include "VertexArray.h"

#include "Renderer.h"
#include "NotRed/Platform/OpenGL/GLVertexArray.h"

namespace NR
{
	Ref<VertexArray> VertexArray::Create()
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    NR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPIType::OpenGL:  return std::make_shared<GLVertexArray>();
		default:
		{
			NR_CORE_ASSERT(false, "Unknown RendererAPI");
			return nullptr;
		}
		}
	}
}