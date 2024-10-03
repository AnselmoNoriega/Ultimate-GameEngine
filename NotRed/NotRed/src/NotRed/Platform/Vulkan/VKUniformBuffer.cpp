#include "nrpch.h"
#include "VKUniformBuffer.h"

#include "VKContext.h"

namespace NR
{
	VKUniformBuffer::VKUniformBuffer(uint32_t size, uint32_t binding)
		: mSize(size), mBinding(binding)
	{
		mLocalStorage = new uint8_t[size];

		Ref<VKUniformBuffer> instance = this;
		Renderer::Submit([instance]() mutable
			{
				instance->RT_Invalidate();
			});
	}

	VKUniformBuffer::~VKUniformBuffer()
	{
		delete[] mLocalStorage;
	}

	void VKUniformBuffer::RT_Invalidate()
	{
		VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.size = mSize;

		// Create a new buffer
		VKAllocator allocator("UniformBuffer");
		mMemoryAlloc = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_ONLY, mBuffer);

		// Store information in the uniform's descriptor that is used by the descriptor set
		mDescriptorInfo.buffer = mBuffer;
		mDescriptorInfo.offset = 0;
		mDescriptorInfo.range = mSize;
	}

	void VKUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		memcpy(mLocalStorage, data, size);
		Ref<VKUniformBuffer> instance = this;
		Renderer::Submit([instance, size, offset]() mutable
			{
				instance->RT_SetData(instance->mLocalStorage, size, offset);
			});
	}

	void VKUniformBuffer::RT_SetData(const void* data, uint32_t size, uint32_t offset)
	{
		VKAllocator allocator("VKUniformBuffer");
		uint8_t* pData = allocator.MapMemory<uint8_t>(mMemoryAlloc);
		memcpy(pData, (uint8_t*)data + offset, size);
		allocator.UnmapMemory(mMemoryAlloc);
	}
}