#pragma once

#include "NotRed/Renderer/UniformBuffer.h"

#include "VKAllocator.h"

namespace NR
{
	class VKUniformBuffer : public UniformBuffer
	{
	public:
		VKUniformBuffer(uint32_t size, uint32_t binding);
		~VKUniformBuffer() override;

		void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		uint32_t GetBinding() const override { return mBinding; }

		const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return mDescriptor; }

	private:
		void RT_Invalidate();
		void RT_SetData(const void* data, uint32_t size, uint32_t offset);

	private:
		std::string mName;
		VkBuffer mBuffer;
		VmaAllocation mMemoryAlloc = nullptr;
		VkDescriptorBufferInfo mDescriptor;
		uint32_t mSize = 0;
		uint32_t mBinding = 0;
		VkShaderStageFlagBits mShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

		uint8_t* mLocalStorage = nullptr;
	};
}