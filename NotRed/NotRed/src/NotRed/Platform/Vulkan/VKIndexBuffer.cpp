#include "nrpch.h"
#include "VKIndexBuffer.h"

#include "VKContext.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
	VKIndexBuffer::VKIndexBuffer(uint32_t size)
		: mSize(size)
	{
	}

	VKIndexBuffer::VKIndexBuffer(void* data, uint32_t size)
		: mSize(size)
	{
		mLocalData = Buffer::Copy(data, size);

		Ref<VKIndexBuffer> instance = this;
		Renderer::Submit([instance]() mutable
			{
				auto device = VKContext::GetCurrentDevice()->GetVulkanDevice();

				VkBufferCreateInfo indexbufferCreateInfo = {};
				indexbufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				indexbufferCreateInfo.size = instance->mSize;
				indexbufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

				VKAllocator allocator("IndexBuffer");
				auto bufferAlloc = allocator.AllocateBuffer(indexbufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, instance->mVulkanBuffer);

				void* dstBuffer = allocator.MapMemory<void>(bufferAlloc);
				memcpy(dstBuffer, instance->mLocalData.Data, instance->mSize);
				allocator.UnmapMemory(bufferAlloc);
			});
	}

	void VKIndexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset)
	{
	}

	void VKIndexBuffer::Bind() const
	{
	}

	RendererID VKIndexBuffer::GetRendererID() const
	{
		return 0;
	}

}