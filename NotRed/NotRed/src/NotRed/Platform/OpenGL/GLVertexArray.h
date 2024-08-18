#pragma once

#include "NotRed/Renderer/VertexArray.h"

namespace NR
{
	class GLVertexArray : public VertexArray
	{
	public:
		GLVertexArray();
		virtual ~GLVertexArray();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer) override;
		virtual void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) override;

		virtual const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const { return mVertexBuffers; }
		virtual const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const { return mIndexBuffer; }

	private:
		RendererID mID = 0;
		uint32_t mVertexBufferIndex = 0;
		std::vector<std::shared_ptr<VertexBuffer>> mVertexBuffers;
		std::shared_ptr<IndexBuffer> mIndexBuffer;
	};
}