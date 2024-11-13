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
                auto device = VKContext::GetCurrentDevice();
                VKAllocator allocator("IndexBuffer");

#define USE_STAGING 1
#if USE_STAGING
                VkBufferCreateInfo bufferCreateInfo{};
                bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferCreateInfo.size = instance->mSize;
                bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                VkBuffer stagingBuffer;
                VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);
                
                // Copy data to staging buffer
                uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
                memcpy(destData, instance->mLocalData.Data, instance->mLocalData.Size);
                allocator.UnmapMemory(stagingBufferAllocation);

                VkBufferCreateInfo indexBufferCreateInfo = {};
                indexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                indexBufferCreateInfo.size = instance->mSize;
                indexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                instance->mMemoryAllocation = allocator.AllocateBuffer(indexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, instance->mVulkanBuffer);

                VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

                VkBufferCopy copyRegion = {};
                copyRegion.size = instance->mLocalData.Size;
                vkCmdCopyBuffer(
                    copyCmd,
                    stagingBuffer,
                    instance->mVulkanBuffer,
                    1,
                    &copyRegion);
                device->FlushCommandBuffer(copyCmd);			
                allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
#else
                VkBufferCreateInfo indexbufferCreateInfo = {};
                indexbufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                indexbufferCreateInfo.size = instance->mSize;
                indexbufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

                auto bufferAlloc = allocator.AllocateBuffer(indexbufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, instance->mVulkanBuffer);

                void* dstBuffer = allocator.MapMemory<void>(bufferAlloc);
                memcpy(dstBuffer, instance->mLocalData.Data, instance->mSize);
                allocator.UnmapMemory(bufferAlloc);
#endif
            });
    }

    VKIndexBuffer::~VKIndexBuffer()
    {
        VkBuffer buffer = mVulkanBuffer;
        VmaAllocation allocation = mMemoryAllocation;
        Renderer::SubmitResourceFree([buffer, allocation]()
            {
                VKAllocator allocator("IndexBuffer");
                allocator.DestroyBuffer(buffer, allocation);
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