#include "nrpch.h"
#include "VKVertexBuffer.h"

#include "VKContext.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
    VKVertexBuffer::VKVertexBuffer(uint32_t size, VertexBufferUsage usage)
        : mSize(size)
    {
        mLocalData.Allocate(size);

        Ref<VKVertexBuffer> instance = this;
        Renderer::Submit([instance]() mutable
            {
                auto device = VKContext::GetCurrentDevice();
                VKAllocator allocator("VertexBuffer");
                VkBufferCreateInfo vertexBufferCreateInfo = {};
                vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                vertexBufferCreateInfo.size = instance->mSize;
                vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                instance->mMemoryAllocation = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, instance->mVulkanBuffer);
            });
    }

    VKVertexBuffer::VKVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage)
        : mSize(size)
    {
        mLocalData = Buffer::Copy(data, size);

        Ref<VKVertexBuffer> instance = this;
        Renderer::Submit([instance]() mutable
            {
                auto device = VKContext::GetCurrentDevice();
                VKAllocator allocator("VertexBuffer");

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

                // Index buffer
                VkBufferCreateInfo vertexBufferCreateInfo = {};
                vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                vertexBufferCreateInfo.size = instance->mSize;
                vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                instance->mMemoryAllocation = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, instance->mVulkanBuffer);

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
                VkBufferCreateInfo vertexBufferCreateInfo = {};
                vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                vertexBufferCreateInfo.size = instance->mSize;
                vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                auto bufferAlloc = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, instance->mVulkanBuffer);

                void* dstBuffer = allocator.MapMemory<void>(bufferAlloc);
                memcpy(dstBuffer, instance->mLocalData.Data, instance->mSize);
                allocator.UnmapMemory(bufferAlloc);
#endif
            });
    }

    VKVertexBuffer::~VKVertexBuffer()
    {
        VkBuffer buffer = mVulkanBuffer;
        VmaAllocation allocation = mMemoryAllocation;

        Renderer::SubmitResourceFree([buffer, allocation]()
            {
                VKAllocator allocator("VertexBuffer");
                allocator.DestroyBuffer(buffer, allocation);
            });
    }

    void VKVertexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset)
    {
        NR_CORE_ASSERT(size <= mLocalData.Size);
        
        memcpy(mLocalData.Data, (uint8_t*)buffer + offset, size);;

        Ref<VKVertexBuffer> instance = this;
        Renderer::Submit([instance, size, offset]() mutable
            {
                instance->RT_SetData(instance->mLocalData.Data, size, offset);
            });
    }

    void VKVertexBuffer::RT_SetData(void* buffer, uint32_t size, uint32_t offset)
    {
        VKAllocator allocator("VulkanVertexBuffer");
        uint8_t* pData = allocator.MapMemory<uint8_t>(mMemoryAllocation);
        memcpy(pData, (uint8_t*)buffer + offset, size);
        allocator.UnmapMemory(mMemoryAllocation);
    }
}