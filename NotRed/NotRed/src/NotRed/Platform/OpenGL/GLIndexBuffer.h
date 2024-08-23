#pragma once

#include "NotRed/Renderer/IndexBuffer.h"
#include "NotRed/Core/Buffer.h"

namespace NR
{
	class GLIndexBuffer : public IndexBuffer
	{
	public:
		GLIndexBuffer(uint32_t size);
		GLIndexBuffer(void* data, uint32_t size);
		~GLIndexBuffer() override;

		void SetData(void* data, uint32_t size, uint32_t offset = 0) override;
		void Bind() const override;

		uint32_t GetSize() const  override { return mSize; }
		RendererID GetRendererID() const  override { return mID; }

		uint32_t GetCount() const override { return mSize / sizeof(uint32_t); }

	private:
		RendererID mID = 0;
		uint32_t mSize;

		Buffer mLocalData;
	};
}