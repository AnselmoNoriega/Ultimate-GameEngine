#include "nrpch.h"
#include "VKDevice.h"

#include "VKContext.h"
#include "Debug/NsightAftermathGpuCrashTracker.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace NR
{
    // Vulkan Physical Device-----------------------------------------

    VKPhysicalDevice::VKPhysicalDevice()
    {
        auto vkInstance = VKContext::GetInstance();

        uint32_t gpuCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, &gpuCount, nullptr);
        NR_CORE_ASSERT(gpuCount > 0);

        std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &gpuCount, physicalDevices.data()));

        VkPhysicalDevice selectedPhysicalDevice = nullptr;
        for (VkPhysicalDevice physicalDevice : physicalDevices)
        {
            vkGetPhysicalDeviceProperties(physicalDevice, &mProperties);
            if (mProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                selectedPhysicalDevice = physicalDevice;
                break;
            }
        }

        if (!selectedPhysicalDevice)
        {
            NR_CORE_TRACE("Could not find discrete GPU.");
            selectedPhysicalDevice = physicalDevices.back();
        }

        NR_CORE_ASSERT(selectedPhysicalDevice, "Could not find any physical devices!");
        mPhysicalDevice = selectedPhysicalDevice;

        vkGetPhysicalDeviceFeatures(mPhysicalDevice, &mFeatures);
        vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mMemoryProperties);

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);
        NR_CORE_ASSERT(queueFamilyCount > 0, "");
        mQueueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, mQueueFamilyProperties.data());

        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &extCount, nullptr);
        if (extCount > 0)
        {
            std::vector<VkExtensionProperties> extensions(extCount);
            if (vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
            {
                NR_CORE_TRACE("Selected physical device has {0} extensions", extensions.size());
                for (const auto& ext : extensions)
                {
                    mSupportedExtensions.emplace(ext.extensionName);
                    NR_CORE_TRACE("  {0}", ext.extensionName);
                }
            }
        }

        // Queue families
        // Desired queues need to be requested upon logical device creation
        // Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, 
        // especially if the application
        // requests different queue types

        // Get queue family indices for the requested queue family types
        // Note that the indices may overlap depending on the implementation

        static const float defaultQueuePriority(0.0f);

        int requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        mQueueFamilyIndices = GetQueueFamilyIndices(requestedQueueTypes);

        // Graphics queue
        if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
        {
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = mQueueFamilyIndices.Graphics;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            mQueueCreateInfos.push_back(queueInfo);
        }

        // Dedicated compute queue
        if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
        {
            if (mQueueFamilyIndices.Compute != mQueueFamilyIndices.Graphics)
            {
                // If compute family index differs, we need an additional queue create info for the compute queue
                VkDeviceQueueCreateInfo queueInfo{};
                queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = mQueueFamilyIndices.Compute;
                queueInfo.queueCount = 1;
                queueInfo.pQueuePriorities = &defaultQueuePriority;
                mQueueCreateInfos.push_back(queueInfo);
            }
        }

        // Dedicated transfer queue
        if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
        {
            if ((mQueueFamilyIndices.Transfer != mQueueFamilyIndices.Graphics) && (mQueueFamilyIndices.Transfer != mQueueFamilyIndices.Compute))
            {
                // If compute family index differs, we need an additional queue create info for the compute queue
                VkDeviceQueueCreateInfo queueInfo{};
                queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = mQueueFamilyIndices.Transfer;
                queueInfo.queueCount = 1;
                queueInfo.pQueuePriorities = &defaultQueuePriority;
                mQueueCreateInfos.push_back(queueInfo);
            }
        }

        mDepthFormat = FindDepthFormat();
        NR_CORE_ASSERT(mDepthFormat);
    }

    VKPhysicalDevice::~VKPhysicalDevice()
    {
    }

    VkFormat VKPhysicalDevice::FindDepthFormat() const
    {
        // Start with the highest precision packed format
        std::vector<VkFormat> depthFormats = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM
        };

        for (auto& format : depthFormats)
        {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &formatProps);
            // Format must support depth stencil attachment for optimal tiling
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                return format;
            }
        }
        return VK_FORMAT_UNDEFINED;
    }

    bool VKPhysicalDevice::IsExtensionSupported(const std::string& extensionName) const
    {
        return mSupportedExtensions.find(extensionName) != mSupportedExtensions.end();
    }

    VKPhysicalDevice::QueueFamilyIndices VKPhysicalDevice::GetQueueFamilyIndices(int flags)
    {
        QueueFamilyIndices indices;

        // Dedicated queue for compute
        // Try to find a queue family index that supports compute but not graphics
        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            for (uint32_t i = 0; i < mQueueFamilyProperties.size(); ++i)
            {
                auto& queueFamilyProperties = mQueueFamilyProperties[i];
                if ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    indices.Compute = i;
                    break;
                }
            }
        }

        // Dedicated queue for transfer
        // Try to find a queue family index that supports transfer but not graphics and compute
        if (flags & VK_QUEUE_TRANSFER_BIT)
        {
            for (uint32_t i = 0; i < mQueueFamilyProperties.size(); ++i)
            {
                auto& queueFamilyProperties = mQueueFamilyProperties[i];
                if ((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                {
                    indices.Transfer = i;
                    break;
                }
            }
        }

        // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
        for (uint32_t i = 0; i < mQueueFamilyProperties.size(); ++i)
        {
            if ((flags & VK_QUEUE_TRANSFER_BIT) && indices.Transfer == -1)
            {
                if (mQueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    indices.Transfer = i;
                }
            }

            if ((flags & VK_QUEUE_COMPUTE_BIT) && indices.Compute == -1)
            {
                if (mQueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    indices.Compute = i;
                }
            }

            if (flags & VK_QUEUE_GRAPHICS_BIT)
            {
                if (mQueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    indices.Graphics = i;
                }
            }
        }

        return indices;
    }

    uint32_t VKPhysicalDevice::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const
    {
        // Iterate over all memory types available for the device used in this example
        for (uint32_t i = 0; i < mMemoryProperties.memoryTypeCount; ++i)
        {
            if ((typeBits & 1) == 1)
            {
                if ((mMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }
            typeBits >>= 1;
        }

        NR_CORE_ASSERT(false, "Could not find a suitable memory type!");
        return UINT32_MAX;
    }

    Ref<VKPhysicalDevice> VKPhysicalDevice::Select()
    {
        return Ref<VKPhysicalDevice>::Create();
    }

    // Vulkan Device-------------------------------------------------------

    VKDevice::VKDevice(const Ref<VKPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures)
        : mPhysicalDevice(physicalDevice), mEnabledFeatures(enabledFeatures)
    {
        std::vector<const char*> deviceExtensions;
        // If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
        NR_CORE_ASSERT(mPhysicalDevice->IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME));
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        if (mPhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME))
        {
            deviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
        }

        if (mPhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME))
        {
            deviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);
        }

        VkDeviceDiagnosticsConfigCreateInfoNV aftermathInfo = {};
        bool canEnableAftermath = mPhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME) && mPhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);
        if (canEnableAftermath)
        {
            GpuCrashTracker* gpuCrashTracker = new GpuCrashTracker({});
            gpuCrashTracker->Initialize();
            VkDeviceDiagnosticsConfigFlagBitsNV aftermathFlags = (VkDeviceDiagnosticsConfigFlagBitsNV)(VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
                VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |
                VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV);
            aftermathInfo.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
            aftermathInfo.flags = aftermathFlags;
        }

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        if (canEnableAftermath)
        {
            deviceCreateInfo.pNext = &aftermathInfo;
        }
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(physicalDevice->mQueueCreateInfos.size());;
        deviceCreateInfo.pQueueCreateInfos = physicalDevice->mQueueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

        // If a pNext(Chain) has been passed, adding it to the device creation info is needed
        VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};

        // Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
        if (mPhysicalDevice->IsExtensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
        {
            deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
            mEnableDebugMarkers = true;
        }

        if (deviceExtensions.size() > 0)
        {
            deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
            deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        }

        VkResult result = vkCreateDevice(mPhysicalDevice->GetVulkanPhysicalDevice(), &deviceCreateInfo, nullptr, &mLogicalDevice);
        NR_CORE_ASSERT(result == VK_SUCCESS);

        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = mPhysicalDevice->mQueueFamilyIndices.Graphics;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK_RESULT(vkCreateCommandPool(mLogicalDevice, &cmdPoolInfo, nullptr, &mCommandPool));

        cmdPoolInfo.queueFamilyIndex = mPhysicalDevice->mQueueFamilyIndices.Compute;
        VK_CHECK_RESULT(vkCreateCommandPool(mLogicalDevice, &cmdPoolInfo, nullptr, &mComputeCommandPool));

        // Get a graphics queue from the device
        vkGetDeviceQueue(mLogicalDevice, mPhysicalDevice->mQueueFamilyIndices.Graphics, 0, &mQueue);
        vkGetDeviceQueue(mLogicalDevice, mPhysicalDevice->mQueueFamilyIndices.Compute, 0, &mComputeQueue);
    }

    VKDevice::~VKDevice()
    {
    }

    void VKDevice::Destroy()
    {
        vkDestroyCommandPool(mLogicalDevice, mCommandPool, nullptr);
        vkDestroyCommandPool(mLogicalDevice, mComputeCommandPool, nullptr);

        vkDeviceWaitIdle(mLogicalDevice);
        vkDestroyDevice(mLogicalDevice, nullptr);
    }

    VkCommandBuffer VKDevice::GetCommandBuffer(bool begin, bool compute)
    {
        VkCommandBuffer cmdBuffer;

        VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
        cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool = compute ? mComputeCommandPool : mCommandPool;
        cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocateInfo.commandBufferCount = 1;

        VK_CHECK_RESULT(vkAllocateCommandBuffers(mLogicalDevice, &cmdBufAllocateInfo, &cmdBuffer));

        // If requested, also start the new command buffer
        if (begin)
        {
            VkCommandBufferBeginInfo cmdBufferBeginInfo{};
            cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));
        }

        return cmdBuffer;
    }

    void VKDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer)
    {
        FlushCommandBuffer(commandBuffer, mQueue);
    }

    void VKDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
    {
        const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;

        NR_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE);

        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = 0;
        VkFence fence;
        VK_CHECK_RESULT(vkCreateFence(mLogicalDevice, &fenceCreateInfo, nullptr, &fence));

        VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
        VK_CHECK_RESULT(vkWaitForFences(mLogicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

        vkDestroyFence(mLogicalDevice, fence, nullptr);
        vkFreeCommandBuffers(mLogicalDevice, mCommandPool, 1, &commandBuffer);
    }

    VkCommandBuffer VKDevice::CreateSecondaryCommandBuffer()
    {
        VkCommandBuffer cmdBuffer;

        VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
        cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool = mCommandPool;
        cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        cmdBufAllocateInfo.commandBufferCount = 1;

        VK_CHECK_RESULT(vkAllocateCommandBuffers(mLogicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
        return cmdBuffer;
    }
}