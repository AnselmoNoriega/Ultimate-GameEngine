#include "nrpch.h"
#include "VKRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Vulkan.h"
#include "VkContext.h"

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/SceneRenderer.h"

#include "VKPipeline.h"
#include "VKVertexBuffer.h"
#include "VKIndexBuffer.h"
#include "VKFrameBuffer.h"
#include "VKMaterial.h"
#include "VKUniformBuffer.h"
#include "VKRenderCommandBuffer.h"
#include "VKStorageBuffer.h"

#include "VKShader.h"
#include "VKTexture.h"

#include "backends/imgui_impl_glfw.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

#include "imgui.h"

#include "NotRed/Core/Timer.h"

#include "NotRed/Debug/Profiler.h"

namespace NR
{
    struct VKRendererData
    {
        RendererCapabilities RenderCaps;

        Ref<Texture2D> BRDFLut;

        VKShader::ShaderMaterialDescriptorSet ParticleDescriptorSet;

        Ref<VertexBuffer> QuadVertexBuffer;
        Ref<IndexBuffer> QuadIndexBuffer;
        VKShader::ShaderMaterialDescriptorSet QuadDescriptorSet;

        std::unordered_map<SceneRenderer*, std::vector<VKShader::ShaderMaterialDescriptorSet>> RendererDescriptorSet;
        VkDescriptorSet ActiveRendererDescriptorSet = nullptr;
        std::vector<VkDescriptorPool> DescriptorPools;
        std::vector<uint32_t> DescriptorPoolAllocationCount;


        std::unordered_map<UniformBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> UniformBufferWriteDescriptorCache;
        std::unordered_map<StorageBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> StorageBufferWriteDescriptorCache;
    };

    namespace Utils
    {
        static const char* VulkanVendorIDToString(uint32_t vendorID)
        {
            switch (vendorID)
            {
            case 0x10DE: return "NVIDIA";
            case 0x1002: return "AMD";
            case 0x8086: return "INTEL";
            case 0x13B5: return "ARM";
            default: return "Unknown";
            }
        }
    }

    static VKRendererData* sData = nullptr;

