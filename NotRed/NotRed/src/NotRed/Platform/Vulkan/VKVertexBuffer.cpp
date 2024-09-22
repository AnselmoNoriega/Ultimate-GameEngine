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
                VkBufferCreateInfo vertexBufferCreateInfo = {};
                vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                vertexBufferCreateInfo.size = instance->mSize;
                vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

                VKAllocator allocator("VertexBuffer");
                auto bufferAlloc = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, instance->mVulkanBuffer);

                void* dstBuffer = allocator.MapMemory<void>(bufferAlloc);
                memcpy(dstBuffer, instance->mLocalData.Data, instance->mSize);
                allocator.UnmapMemory(bufferAlloc);
            });
    }
}