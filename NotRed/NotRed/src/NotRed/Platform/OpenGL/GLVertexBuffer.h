#pragma once

#include "NotRed/Renderer/VertexBuffer.h"

namespace NR
{
	class GLVertexBuffer : public VertexBuffer
	{
	public:
		GLVertexBuffer(uint32_t size);
		~GLVertexBuffer() override;

		void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;
		void Bind() const override;

		uint32_t GetSize() const override { return mSize; }
		RendererID GetRendererID() const override { return mID; }

	private:
		RendererID mID;
		uint32_t mSize;
	};
}