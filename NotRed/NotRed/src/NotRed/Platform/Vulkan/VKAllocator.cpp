#include "nrpch.h"
#include "VKAllocator.h"

#include "VKContext.h"

#include "NotRed/Util/StringUtils.h"

#if NR_LOG_RENDERER_ALLOCATIONS
    #define NR_ALLOCATOR_LOG(...) NR_CORE_TRACE(__VA_ARGS__)
#else
    #define NR_ALLOCATOR_LOG(...)
#endif

namespace NR
{
    struct VKAllocatorData
    {
        VmaAllocator Allocator;
        uint64_t TotalAllocatedBytes = 0;
    };
    static VKAllocatorData* sData = nullptr;
    VKAllocator::VKAllocator(const std::string& tag)
        : mTag(tag)
    {
    }

    VKAllocator::~VKAllocator()
    {
    }

    VmaAllocation VKAllocator::AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer)
    {
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usage;

        VmaAllocation allocation;
        vmaCreateBuffer(sData->Allocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, nullptr);

        VmaAllocationInfo allocInfo {};
        vmaGetAllocationInfo(sData->Allocator, allocation, &allocInfo);
        NR_ALLOCATOR_LOG("VKAllocator ({0}): allocating buffer; size = {1}", mTag, Utils::BytesToString(allocInfo.size));

        sData->TotalAllocatedBytes += allocInfo.size;
        NR_ALLOCATOR_LOG("VKAllocator ({0}): total allocated since start is {1}", mTag, Utils::BytesToString(sData->TotalAllocatedBytes));

        return allocation;
    }

    VmaAllocation VKAllocator::AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage)
    {
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usage;

        VmaAllocation allocation;
        vmaCreateImage(sData->Allocator, &imageCreateInfo, &allocCreateInfo, &outImage, &allocation, nullptr);

        VmaAllocationInfo allocInfo {};
        vmaGetAllocationInfo(sData->Allocator, allocation, &allocInfo);
        NR_ALLOCATOR_LOG("VKAllocator ({0}): allocating image; size = {1}", mTag, Utils::BytesToString(allocInfo.size));

        sData->TotalAllocatedBytes += allocInfo.size;
        NR_ALLOCATOR_LOG("VKAllocator ({0}): total allocated since start is {1}", mTag, Utils::BytesToString(sData->TotalAllocatedBytes));

        return allocation;
    }

    void VKAllocator::Free(VmaAllocation allocation)
    {
        vmaFreeMemory(sData->Allocator, allocation);
    }

    void VKAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
    {
        NR_CORE_ASSERT(image);
        NR_CORE_ASSERT(allocation);
        vmaDestroyImage(sData->Allocator, image, allocation);
    }

    void VKAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
    {
        NR_CORE_ASSERT(buffer);
        NR_CORE_ASSERT(allocation);
        vmaDestroyBuffer(sData->Allocator, buffer, allocation);
    }

    void VKAllocator::UnmapMemory(VmaAllocation allocation)
    {
        vmaUnmapMemory(sData->Allocator, allocation);
    }

    void VKAllocator::DumpStats()
    {
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS] = {};
        vmaGetHeapBudgets(sData->Allocator, budgets);

        uint64_t usedMemory = budgets[0].usage;
        uint64_t freeMemory = budgets[0].budget - budgets[0].usage;
    }

    GPUMemoryStats VKAllocator::GetStats()
    {
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS] = {};
        vmaGetHeapBudgets(sData->Allocator, budgets);

        uint64_t totalUsedMemory = 0;
        uint64_t totalFreeMemory = 0;

        for (uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; ++i) 
        {
            totalUsedMemory += budgets[i].usage;
            totalFreeMemory += (budgets[i].budget - budgets[i].usage);
        }

        return { totalUsedMemory, totalFreeMemory };
    }

    void VKAllocator::Init(Ref<VKDevice> device)
    {
        sData = new VKAllocatorData();
        // Initialize VulkanMemoryAllocator
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        allocatorInfo.physicalDevice = device->GetPhysicalDevice()->GetVulkanPhysicalDevice();
        allocatorInfo.device = device->GetVulkanDevice();
        allocatorInfo.instance = VKContext::GetInstance();
        vmaCreateAllocator(&allocatorInfo, &sData->Allocator);
    }

    void VKAllocator::Shutdown()
    {
        vmaDestroyAllocator(sData->Allocator);
        delete sData;
        sData = nullptr;
    }

    VmaAllocator& VKAllocator::GetVMAAllocator()
    {
        return sData->Allocator;
    }
}