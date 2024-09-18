#include "nrpch.h"
#include "VKVertexBuffer.h"

#include "VKContext.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
    VKVertexBuffer::VKVertexBuffer(uint32_t size, VertexBufferUsage usage)
        : mSize(size)
    {
    }

    VKVertexBuffer::VKVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage)
        : mSize(size)
    {
        mLocalData = Buffer::Copy(data, size);

        Ref<VKVertexBuffer> instance = this;
        Renderer::Submit([instance]() mutable
            {
                auto device = VKContext::GetCurrentDevice()->GetVulkanDevice();

                // Index buffer
                VkBufferCreateInfo vertexbufferInfo = {};
                vertexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                vertexbufferInfo.size = instance->mSize;
                vertexbufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

                // Copy index data to a buffer visible to the host
                VK_CHECK_RESULT(vkCreateBuffer(device, &vertexbufferInfo, nullptr, &instance->mVulkanBuffer));
                VkMemoryRequirements memoryRequirements;
                vkGetBufferMemoryRequirements(device, instance->mVulkanBuffer, &memoryRequirements);

                VKAllocator allocator("VertexBuffer");
                allocator.Allocate(memoryRequirements, &instance->mDeviceMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                void* dstBuffer;
                VK_CHECK_RESULT(vkMapMemory(device, instance->mDeviceMemory, 0, instance->mSize, 0, &dstBuffer));
                memcpy(dstBuffer, instance->mLocalData.Data, instance->mSize);
                vkUnmapMemory(device, instance->mDeviceMemory);

                VK_CHECK_RESULT(vkBindBufferMemory(device, instance->mVulkanBuffer, instance->mDeviceMemory, 0));
            });
    }
}