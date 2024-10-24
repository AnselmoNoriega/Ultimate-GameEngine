#include "nrpch.h"
#include "VKComputePipeline.h"

#include "NotRed/Platform/Vulkan/VKContext.h"
#include "NotRed/Platform/Vulkan/VKDiagnostics.h"

#include "NotRed/Renderer/Renderer.h"

#include "NotRed/Core/Timer.h"

namespace NR
{
    static VkFence sComputeFence = nullptr;

    VKComputePipeline::VKComputePipeline(Ref<Shader> computeShader)
        : mShader(computeShader.As<VKShader>())
    {
        Ref<VKComputePipeline> instance = this;
        Renderer::Submit([instance]() mutable
            {
                instance->RT_CreatePipeline();
            });

        Renderer::RegisterShaderDependency(computeShader, this);
    }

    void VKComputePipeline::CreatePipeline()
    {
        Renderer::Submit([instance = Ref(this)]() mutable
            {
                instance->RT_CreatePipeline();
            });
    }

    void VKComputePipeline::RT_CreatePipeline()
    {
        VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

        auto descriptorSetLayouts = mShader->GetAllDescriptorSetLayouts();

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

        const auto& pushConstantRanges = mShader->GetPushConstantRanges();
        std::vector<VkPushConstantRange> vulkanPushConstantRanges(pushConstantRanges.size());
        if (pushConstantRanges.size())
        {
            for (uint32_t i = 0; i < pushConstantRanges.size(); ++i)
            {
                const auto& pushConstantRange = pushConstantRanges[i];
                auto& vulkanPushConstantRange = vulkanPushConstantRanges[i];

                vulkanPushConstantRange.stageFlags = pushConstantRange.ShaderStage;
                vulkanPushConstantRange.offset = pushConstantRange.Offset;
                vulkanPushConstantRange.size = pushConstantRange.Size;
            }

            pipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)vulkanPushConstantRanges.size();
            pipelineLayoutCreateInfo.pPushConstantRanges = vulkanPushConstantRanges.data();
        }

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &mComputePipelineLayout));

        VkComputePipelineCreateInfo computePipelineCreateInfo{};
        computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineCreateInfo.layout = mComputePipelineLayout;
        computePipelineCreateInfo.flags = 0;
        const auto& shaderStages = mShader->GetPipelineShaderStageCreateInfos();
        computePipelineCreateInfo.stage = shaderStages[0];

        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &mPipelineCache));
        VK_CHECK_RESULT(vkCreateComputePipelines(device, mPipelineCache, 1, &computePipelineCreateInfo, nullptr, &mComputePipeline));
    }

    void VKComputePipeline::Execute(VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

        VkQueue computeQueue = VKContext::GetCurrentDevice()->GetComputeQueue();

        VkCommandBuffer computeCommandBuffer = VKContext::GetCurrentDevice()->GetCommandBuffer(true, true);

        Utils::SetVKCheckpoint(computeCommandBuffer, "VKComputePipeline::Execute");

        vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline);
        for (uint32_t i = 0; i < descriptorSetCount; ++i)
        {
            vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipelineLayout, 0, 1, &descriptorSets[i], 0, 0);
            vkCmdDispatch(computeCommandBuffer, groupCountX, groupCountY, groupCountZ);
        }

        vkEndCommandBuffer(computeCommandBuffer);
        if (!sComputeFence)
        {
            VkFenceCreateInfo fenceCreateInfo{};
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &sComputeFence));
        }

        // Make sure previous compute shader in pipeline has completed
        vkWaitForFences(device, 1, &sComputeFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &sComputeFence);

        VkSubmitInfo computeSubmitInfo{};
        computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        computeSubmitInfo.commandBufferCount = 1;
        computeSubmitInfo.pCommandBuffers = &computeCommandBuffer;
        VK_CHECK_RESULT(vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, sComputeFence));

        // Wait for execution of compute shader to complete
        {
            Timer timer;
            vkWaitForFences(device, 1, &sComputeFence, VK_TRUE, UINT64_MAX);
            NR_CORE_TRACE("Compute shader execution took {0} ms", timer.ElapsedMillis());
        }
    }

    void VKComputePipeline::Begin(Ref<RenderCommandBuffer> renderCommandBuffer)
    {
        NR_CORE_ASSERT(!mActiveComputeCommandBuffer);

        if (renderCommandBuffer)
        {
            uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
            mActiveComputeCommandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);
            mUsingGraphicsQueue = true;
        }
        else
        {
            mActiveComputeCommandBuffer = VKContext::GetCurrentDevice()->GetCommandBuffer(true, true);
            mUsingGraphicsQueue = false;
        }

        vkCmdBindPipeline(mActiveComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline);
    }

    void VKComputePipeline::Dispatch(VkDescriptorSet descriptorSet, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        NR_CORE_ASSERT(mActiveComputeCommandBuffer);

        vkCmdBindDescriptorSets(mActiveComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipelineLayout, 0, 1, &descriptorSet, 0, 0);
        vkCmdDispatch(mActiveComputeCommandBuffer, groupCountX, groupCountY, groupCountZ);
    }

    void VKComputePipeline::End()
    {
        NR_CORE_ASSERT(mActiveComputeCommandBuffer);

        VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

        if (!mUsingGraphicsQueue)
        {
            VkQueue computeQueue = VKContext::GetCurrentDevice()->GetComputeQueue();

            vkEndCommandBuffer(mActiveComputeCommandBuffer);

            if (!sComputeFence)
            {
                VkFenceCreateInfo fenceCreateInfo{};
                fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &sComputeFence));
            }

            vkWaitForFences(device, 1, &sComputeFence, VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &sComputeFence);

            VkSubmitInfo computeSubmitInfo{};
            computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            computeSubmitInfo.commandBufferCount = 1;
            computeSubmitInfo.pCommandBuffers = &mActiveComputeCommandBuffer;
            VK_CHECK_RESULT(vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, sComputeFence));

            // Wait for execution of compute shader to complete
            // Currently this is here for "safety"
            {
                Timer timer;
                vkWaitForFences(device, 1, &sComputeFence, VK_TRUE, UINT64_MAX);
                NR_CORE_TRACE("Compute shader execution took {0} ms", timer.ElapsedMillis());
            }
        }

        mActiveComputeCommandBuffer = nullptr;
    }

    void VKComputePipeline::SetPushConstants(const void* data, uint32_t size)
    {
        vkCmdPushConstants(mActiveComputeCommandBuffer, mComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, size, data);
    }
}