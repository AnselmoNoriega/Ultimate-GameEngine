#include "nrpch.h"
#include "VKStorageBuffer.h"

#include "VKContext.h"

namespace NR
{
	VKStorageBuffer::VKStorageBuffer(uint32_t size, uint32_t binding)
		: mSize(size), mBinding(binding)
	{
		Ref<VKStorageBuffer> instance = this;
		Renderer::Submit([instance]() mutable
			{
				instance->RT_Invalidate();
			});
	}

	VKStorageBuffer::~VKStorageBuffer()
	{
		Release();
	}

	void VKStorageBuffer::RT_Invalidate()
	{
		Release();

		VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferInfo.size = mSize;

		VKAllocator allocator("StorageBuffer");
		mMemoryAlloc = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, mBuffer);
		mDescriptorInfo.buffer = mBuffer;
		mDescriptorInfo.offset = 0;
		mDescriptorInfo.range = mSize;
	}

	void VKStorageBuffer::Release()
	{
		if (!mMemoryAlloc)
		{
			return;
		}

		Renderer::SubmitResourceFree([buffer = mBuffer, memoryAlloc = mMemoryAlloc]()
			{
				VKAllocator allocator("StorageBuffer");
				allocator.DestroyBuffer(buffer, memoryAlloc);
			});

		mBuffer = nullptr;
		mMemoryAlloc = nullptr;
	}

	void VKStorageBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		memcpy(mLocalStorage, data, size);
		Ref<VKStorageBuffer> instance = this;
		Renderer::Submit([instance, size, offset]() mutable
			{
				instance->RT_SetData(instance->mLocalStorage, size, offset);
			});
	}

	void VKStorageBuffer::RT_SetData(const void* data, uint32_t size, uint32_t offset)
	{
		VKAllocator allocator("VulkanStorageBuffer");
		uint8_t* pData = allocator.MapMemory<uint8_t>(mMemoryAlloc);
		memcpy(pData, (uint8_t*)data + offset, size);
		allocator.UnmapMemory(mMemoryAlloc);
	}

	void VKStorageBuffer::Resize(uint32_t newSize)
	{
		mSize = newSize;
		Ref<VKStorageBuffer> instance = this;
		Renderer::Submit([instance]() mutable
			{
				instance->RT_Invalidate();
			});
	}
}