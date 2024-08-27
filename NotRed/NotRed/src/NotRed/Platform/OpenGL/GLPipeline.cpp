#include "nrpch.h"
#include "GLPipeline.h"

#include <glad/glad.h>

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
	static GLenum ToGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:    return GL_FLOAT;
		case ShaderDataType::Float2:   return GL_FLOAT;
		case ShaderDataType::Float3:   return GL_FLOAT;
		case ShaderDataType::Float4:   return GL_FLOAT;
		case ShaderDataType::Mat3:     return GL_FLOAT;
		case ShaderDataType::Mat4:     return GL_FLOAT;
		case ShaderDataType::Int:      return GL_INT;
		case ShaderDataType::Int2:     return GL_INT;
		case ShaderDataType::Int3:     return GL_INT;
		case ShaderDataType::Int4:     return GL_INT;
		case ShaderDataType::Bool:     return GL_BOOL;
		}

		NR_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	GLPipeline::GLPipeline(const PipelineSpecification& spec)
		: mSpecification(spec)
	{
		Invalidate();
	}

	GLPipeline::~GLPipeline()
	{
		GLuint rendererID = mID;
		Renderer::Submit([rendererID]()
			{
				glDeleteVertexArrays(1, &rendererID);
			});
	}

	void GLPipeline::Invalidate()
	{
		NR_CORE_ASSERT(mSpecification.Layout.GetElements().size(), "Layout is empty!");

		Ref<GLPipeline> instance = this;
		Renderer::Submit([instance]() mutable
			{
				auto& vertexArrayRendererID = instance->mID;

				if (vertexArrayRendererID)
					glDeleteVertexArrays(1, &vertexArrayRendererID);

				glGenVertexArrays(1, &vertexArrayRendererID);
				glBindVertexArray(vertexArrayRendererID);
				glBindVertexArray(0);
			});
	}

	void GLPipeline::Bind()
	{
		Ref<GLPipeline> instance = this;
		Renderer::Submit([instance]()
			{
				glBindVertexArray(instance->mID);

				const auto& layout = instance->mSpecification.Layout;
				uint32_t attribIndex = 0;
				for (const auto& element : layout)
				{
					auto glBaseType = ToGLBaseType(element.Type);
					glEnableVertexAttribArray(attribIndex);
					if (glBaseType == GL_INT)
					{
						glVertexAttribIPointer(attribIndex,
							element.GetComponentCount(),
							glBaseType,
							layout.GetStride(),
							(const void*)(intptr_t)element.Offset);
					}
					else
					{
						glVertexAttribPointer(attribIndex,
							element.GetComponentCount(),
							glBaseType,
							element.Normalized ? GL_TRUE : GL_FALSE,
							layout.GetStride(),
							(const void*)(intptr_t)element.Offset);
					}
					++attribIndex;
				}
			});
	}

}