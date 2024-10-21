#pragma once

#include "NotRed/Renderer/IndexBuffer.h"

#include "VKAllocator.h"

#include "NotRed/Core/Buffer.h"

namespace NR
{
	class VKIndexBuffer : public IndexBuffer
	{
	public:
		VKIndexBuffer(uint32_t size);
		VKIndexBuffer(void* data, uint32_t size = 0);
		~VKIndexBuffer() override;

		void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;
		void Bind() const override;

		virtual uint32_t GetCount() const override { return mSize / sizeof(uint32_t); }

		uint32_t GetSize() const override { return mSize; }
		RendererID GetRendererID() const override;

		VkBuffer GetVulkanBuffer() { return mVulkanBuffer; }

	private:
		uint32_t mSize = 0;
		Buffer mLocalData;

		VkBuffer mVulkanBuffer = nullptr;
		VmaAllocation mMemoryAllocation;
	};
}