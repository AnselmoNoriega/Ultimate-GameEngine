#pragma once

#include "NotRed/Renderer/VertexBuffer.h"
#include "NotRed/Core/Buffer.h"

namespace NR
{
	class GLVertexBuffer : public VertexBuffer
	{
	public:
		GLVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
		GLVertexBuffer(uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);
		~GLVertexBuffer() override;

		void SetData(void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void RT_SetData(void* data, uint32_t size, uint32_t offset = 0) {}
		void Bind() const override;

		uint32_t GetSize() const override { return mSize; }

		RendererID GetRendererID() const override { return mID; }

	private:
		RendererID mID = 0;
		uint32_t mSize;
		VertexBufferUsage mUsage;
		VertexBufferLayout mLayout;

		Buffer mLocalData;
	};
}