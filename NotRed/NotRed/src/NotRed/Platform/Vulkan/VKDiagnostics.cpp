#include "nrpch.h"
#include "VKDiagnostics.h"

#include "NotRed/Platform/Vulkan/VKContext.h"

namespace NR::Utils
{
    static std::vector<VKCheckpointData> sCheckpointStorage(1024);
    static uint32_t sCheckpointStorageIndex = 0;

    void SetVKCheckpoint(VkCommandBuffer commandBuffer, const std::string& data)
    {
        bool supported = VKContext::GetCurrentDevice()->GetPhysicalDevice()->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
        if (!supported)
        {
            return;
        }

        sCheckpointStorageIndex = (sCheckpointStorageIndex + 1) % 1024;
        VKCheckpointData& checkpoint = sCheckpointStorage[sCheckpointStorageIndex];
        memset(checkpoint.Data, 0, sizeof(checkpoint.Data));
        strcpy(checkpoint.Data, data.data());
        vkCmdSetCheckpointNV(commandBuffer, &checkpoint);
    }
}