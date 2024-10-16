#pragma once

#include "NotRed/Renderer/StorageBuffer.h"
#include "VKAllocator.h"

namespace NR
{
	class VKStorageBuffer : public StorageBuffer
	{
	public:
		VKStorageBuffer(uint32_t size, uint32_t binding);

		void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		void RT_SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		uint32_t GetBinding() const override { return mBinding; }

		void Resize(uint32_t newSize) override;

		const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return mDescriptorInfo; }

	private:
		void RT_Invalidate();

	private:
		VmaAllocation mMemoryAlloc = nullptr;
		VkBuffer mBuffer{};
		VkDescriptorBufferInfo mDescriptorInfo{};

		uint32_t mSize = 0;
		uint32_t mBinding = 0;

		std::string mName;

		VkShaderStageFlagBits mShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

		uint8_t* mLocalStorage = nullptr;
	};
}