#include "nrpch.h"
#include "GLVertexArray.h"

#include <glad/glad.h>

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
	static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
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

	GLVertexArray::GLVertexArray()
	{
		Renderer::Submit([this]() {
			glCreateVertexArrays(1, &mID);
			});
	}

	GLVertexArray::~GLVertexArray()
	{
		GLuint rendererID = mID;
		Renderer::Submit([rendererID]() {
			glDeleteVertexArrays(1, &rendererID);
			});
	}

	void GLVertexArray::Bind() const
	{
		Ref<const GLVertexArray> instance = this;
		Renderer::Submit([instance]() {
			glBindVertexArray(instance->mID);
			});
	}

	void GLVertexArray::Unbind() const
	{
		Ref<const GLVertexArray> instance = this;
		Renderer::Submit([instance]() {
			glBindVertexArray(0);
			});
	}

	void GLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
	{
		NR_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

		Bind();
		vertexBuffer->Bind();

		Ref<GLVertexArray> instance = this;
		Renderer::Submit([instance, vertexBuffer]() mutable {
			const auto & layout = vertexBuffer->GetLayout();
			for (const auto& element : layout)
			{
				auto glBaseType = ShaderDataTypeToOpenGLBaseType(element.Type);
				glEnableVertexAttribArray(instance->mVertexBufferIndex);
				if (glBaseType == GL_INT)
				{
					glVertexAttribIPointer(instance->mVertexBufferIndex,
						element.GetComponentCount(),
						glBaseType,
						layout.GetStride(),
						(const void*)(intptr_t)element.Offset);
				}
				else
				{
					glVertexAttribPointer(instance->mVertexBufferIndex,
						element.GetComponentCount(),
						glBaseType,
						element.Normalized ? GL_TRUE : GL_FALSE,
						layout.GetStride(),
						(const void*)(intptr_t)element.Offset);
				}
				instance->mVertexBufferIndex++;
			}
			});
		mVertexBuffers.push_back(vertexBuffer);
	}

	void GLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
	{
		Bind();
		indexBuffer->Bind();

		mIndexBuffer = indexBuffer;
	}

}