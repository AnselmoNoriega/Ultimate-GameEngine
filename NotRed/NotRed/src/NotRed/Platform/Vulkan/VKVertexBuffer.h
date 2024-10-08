#pragma once

#include "NotRed/Renderer/VertexBuffer.h"

#include "NotRed/Core/Buffer.h"

#include "VKAllocator.h"

namespace NR
{
	class VKVertexBuffer : public VertexBuffer
	{
	public:
		VKVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
		VKVertexBuffer(uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);

		~VKVertexBuffer() override;

		void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override {}
		void Bind() const override {}

		const VertexBufferLayout& GetLayout() const override { return {}; }
		void SetLayout(const VertexBufferLayout& layout) override {}

		unsigned int GetSize() const override { return mSize; }
		RendererID GetRendererID() const override { return 0; }

		VkBuffer GetVulkanBuffer() { return mVulkanBuffer; }

	private:
		uint32_t mSize = 0;
		Buffer mLocalData;

		VkBuffer mVulkanBuffer = nullptr;
		VmaAllocation mMemoryAllocation;
	};
}