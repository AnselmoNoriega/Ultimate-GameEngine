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
		Renderer::Submit([this]() {
			glDeleteVertexArrays(1, &mID);
			});
	}

	void GLVertexArray::Bind() const
	{
		Renderer::Submit([this]() {
			glBindVertexArray(mID);
			});
	}

	void GLVertexArray::Unbind() const
	{
		Renderer::Submit([this]() {
			glBindVertexArray(0);
			});
	}

	void GLVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
	{
		NR_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

		Bind();
		vertexBuffer->Bind();

		Renderer::Submit([this, vertexBuffer]() {
			const auto & layout = vertexBuffer->GetLayout();
			for (const auto& element : layout)
			{
				auto glBaseType = ShaderDataTypeToOpenGLBaseType(element.Type);
				glEnableVertexAttribArray(mVertexBufferIndex);
				if (glBaseType == GL_INT)
				{
					glVertexAttribIPointer(mVertexBufferIndex,
						element.GetComponentCount(),
						glBaseType,
						layout.GetStride(),
						(const void*)(intptr_t)element.Offset);
				}
				else
				{
					glVertexAttribPointer(mVertexBufferIndex,
						element.GetComponentCount(),
						glBaseType,
						element.Normalized ? GL_TRUE : GL_FALSE,
						layout.GetStride(),
						(const void*)(intptr_t)element.Offset);
				}
				mVertexBufferIndex++;
			}
			});
		mVertexBuffers.push_back(vertexBuffer);
	}

	void GLVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
	{
		Bind();
		indexBuffer->Bind();

		mIndexBuffer = indexBuffer;
	}

}