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

				VkBufferCreateInfo indexbufferInfo = {};
				indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				indexbufferInfo.size = instance->mSize;
				indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

				VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferInfo, nullptr, &instance->mVulkanBuffer));
				VkMemoryRequirements memoryRequirements;
				vkGetBufferMemoryRequirements(device, instance->mVulkanBuffer, &memoryRequirements);

				VKAllocator allocator("IndexBuffer");
				allocator.Allocate(memoryRequirements, &instance->mDeviceMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				void* dstBuffer;
				VK_CHECK_RESULT(vkMapMemory(device, instance->mDeviceMemory, 0, instance->mSize, 0, &dstBuffer));
				memcpy(dstBuffer, instance->mLocalData.Data, instance->mSize);
				vkUnmapMemory(device, instance->mDeviceMemory);

				VK_CHECK_RESULT(vkBindBufferMemory(device, instance->mVulkanBuffer, instance->mDeviceMemory, 0));
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