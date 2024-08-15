#pragma once

#include "NotRed/Renderer/IndexBuffer.h"

namespace NR
{
	class NOT_RED_API GLIndexBuffer : public IndexBuffer
	{
	public:
		GLIndexBuffer(uint32_t size);
		~GLIndexBuffer() override;

		void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;
		void Bind() const override;

		uint32_t GetSize() const  override { return mSize; }
		RendererID GetRendererID() const  override { return mID; }

		uint32_t GetCount() const override { return mSize / sizeof(uint32_t); }

	private:
		RendererID mID;
		uint32_t mSize;
	};
}