    void VKRenderer::Init()
    {
        sData = new VKRendererData();
        const auto& config = Renderer::GetConfig();
        sData->DescriptorPools.resize(config.FramesInFlight);
        sData->DescriptorPoolAllocationCount.resize(config.FramesInFlight);

        auto& caps = sData->RenderCaps;
        auto& properties = VKContext::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
        caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
        caps.Device = properties.deviceName;
        caps.Version = std::to_string(properties.driverVersion);
        Utils::DumpGPUInfo();

        // Create descriptor pools
        Renderer::Submit([]() mutable
            {
                VkDescriptorPoolSize pool_sizes[] =
                {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };

                VkDescriptorPoolCreateInfo pool_info = {};
                pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                pool_info.maxSets = 10000;
                pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
                pool_info.pPoolSizes = pool_sizes;

                VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
                uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
                for (uint32_t i = 0; i < framesInFlight; ++i)
                {
                    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &sData->DescriptorPools[i]));
                    sData->DescriptorPoolAllocationCount[i] = 0;
                }
            });

        float x = -1;
        float y = -1;
        float width = 2, height = 2;
        struct QuadVertex
        {
            glm::vec3 Position;
            glm::vec2 TexCoord;
        };

        QuadVertex* data = new QuadVertex[4];

        data[0].Position = glm::vec3(x, y, 0.0f);
        data[0].TexCoord = glm::vec2(0, 0);

        data[1].Position = glm::vec3(x + width, y, 0.0f);
        data[1].TexCoord = glm::vec2(1, 0);

        data[2].Position = glm::vec3(x + width, y + height, 0.0f);
        data[2].TexCoord = glm::vec2(1, 1);

        data[3].Position = glm::vec3(x, y + height, 0.0f);
        data[3].TexCoord = glm::vec2(0, 1);

        sData->QuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
        uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
        sData->QuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

        sData->BRDFLut = Renderer::GetBRDFLutTexture();
    }

    void VKRenderer::Shutdown()
    {
        VKShader::ClearUniformBuffers();
        delete sData;
    }

    RendererCapabilities& VKRenderer::GetCapabilities()
    {
        return sData->RenderCaps;
    }

    static const std::vector<std::vector<VkWriteDescriptorSet>>& RT_RetrieveOrCreateUniformBufferWriteDescriptors(Ref<UniformBufferSet> uniformBufferSet, Ref<VKMaterial> vulkanMaterial)
    {
        NR_PROFILE_FUNC();

        size_t shaderHash = vulkanMaterial->GetShader()->GetHash();
        if (sData->UniformBufferWriteDescriptorCache.find(uniformBufferSet.Raw()) != sData->UniformBufferWriteDescriptorCache.end())
        {
            const auto& shaderMap = sData->UniformBufferWriteDescriptorCache.at(uniformBufferSet.Raw());
            if (shaderMap.find(shaderHash) != shaderMap.end())
            {
                const auto& writeDescriptors = shaderMap.at(shaderHash);
                return writeDescriptors;
            }
        }

        uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
        Ref<VKShader> vulkanShader = vulkanMaterial->GetShader().As<VKShader>();
        if (vulkanShader->HasDescriptorSet(0))
        {
            const auto& shaderDescriptorSets = vulkanShader->GetShaderDescriptorSets();
            if (!shaderDescriptorSets.empty())
            {
                for (auto&& [binding, shaderUB] : shaderDescriptorSets[0].UniformBuffers)
                {
                    auto& writeDescriptors = sData->UniformBufferWriteDescriptorCache[uniformBufferSet.Raw()][shaderHash];
                    writeDescriptors.resize(framesInFlight);
                    for (uint32_t frame = 0; frame < framesInFlight; ++frame)
                    {
                        Ref<VKUniformBuffer> uniformBuffer = uniformBufferSet->Get(binding, 0, frame); // set = 0 for now
                        VkWriteDescriptorSet writeDescriptorSet = {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.descriptorCount = 1;
                        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        writeDescriptorSet.pBufferInfo = &uniformBuffer->GetDescriptorBufferInfo();
                        writeDescriptorSet.dstBinding = uniformBuffer->GetBinding();
                        writeDescriptors[frame].push_back(writeDescriptorSet);
                    }
                }
            }
        }
        return sData->UniformBufferWriteDescriptorCache[uniformBufferSet.Raw()][shaderHash];
    }

    static const std::vector<std::vector<VkWriteDescriptorSet>>& RT_RetrieveOrCreateStorageBufferWriteDescriptors(Ref<StorageBufferSet> storageBufferSet, Ref<VKMaterial> vulkanMaterial)
    {
        NR_PROFILE_FUNC();

        size_t shaderHash = vulkanMaterial->GetShader()->GetHash();
        if (sData->StorageBufferWriteDescriptorCache.find(storageBufferSet.Raw()) != sData->StorageBufferWriteDescriptorCache.end())
        {
            const auto& shaderMap = sData->StorageBufferWriteDescriptorCache.at(storageBufferSet.Raw());
            if (shaderMap.find(shaderHash) != shaderMap.end())
            {
                const auto& writeDescriptors = shaderMap.at(shaderHash);
                return writeDescriptors;
            }
        }

        uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
        Ref<VKShader> vulkanShader = vulkanMaterial->GetShader().As<VKShader>();
        if (vulkanShader->HasDescriptorSet(0))
        {
            const auto& shaderDescriptorSets = vulkanShader->GetShaderDescriptorSets();
            if (!shaderDescriptorSets.empty())
            {
                for (auto&& [binding, shaderSB] : shaderDescriptorSets[0].StorageBuffers)
                {
                    auto& writeDescriptors = sData->StorageBufferWriteDescriptorCache[storageBufferSet.Raw()][shaderHash];
                    writeDescriptors.resize(framesInFlight);

                    for (uint32_t frame = 0; frame < framesInFlight; ++frame)
                    {
                        Ref<VKStorageBuffer> storageBuffer = storageBufferSet->Get(binding, 0, frame);

                        VkWriteDescriptorSet writeDescriptorSet = {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.descriptorCount = 1;
                        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        writeDescriptorSet.pBufferInfo = &storageBuffer->GetDescriptorBufferInfo();
                        writeDescriptorSet.dstBinding = storageBuffer->GetBinding();
                        writeDescriptors[frame].push_back(writeDescriptorSet);
                    }
                }
            }
        }

        return sData->StorageBufferWriteDescriptorCache[storageBufferSet.Raw()][shaderHash];
    }

    static void RT_UpdateMaterialForRendering(Ref<VKMaterial> material, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet)
    {
        if (uniformBufferSet)
        {
            auto writeDescriptors = RT_RetrieveOrCreateUniformBufferWriteDescriptors(uniformBufferSet, material);
            if (storageBufferSet)
            {
                const auto& storageBufferWriteDescriptors = RT_RetrieveOrCreateStorageBufferWriteDescriptors(storageBufferSet, material);
                
                uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
                for (uint32_t frame = 0; frame < framesInFlight; ++frame)
                {
                    writeDescriptors[frame].reserve(writeDescriptors[frame].size() + storageBufferWriteDescriptors[frame].size());
                    writeDescriptors[frame].insert(writeDescriptors[frame].end(), storageBufferWriteDescriptors[frame].begin(), storageBufferWriteDescriptors[frame].end());
                }
            }
            material->RT_UpdateForRendering(writeDescriptors);
        }
        else
        {
            material->RT_UpdateForRendering();
        }
    }

    void VKRenderer::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, const glm::mat4& transform)
    {
        Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, transform]() mutable
            {
                NR_PROFILE_FUNC("VulkanRenderer::RenderMesh");
                NR_SCOPE_PERF("VulkanRenderer::RenderMesh");

                uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
                VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

                Ref<MeshAsset> meshAsset = mesh->GetMeshAsset();
                Ref<VKVertexBuffer> vulkanMeshVB = meshAsset->GetVertexBuffer().As<VKVertexBuffer>();

                VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
                VkDeviceSize offsets[1] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

                auto vulkanMeshIB = Ref<VKIndexBuffer>(meshAsset->GetIndexBuffer());
                VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
                vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

                Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
                VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                std::vector<std::vector<VkWriteDescriptorSet>> writeDescriptors;

                auto& materials = mesh->GetMaterials();
                for (auto& material : materials)
                {
                    Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
                    RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);
                }

                const auto& meshAssetSubmeshes = meshAsset->GetSubmeshes();
                auto& submeshes = mesh->GetSubmeshes();
                for (uint32_t submeshIndex : submeshes)
                {
                    const Submesh& submesh = meshAssetSubmeshes[submeshIndex];
                    auto material = mesh->GetMaterials()[submesh.MaterialIndex].As<VKMaterial>();
                    material->RT_UpdateForRendering();

                    VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
                    VkDescriptorSet descriptorSet = material->GetDescriptorSet(frameIndex);

                    // Bind descriptor sets describing shader binding points
                    std::array<VkDescriptorSet, 2> descriptorSets = {
                        descriptorSet,
                        sData->ActiveRendererDescriptorSet
                    };
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

                    glm::mat4 worldTransform = transform * submesh.Transform;

                    Buffer uniformStorageBuffer = material->GetUniformStorageBuffer();
                    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
                    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
                    vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
                }
            });
    }

    void VKRenderer::RenderParticles(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, const glm::mat4& transform)
    {
        //TODO
    }

    void VKRenderer::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms)
    {
        NR_CORE_ASSERT(mesh);
        NR_CORE_ASSERT(mesh->GetMeshAsset());

        Buffer pushConstantBuffer;
        pushConstantBuffer.Allocate(sizeof(glm::mat4) + additionalUniforms.Size);
        if (additionalUniforms.Size)
        {
            pushConstantBuffer.Write(additionalUniforms.Data, additionalUniforms.Size, sizeof(glm::mat4));
        }

        Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
        Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, vulkanMaterial, transform, pushConstantBuffer]() mutable
            {
                NR_PROFILE_FUNC("VulkanRenderer::RenderMeshWithMaterial");
                NR_SCOPE_PERF("VulkanRenderer::RenderMeshWithMaterial");

                uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
                VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

                Ref<MeshAsset> meshAsset = mesh->GetMeshAsset();
                auto vulkanMeshVB = meshAsset->GetVertexBuffer().As<VKVertexBuffer>();

                VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
                VkDeviceSize offsets[1] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

                auto vulkanMeshIB = Ref<VKIndexBuffer>(meshAsset->GetIndexBuffer());
                VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
                vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

                RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

                Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
                VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
                VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                float lineWidth = vulkanPipeline->GetSpecification().LineWidth;
                if (lineWidth != 1.0f)
                {
                    vkCmdSetLineWidth(commandBuffer, lineWidth);
                }

                // Bind descriptor sets describing shader binding points
                VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);
                if (descriptorSet)
                {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
                }

                Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
                if (uniformStorageBuffer)
                {
                    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantBuffer.Size, uniformStorageBuffer.Size, uniformStorageBuffer.Data);
                }

                const auto& meshAssetSubmeshes = meshAsset->GetSubmeshes();
                auto& submeshes = mesh->GetSubmeshes();
                for (uint32_t submeshIndex : submeshes)
                {
                    const Submesh& submesh = meshAssetSubmeshes[submeshIndex];
                    glm::mat4 worldTransform = transform * submesh.Transform;
                    pushConstantBuffer.Write(&worldTransform, sizeof(glm::mat4));
                    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, pushConstantBuffer.Size, pushConstantBuffer.Data);
                    vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
                }

                pushConstantBuffer.Release();
            });
    }

    void VKRenderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::mat4& transform)
    {
        Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();

        Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, vulkanMaterial, transform]() mutable
            {
                NR_PROFILE_FUNC("VulkanRenderer::RenderQuad");

                uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
                VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

                Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();

                VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

                auto vulkanMeshVB = sData->QuadVertexBuffer.As<VKVertexBuffer>();
                VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
                VkDeviceSize offsets[1] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

                auto vulkanMeshIB = sData->QuadIndexBuffer.As<VKIndexBuffer>();
                VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
                vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

                VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

                uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();

                VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
                if (descriptorSet)
                {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
                }

                Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();

                vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
                vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
                vkCmdDrawIndexed(commandBuffer, sData->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
            });
    }

    void VKRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount)
    {
        Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
        if (indexCount == 0)
            indexCount = indexBuffer->GetCount();
        Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, vulkanMaterial, vertexBuffer, indexBuffer, transform, indexCount]() mutable
            {
                NR_PROFILE_FUNC("VulkanRenderer::RenderGeometry");

                uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

                VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);
                
                Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
                VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
                
                auto vulkanMeshVB = vertexBuffer.As<VKVertexBuffer>();
                VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
                VkDeviceSize offsets[1] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);
                
                auto vulkanMeshIB = indexBuffer.As<VKIndexBuffer>();
                VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
                vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);
                VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

                uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();

                VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
                if (descriptorSet)
                {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
                }

                vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
                Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
                if (uniformStorageBuffer)
                {
                    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
                }

                vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
            });
    }

    void VKRenderer::LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups)
    {
        auto vulkanMaterial = material.As<VKMaterial>();
        Renderer::Submit([renderCommandBuffer, pipeline, vulkanMaterial, uniformBufferSet, storageBufferSet, screenSize, workGroups]() mutable
            {
                const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
                RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

                const VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);

                pipeline->Begin(renderCommandBuffer);
                pipeline->SetPushConstants(glm::value_ptr(screenSize), sizeof(glm::ivec2));
                pipeline->Dispatch(descriptorSet, workGroups.x, workGroups.y, workGroups.z);
                pipeline->End();
            });
    }

    VkDescriptorSet VKRenderer::RT_AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo)
    {
        NR_PROFILE_FUNC();

        uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
        allocInfo.descriptorPool = sData->DescriptorPools[bufferIndex];

        VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
        VkDescriptorSet result;
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &result));

        sData->DescriptorPoolAllocationCount[bufferIndex] += allocInfo.descriptorSetCount;
        return result;
    }

    void VKRenderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material)
    {
        Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();

        Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, vulkanMaterial]() mutable
            {
                NR_PROFILE_FUNC("VulkanRenderer::SubmitFullscreenQuad");

                uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
                VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

                Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();

                VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

                auto vulkanMeshVB = sData->QuadVertexBuffer.As<VKVertexBuffer>();
                VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
                VkDeviceSize offsets[1] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

                auto vulkanMeshIB = sData->QuadIndexBuffer.As<VKIndexBuffer>();
                VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
                vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

                VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

                uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();

                VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
                if (descriptorSet)
                {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
                }

                Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
                if (uniformStorageBuffer.Size)
                {
                    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, uniformStorageBuffer.Size, uniformStorageBuffer.Data);
                }

                vkCmdDrawIndexed(commandBuffer, sData->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
            });
    }

    void VKRenderer::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow)
    {
        if (!environment)
        {
            environment = Renderer::GetEmptyEnvironment();
        }

        Renderer::Submit([sceneRenderer, environment, shadow]() mutable
            {
                NR_PROFILE_FUNC("VulkanRenderer::SetSceneEnvironment");

                auto shader = Renderer::GetShaderLibrary()->Get("PBR_Static");
                Ref<VKShader> pbrShader = shader.As<VKShader>();
                uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
                
                if (sData->RendererDescriptorSet.find(sceneRenderer.Raw()) == sData->RendererDescriptorSet.end())
                {
                    uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
                    sData->RendererDescriptorSet[sceneRenderer.Raw()].resize(framesInFlight);

                    for (uint32_t i = 0; i < framesInFlight; ++i)
                    {
                        sData->RendererDescriptorSet.at(sceneRenderer.Raw())[i] = pbrShader->CreateDescriptorSets(1);
                    }
                }

                VkDescriptorSet descriptorSet = sData->RendererDescriptorSet.at(sceneRenderer.Raw())[bufferIndex].DescriptorSets[0];
                sData->ActiveRendererDescriptorSet = descriptorSet;

                std::array<VkWriteDescriptorSet, 4> writeDescriptors;

                Ref<VKTextureCube> radianceMap = environment->RadianceMap.As<VKTextureCube>();
                Ref<VKTextureCube> irradianceMap = environment->IrradianceMap.As<VKTextureCube>();

                writeDescriptors[0] = *pbrShader->GetDescriptorSet("uEnvRadianceTex", 1);
                writeDescriptors[0].dstSet = descriptorSet;
                const auto& radianceMapImageInfo = radianceMap->GetVulkanDescriptorInfo();
                writeDescriptors[0].pImageInfo = &radianceMapImageInfo;

                writeDescriptors[1] = *pbrShader->GetDescriptorSet("uEnvIrradianceTex", 1);
                writeDescriptors[1].dstSet = descriptorSet;
                const auto& irradianceMapImageInfo = irradianceMap->GetVulkanDescriptorInfo();
                writeDescriptors[1].pImageInfo = &irradianceMapImageInfo;

                writeDescriptors[2] = *pbrShader->GetDescriptorSet("uBRDFLUTTexture", 1);
                writeDescriptors[2].dstSet = descriptorSet;
                const auto& brdfLutImageInfo = sData->BRDFLut.As<VKTexture2D>()->GetVulkanDescriptorInfo();
                writeDescriptors[2].pImageInfo = &brdfLutImageInfo;

                writeDescriptors[3] = *pbrShader->GetDescriptorSet("uShadowMapTexture", 1);
                writeDescriptors[3].dstSet = descriptorSet;
                const auto& shadowImageInfo = shadow.As<VKImage2D>()->GetDescriptor();
                writeDescriptors[3].pImageInfo = &shadowImageInfo;

                auto vulkanDevice = VKContext::GetCurrentDevice()->GetVulkanDevice();
                vkUpdateDescriptorSets(vulkanDevice, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
            });
    }

    void VKRenderer::BeginFrame()
    {
        Renderer::Submit([]()
            {
                NR_PROFILE_FUNC("VulkanRenderer::BeginFrame");

                VKSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();

                // Reset descriptor pools here
                VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
                uint32_t bufferIndex = swapChain.GetCurrentBufferIndex();
                vkResetDescriptorPool(device, sData->DescriptorPools[bufferIndex], 0);
                memset(sData->DescriptorPoolAllocationCount.data(), 0, sData->DescriptorPoolAllocationCount.size() * sizeof(uint32_t));
            });
    }

    void VKRenderer::EndFrame()
    {
    }

    void VKRenderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, const Ref<RenderPass>& renderPass, bool explicitClear)
    {
        Renderer::Submit([renderCommandBuffer, renderPass, explicitClear]()
            {
                NR_PROFILE_FUNC("VulkanRenderer::BeginRenderPass");

                uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
                VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

                auto fb = renderPass->GetSpecification().TargetFrameBuffer;
                Ref<VKFrameBuffer> framebuffer = fb.As<VKFrameBuffer>();
                const auto& fbSpec = framebuffer->GetSpecification();

                uint32_t width = framebuffer->GetWidth();
                uint32_t height = framebuffer->GetHeight();

                VkViewport viewport = {};
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.pNext = nullptr;
                renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
                renderPassBeginInfo.renderArea.offset.x = 0;
                renderPassBeginInfo.renderArea.offset.y = 0;
                renderPassBeginInfo.renderArea.extent.width = width;
                renderPassBeginInfo.renderArea.extent.height = height;

                if (framebuffer->GetSpecification().SwapChainTarget)
                {
                    VKSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();

                    width = swapChain.GetWidth();
                    height = swapChain.GetHeight();

                    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassBeginInfo.pNext = nullptr;
                    renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
                    renderPassBeginInfo.renderArea.offset.x = 0;
                    renderPassBeginInfo.renderArea.offset.y = 0;
                    renderPassBeginInfo.renderArea.extent.width = width;
                    renderPassBeginInfo.renderArea.extent.height = height;
                    renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

                    viewport.x = 0.0f;
                    viewport.y = (float)height;
                    viewport.width = (float)width;
                    viewport.height = -(float)height;
                }
                else
                {
                    width = framebuffer->GetWidth();
                    height = framebuffer->GetHeight();

                    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassBeginInfo.pNext = nullptr;
                    renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
                    renderPassBeginInfo.renderArea.offset.x = 0;
                    renderPassBeginInfo.renderArea.offset.y = 0;
                    renderPassBeginInfo.renderArea.extent.width = width;
                    renderPassBeginInfo.renderArea.extent.height = height;
                    renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFrameBuffer();
                    
                    viewport.x = 0.0f;
                    viewport.y = 0.0f;
                    viewport.width = (float)width;
                    viewport.height = (float)height;
                }

                const auto& clearValues = framebuffer->GetVulkanClearValues();

                renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
                renderPassBeginInfo.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                if (explicitClear)
                {
                    uint32_t colorAttachmentCount = (uint32_t)framebuffer->GetColorAttachmentCount();
                    uint32_t totalAttachmentCount = colorAttachmentCount + (framebuffer->HasDepthAttachment() ? 1 : 0);
                    
                    NR_CORE_ASSERT(clearValues.size() == totalAttachmentCount);
                    
                    std::vector<VkClearAttachment> attachments(totalAttachmentCount);
                    std::vector<VkClearRect> clearRects(totalAttachmentCount);
                    
                    for (uint32_t i = 0; i < colorAttachmentCount; ++i)
                    {
                        attachments[i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        attachments[i].colorAttachment = i;
                        attachments[i].clearValue = clearValues[i];
                        clearRects[i].rect.offset = { (int32_t)0, (int32_t)0 };
                        clearRects[i].rect.extent = { width, height };
                        clearRects[i].baseArrayLayer = 0;
                        clearRects[i].layerCount = 1;
                    }
                    if (framebuffer->HasDepthAttachment())
                    {
                        attachments[colorAttachmentCount].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                        attachments[colorAttachmentCount].clearValue = clearValues[colorAttachmentCount];
                        clearRects[colorAttachmentCount].rect.offset = { (int32_t)0, (int32_t)0 };
                        clearRects[colorAttachmentCount].rect.extent = { width, height };
                        clearRects[colorAttachmentCount].baseArrayLayer = 0;
                        clearRects[colorAttachmentCount].layerCount = 1;
                    }

                    vkCmdClearAttachments(commandBuffer, totalAttachmentCount, attachments.data(), totalAttachmentCount, clearRects.data());
                }

                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                // Update dynamic scissor state
                VkRect2D scissor = {};
                scissor.extent.width = width;
                scissor.extent.height = height;
                scissor.offset.x = 0;
                scissor.offset.y = 0;
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            });
    }

    void VKRenderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
    {
        Renderer::Submit([renderCommandBuffer]()
            {
                NR_PROFILE_FUNC("VulkanRenderer::EndRenderPass");

                uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
                VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);
                vkCmdEndRenderPass(commandBuffer);
            });
    }

    std::pair<Ref<TextureCube>, Ref<TextureCube>> VKRenderer::CreateEnvironmentMap(const std::string& filepath)
    {
        if (!Renderer::GetConfig().ComputeEnvironmentMaps)
        {
            return { Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture() };
        }

        const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
        const uint32_t irradianceMapSize = 32;

        Ref<Texture2D> envEquirect = Texture2D::Create(filepath);
        NR_CORE_ASSERT(envEquirect->GetFormat() == ImageFormat::RGBA32F, "Texture is not HDR!");

        Ref<TextureCube> envUnfiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);
        Ref<TextureCube> envFiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);

        // Convert equirectangular to cubemap
        Ref<Shader> equirectangularConversionShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");
        Ref<VKComputePipeline> equirectangularConversionPipeline = Ref<VKComputePipeline>::Create(equirectangularConversionShader);

        Renderer::Submit([equirectangularConversionPipeline, envEquirect, envUnfiltered, cubemapSize]() mutable
            {
                VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
                Ref<VKShader> shader = equirectangularConversionPipeline->GetShader();

                std::array<VkWriteDescriptorSet, 2> writeDescriptors;
                auto descriptorSet = shader->CreateDescriptorSets();
                Ref<VKTextureCube> envUnfilteredCubemap = envUnfiltered.As<VKTextureCube>();
                writeDescriptors[0] = *shader->GetDescriptorSet("oCubeMap");
                writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0];
                writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

                Ref<VKTexture2D> envEquirectVK = envEquirect.As<VKTexture2D>();
                writeDescriptors[1] = *shader->GetDescriptorSet("uEquirectangularTex");
                writeDescriptors[1].dstSet = descriptorSet.DescriptorSets[0];
                writeDescriptors[1].pImageInfo = &envEquirectVK->GetVulkanDescriptorInfo();

                vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
                equirectangularConversionPipeline->Execute(descriptorSet.DescriptorSets.data(), (uint32_t)descriptorSet.DescriptorSets.size(), cubemapSize / 32, cubemapSize / 32, 6);

                envUnfilteredCubemap->GenerateMips(true);
            });

        Ref<Shader> environmentMipFilterShader = Renderer::GetShaderLibrary()->Get("EnvironmentMipFilter");
        Ref<VKComputePipeline> environmentMipFilterPipeline = Ref<VKComputePipeline>::Create(environmentMipFilterShader);

        Renderer::Submit([environmentMipFilterPipeline, envUnfiltered, envFiltered, cubemapSize]() mutable
            {
                VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
                Ref<VKShader> shader = environmentMipFilterPipeline->GetShader();

                Ref<VKTextureCube> envFilteredCubemap = envFiltered.As<VKTextureCube>();
                VkDescriptorImageInfo imageInfo = envFilteredCubemap->GetVulkanDescriptorInfo();

                uint32_t mipCount = Utils::CalculateMipCount(cubemapSize, cubemapSize);
                std::vector<VkWriteDescriptorSet> writeDescriptors(mipCount * 2);
                std::vector<VkDescriptorImageInfo> mipImageInfos(mipCount);

                auto descriptorSet = shader->CreateDescriptorSets(0, 12);
                for (uint32_t i = 0; i < mipCount; ++i)
                {
                    VkDescriptorImageInfo& mipImageInfo = mipImageInfos[i];
                    mipImageInfo = imageInfo;
                    mipImageInfo.imageView = envFilteredCubemap->CreateImageViewSingleMip(i);

                    writeDescriptors[i * 2 + 0] = *shader->GetDescriptorSet("outputTexture");
                    writeDescriptors[i * 2 + 0].dstSet = descriptorSet.DescriptorSets[i];
                    writeDescriptors[i * 2 + 0].pImageInfo = &mipImageInfo;

                    Ref<VKTextureCube> envUnfilteredCubemap = envUnfiltered.As<VKTextureCube>();
                    writeDescriptors[i * 2 + 1] = *shader->GetDescriptorSet("inputTexture");
                    writeDescriptors[i * 2 + 1].dstSet = descriptorSet.DescriptorSets[i];
                    writeDescriptors[i * 2 + 1].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();
                }

                vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

                environmentMipFilterPipeline->Begin(); // begin compute pass
                const float deltaRoughness = 1.0f / glm::max((float)envFiltered->GetMipLevelCount() - 1.0f, 1.0f);
                for (uint32_t i = 0, size = cubemapSize; i < mipCount; ++i, size /= 2)
                {
                    uint32_t numGroups = glm::max(1u, size / 32);
                    float roughness = i * deltaRoughness;
                    roughness = glm::max(roughness, 0.05f);
                    environmentMipFilterPipeline->SetPushConstants(&roughness, sizeof(float));
                    environmentMipFilterPipeline->Dispatch(descriptorSet.DescriptorSets[i], numGroups, numGroups, 6);
                }
                environmentMipFilterPipeline->End();
            });

        Ref<Shader> environmentIrradianceShader = Renderer::GetShaderLibrary()->Get("EnvironmentIrradiance");
        Ref<VKComputePipeline> environmentIrradiancePipeline = Ref<VKComputePipeline>::Create(environmentIrradianceShader);
        Ref<TextureCube> irradianceMap = TextureCube::Create(ImageFormat::RGBA32F, irradianceMapSize, irradianceMapSize);

        Renderer::Submit([environmentIrradiancePipeline, irradianceMap, envFiltered]() mutable
            {
                VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
                Ref<VKShader> shader = environmentIrradiancePipeline->GetShader();

                Ref<VKTextureCube> envFilteredCubemap = envFiltered.As<VKTextureCube>();
                Ref<VKTextureCube> irradianceCubemap = irradianceMap.As<VKTextureCube>();
                auto descriptorSet = shader->CreateDescriptorSets();

                std::array<VkWriteDescriptorSet, 2> writeDescriptors;
                writeDescriptors[0] = *shader->GetDescriptorSet("oIrradianceMap");
                writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0];
                writeDescriptors[0].pImageInfo = &irradianceCubemap->GetVulkanDescriptorInfo();

                writeDescriptors[1] = *shader->GetDescriptorSet("uRadianceMap");
                writeDescriptors[1].dstSet = descriptorSet.DescriptorSets[0];
                writeDescriptors[1].pImageInfo = &envFilteredCubemap->GetVulkanDescriptorInfo();

                vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

                environmentIrradiancePipeline->Begin();
                environmentIrradiancePipeline->SetPushConstants(&Renderer::GetConfig().IrradianceMapComputeSamples, sizeof(uint32_t));
                environmentIrradiancePipeline->Dispatch(descriptorSet.DescriptorSets[0], irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
                environmentIrradiancePipeline->End();

                irradianceCubemap->GenerateMips();
            });

        return { envFiltered, irradianceMap };
    }

    void VKRenderer::GenerateParticles()
    {
        // Convert equirectangular to cubemap
        Ref<Shader> particleGenShader = Renderer::GetShaderLibrary()->Get("ParticleGen");
        Ref<VKComputePipeline> particleGenPipeline = Ref<VKComputePipeline>::Create(particleGenShader);
    }

    Ref<TextureCube> VKRenderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
    {
        const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
        const uint32_t irradianceMapSize = 32;

        Ref<TextureCube> environmentMap = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);
        Ref<Shader> preethamSkyShader = Renderer::GetShaderLibrary()->Get("PreethamSky");
        Ref<VKComputePipeline> preethamSkyComputePipeline = Ref<VKComputePipeline>::Create(preethamSkyShader);

        glm::vec3 params = { turbidity, azimuth, inclination };
        Renderer::Submit([preethamSkyComputePipeline, environmentMap, cubemapSize, params]() mutable
            {
                VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

                Ref<VKShader> shader = preethamSkyComputePipeline->GetShader();
                std::array<VkWriteDescriptorSet, 1> writeDescriptors;
                auto descriptorSet = shader->CreateDescriptorSets();
                Ref<VKTextureCube> envUnfilteredCubemap = environmentMap.As<VKTextureCube>();

                writeDescriptors[0] = *shader->GetDescriptorSet("oCubeMap");
                writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0];
                writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

                vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

                preethamSkyComputePipeline->Begin();
                preethamSkyComputePipeline->SetPushConstants(&params, sizeof(glm::vec3));
                preethamSkyComputePipeline->Dispatch(descriptorSet.DescriptorSets[0], cubemapSize / 32, cubemapSize / 32, 6);
                preethamSkyComputePipeline->End();

                envUnfilteredCubemap->GenerateMips(true);
            });

        return environmentMap;
    }
}