#include "nrpch.h"
#include "VKPipeline.h"

#include "NotRed/Renderer/Renderer.h"

#include "VKShader.h"
#include "VKContext.h"
#include "VKFrameBuffer.h"
#include "VKUniformBuffer.h"

namespace NR
{
    static VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float:     return VK_FORMAT_R32_SFLOAT;
        case ShaderDataType::Float2:    return VK_FORMAT_R32G32_SFLOAT;
        case ShaderDataType::Float3:    return VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderDataType::Float4:    return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:
            NR_CORE_ASSERT(false);
            return VK_FORMAT_UNDEFINED;
        }
    }

    VKPipeline::VKPipeline(const PipelineSpecification& spec)
        : mSpecification(spec)
    {
        NR_CORE_ASSERT(spec.Shader);
        NR_CORE_ASSERT(spec.RenderPass);
        Invalidate();
        Renderer::RegisterShaderDependency(spec.Shader, this);
    }

    VKPipeline::~VKPipeline()
    {
    }

    void VKPipeline::Invalidate()
    {
        Ref<VKPipeline> instance = this;
        Renderer::Submit([instance]() mutable
            {
                VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

                NR_CORE_ASSERT(instance->mSpecification.Shader);
                Ref<VKShader> vulkanShader = Ref<VKShader>(instance->mSpecification.Shader);
                Ref<VKFrameBuffer> frameBuffer = instance->mSpecification.RenderPass->GetSpecification().TargetFrameBuffer.As<VKFrameBuffer>();

                auto descriptorSetLayouts = vulkanShader->GetAllDescriptorSetLayouts();

                const auto& pushConstantRanges = vulkanShader->GetPushConstantRanges();

                std::vector<VkPushConstantRange> vulkanPushConstantRanges(pushConstantRanges.size());
                for (uint32_t i = 0; i < pushConstantRanges.size(); ++i)
                {
                    const auto& pushConstantRange = pushConstantRanges[i];
                    auto& vulkanPushConstantRange = vulkanPushConstantRanges[i];

                    vulkanPushConstantRange.stageFlags = pushConstantRange.ShaderStage;
                    vulkanPushConstantRange.offset = pushConstantRange.Offset;
                    vulkanPushConstantRange.size = pushConstantRange.Size;
                }

                // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
                // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
                VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
                pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pPipelineLayoutCreateInfo.pNext = nullptr;
                pPipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
                pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
                pPipelineLayoutCreateInfo.pushConstantRangeCount = vulkanPushConstantRanges.size();
                pPipelineLayoutCreateInfo.pPushConstantRanges = vulkanPushConstantRanges.data();

                VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &instance->mPipelineLayout));

                // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
                // A pipeline is then stored and hashed on the GPU making pipeline changes very fast
                // Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

                VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
                pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                // The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
                pipelineCreateInfo.layout = instance->mPipelineLayout;
                // Renderpass this pipeline is attached to
                pipelineCreateInfo.renderPass = frameBuffer->GetRenderPass();

                // Construct the differnent states making up the pipeline

                // Input assembly state describes how primitives are assembled
                // This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
                VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
                inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

                // Rasterization state
                VkPipelineRasterizationStateCreateInfo rasterizationState = {};
                rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
                rasterizationState.cullMode = VK_CULL_MODE_NONE;
                rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                rasterizationState.depthClampEnable = VK_FALSE;
                rasterizationState.rasterizerDiscardEnable = VK_FALSE;
                rasterizationState.depthBiasEnable = VK_FALSE;
                rasterizationState.lineWidth = 1.0f;

                // Color blend state describes how blend factors are calculated (if used)
                // We need one blend attachment state per color attachment (even if blending is not used)
                size_t colorAttachmentCount = frameBuffer->GetColorAttachmentCount();
                std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates(colorAttachmentCount);
                for (size_t i = 0; i < colorAttachmentCount; ++i)
                {
                    blendAttachmentStates[i].colorWriteMask = 0xf;
                    blendAttachmentStates[i].blendEnable = VK_TRUE;
                    blendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                    blendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                    blendAttachmentStates[i].colorBlendOp = VK_BLEND_OP_ADD;
                    blendAttachmentStates[i].alphaBlendOp = VK_BLEND_OP_ADD;
                    blendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                }

                VkPipelineColorBlendStateCreateInfo colorBlendState = {};
                colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlendState.attachmentCount = blendAttachmentStates.size();
                colorBlendState.pAttachments = blendAttachmentStates.data();

                // Viewport state sets the number of viewports and scissor used in this pipeline
                // Note: This is actually overriden by the dynamic states (see below)
                VkPipelineViewportStateCreateInfo viewportState = {};
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.viewportCount = 1;
                viewportState.scissorCount = 1;

                // Enable dynamic states
                // Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
                // To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
                // For this example we will set the viewport and scissor using dynamic states
                std::vector<VkDynamicState> dynamicStateEnables;
                dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
                dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
                VkPipelineDynamicStateCreateInfo dynamicState = {};
                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.pDynamicStates = dynamicStateEnables.data();
                dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

                // Depth and stencil state containing depth and stencil compare and test operations
                // We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
                VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
                depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencilState.depthTestEnable = VK_TRUE;
                depthStencilState.depthWriteEnable = VK_TRUE;
                depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                depthStencilState.depthBoundsTestEnable = VK_FALSE;
                depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
                depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
                depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
                depthStencilState.stencilTestEnable = VK_FALSE;
                depthStencilState.front = depthStencilState.back;

                // Multi sampling state
                // This example does not make use fo multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
                VkPipelineMultisampleStateCreateInfo multisampleState = {};
                multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                multisampleState.pSampleMask = nullptr;

                // Vertex input descriptor

                VertexBufferLayout& layout = instance->mSpecification.Layout;

                VkVertexInputBindingDescription vertexInputBinding = {};
                vertexInputBinding.binding = 0;
                vertexInputBinding.stride = layout.GetStride();
                vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                // Inpute attribute bindings describe shader attribute locations and memory layouts
                std::vector<VkVertexInputAttributeDescription> vertexInputAttributs(layout.GetElementCount());

                uint32_t location = 0;
                for (auto element : layout)
                {
                    vertexInputAttributs[location].binding = 0;
                    vertexInputAttributs[location].location = location;
                    vertexInputAttributs[location].format = ShaderDataTypeToVulkanFormat(element.Type);
                    vertexInputAttributs[location].offset = element.Offset;

                    ++location;
                }

                // Vertex input state used for pipeline creation
                VkPipelineVertexInputStateCreateInfo vertexInputState = {};
                vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                vertexInputState.vertexBindingDescriptionCount = 1;
                vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
                vertexInputState.vertexAttributeDescriptionCount = vertexInputAttributs.size();
                vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();

                const auto& shaderStages = vulkanShader->GetPipelineShaderStageCreateInfos();

                // Set pipeline shader stage info
                pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
                pipelineCreateInfo.pStages = shaderStages.data();

                // Assign the pipeline states to the pipeline creation info structure
                pipelineCreateInfo.pVertexInputState = &vertexInputState;
                pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
                pipelineCreateInfo.pRasterizationState = &rasterizationState;
                pipelineCreateInfo.pColorBlendState = &colorBlendState;
                pipelineCreateInfo.pMultisampleState = &multisampleState;
                pipelineCreateInfo.pViewportState = &viewportState;
                pipelineCreateInfo.pDepthStencilState = &depthStencilState;
                pipelineCreateInfo.renderPass = frameBuffer->GetRenderPass();
                pipelineCreateInfo.pDynamicState = &dynamicState;

                VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
                pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
                VkPipelineCache pipelineCache;
                VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));

                // Create rendering pipeline using the specified states
                VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &instance->mVulkanPipeline));

                // Shader modules are no longer needed once the graphics pipeline has been created
                // vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
                // vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
            });
    }

    void VKPipeline::SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t binding, uint32_t set)
    {
        Ref<VKPipeline> instance = this;
        Renderer::Submit([instance, uniformBuffer, binding, set]() mutable
            {
                instance->RT_SetUniformBuffer(uniformBuffer, binding, set);
            });
    }

    void VKPipeline::RT_SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t binding, uint32_t set)
    {
        Ref<VKShader> vulkanShader = Ref<VKShader>(mSpecification.Shader);
        Ref<VKUniformBuffer> vulkanUniformBuffer = uniformBuffer.As<VKUniformBuffer>();
        NR_CORE_ASSERT(mDescriptorSets.DescriptorSets.size() > set);

        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &vulkanUniformBuffer->GetDescriptorBufferInfo();
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.dstSet = mDescriptorSets.DescriptorSets[set];

        NR_CORE_WARN("VulkanPipeline - Updating descriptor set (VulkanPipeline::SetUniformBuffer)");
        VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    }

    void VKPipeline::Bind()
    {

    }
}