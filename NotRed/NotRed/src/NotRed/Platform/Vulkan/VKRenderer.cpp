#include "nrpch.h"
#include "VKRenderer.h"

#include <glm/glm.hpp>

#include "Vulkan.h"
#include "VkContext.h"

#include "NotRed/Renderer/Renderer.h"

#include "VKComputePipeline.h"

#include "VKPipeline.h"
#include "VKVertexBuffer.h"
#include "VKIndexBuffer.h"
#include "VKFrameBuffer.h"
#include "VKMaterial.h"

#include "NotRed/Platform/Vulkan/VKShader.h"
#include "NotRed/Platform/Vulkan/VKTexture.h"

#define IMGUI_IMPL_API
#include "backends/imgui_impl_glfw.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

#include "imgui.h"

namespace NR
{
    struct VKRendererData
    {
        RendererCapabilities RenderCaps;

        VkCommandBuffer ActiveCommandBuffer = nullptr;
        Ref<Texture2D> BRDFLut;
        VKShader::ShaderMaterialDescriptorSet RendererDescriptorSet;

        Ref<VertexBuffer> QuadVertexBuffer;
        Ref<IndexBuffer> QuadIndexBuffer;
        VKShader::ShaderMaterialDescriptorSet QuadDescriptorSet;
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

        auto& caps = sData->RenderCaps;
        auto& properties = VKContext::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
        caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
        caps.Device = properties.deviceName;
        caps.Version = std::to_string(properties.driverVersion);
        Utils::DumpGPUInfo();

        float x = -1;
        float y = -1;
        float width = 2, height = 2;
        struct QuadVertex
        {
            glm::vec3 Position;
            glm::vec2 TexCoord;
        };

        QuadVertex* data = new QuadVertex[4];

        data[0].Position = glm::vec3(x, y, 0.1f);
        data[0].TexCoord = glm::vec2(0, 0);

        data[1].Position = glm::vec3(x + width, y, 0.1f);
        data[1].TexCoord = glm::vec2(1, 0);

        data[2].Position = glm::vec3(x + width, y + height, 0.1f);
        data[2].TexCoord = glm::vec2(1, 1);

        data[3].Position = glm::vec3(x, y + height, 0.1f);
        data[3].TexCoord = glm::vec2(0, 1);

        sData->QuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
        uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
        sData->QuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

        {
            TextureProperties props;
            props.SamplerWrap = TextureWrap::Clamp;
            sData->BRDFLut = Texture2D::Create("Assets/Textures/BRDF_LUT.tga", props);
        }

        Renderer::Submit([]() mutable
            {
                auto shader = Renderer::GetShaderLibrary()->Get("PBR_Static");
                Ref<VKShader> pbrShader = shader.As<VKShader>();
                sData->RendererDescriptorSet = pbrShader->CreateDescriptorSets(1);
            });

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

    void VKRenderer::RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform)
    {
        Renderer::Submit([pipeline, mesh, transform]() mutable
            {
                auto vulkanMeshVB = mesh->GetVertexBuffer().As<VKVertexBuffer>();
                VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
                VkDeviceSize offsets[1] = { 0 };
                vkCmdBindVertexBuffers(sData->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

                auto vulkanMeshIB = Ref<VKIndexBuffer>(mesh->GetIndexBuffer());
                VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
                vkCmdBindIndexBuffer(sData->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

                Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
                VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
                vkCmdBindPipeline(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            });

        auto& submeshes = mesh->GetSubmeshes();
        for (Submesh& submesh : submeshes)
        {
            auto material = mesh->GetMaterials()[submesh.MaterialIndex].As<VKMaterial>();
            material->UpdateForRendering();

            Renderer::Submit([pipeline, submesh, material, transform]() mutable
                {
                    Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
                    VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

                    // Bind descriptor sets describing shader binding points
                    std::array<VkDescriptorSet, 2> descriptorSets = {
                        material->GetDescriptorSet().DescriptorSets[0],
                        sData->RendererDescriptorSet.DescriptorSets[0]
                    };
                    vkCmdBindDescriptorSets(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

                    glm::mat4 worldTransform = transform * submesh.Transform;

                    Buffer uniformStorageBuffer = material->GetUniformStorageBuffer();
                    vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
                    vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
                    vkCmdDrawIndexed(sData->ActiveCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
                });
        }
    }

    void VKRenderer::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform)
    {
        Renderer::Submit([pipeline, mesh, transform]() mutable
            {
                auto vulkanMeshVB = mesh->GetVertexBuffer().As<VKVertexBuffer>();
                VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
                VkDeviceSize offsets[1] = { 0 };
                vkCmdBindVertexBuffers(sData->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

                auto vulkanMeshIB = Ref<VKIndexBuffer>(mesh->GetIndexBuffer());
                VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
                vkCmdBindIndexBuffer(sData->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);
            });

        auto& submeshes = mesh->GetSubmeshes();
        for (Submesh& submesh : submeshes)
        {
            Renderer::Submit([pipeline, submesh, transform]() mutable
                {
                    Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
                    VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
                    VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
                    vkCmdBindPipeline(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                    // Bind descriptor sets describing shader binding points
                    VkDescriptorSet descriptorSet = {
                        vulkanPipeline->GetDescriptorSet()
                    };
                    vkCmdBindDescriptorSets(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

                    glm::mat4 worldTransform = transform * submesh.Transform;
                    vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
                    vkCmdDrawIndexed(sData->ActiveCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
                });
        }
    }

    void VKRenderer::RenderQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform)
    {
        Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
        vulkanMaterial->UpdateForRendering();

        Renderer::Submit([pipeline, vulkanMaterial, transform]() mutable
            {
                Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();

                VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

                auto vulkanMeshVB = sData->QuadVertexBuffer.As<VKVertexBuffer>();
                VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
                VkDeviceSize offsets[1] = { 0 };
                vkCmdBindVertexBuffers(sData->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

                auto vulkanMeshIB = sData->QuadIndexBuffer.As<VKIndexBuffer>();
                VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
                vkCmdBindIndexBuffer(sData->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

                VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
                vkCmdBindPipeline(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                // Bind descriptor sets describing shader binding points
                vkCmdBindDescriptorSets(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &vulkanMaterial->GetDescriptorSet().DescriptorSets[0], 0, nullptr);

                Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();

                vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
                vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
                vkCmdDrawIndexed(sData->ActiveCommandBuffer, sData->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
            });
    }

    void VKRenderer::SubmitFullScreenQuad(Ref<Pipeline> pipeline, Ref<Material> material)
    {
        Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
        vulkanMaterial->UpdateForRendering();

        Renderer::Submit([pipeline, vulkanMaterial]() mutable
            {
                Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();

                VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

                auto vulkanMeshVB = sData->QuadVertexBuffer.As<VKVertexBuffer>();
                VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
                VkDeviceSize offsets[1] = { 0 };
                vkCmdBindVertexBuffers(sData->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

                auto vulkanMeshIB = sData->QuadIndexBuffer.As<VKIndexBuffer>();
                VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
                vkCmdBindIndexBuffer(sData->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

                VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
                vkCmdBindPipeline(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                // Bind descriptor sets describing shader binding points
                const auto& descriptorSets = vulkanMaterial->GetDescriptorSet().DescriptorSets;
                if (!descriptorSets.empty())
                {
                    vkCmdBindDescriptorSets(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSets[0], 0, nullptr);
                }

                Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
                if (uniformStorageBuffer.Size)
                {
                    vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, uniformStorageBuffer.Size, uniformStorageBuffer.Data);
                }
                vkCmdDrawIndexed(sData->ActiveCommandBuffer, sData->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
            });
    }

    void VKRenderer::SetSceneEnvironment(Ref<Environment> environment, Ref<Image2D> shadow)
    {
        if (!environment)
        {
            environment = Renderer::GetEmptyEnvironment();
        }

        Renderer::Submit([environment, shadow]() mutable
            {
                auto shader = Renderer::GetShaderLibrary()->Get("PBR_Static");
                Ref<VKShader> pbrShader = shader.As<VKShader>();

                std::array<VkWriteDescriptorSet, 4> writeDescriptors;

                Ref<VKTextureCube> radianceMap = environment->RadianceMap.As<VKTextureCube>();
                Ref<VKTextureCube> irradianceMap = environment->IrradianceMap.As<VKTextureCube>();

                writeDescriptors[0] = *pbrShader->GetDescriptorSet("uEnvRadianceTex", 1);
                writeDescriptors[0].dstSet = sData->RendererDescriptorSet.DescriptorSets[0];
                const auto& radianceMapImageInfo = radianceMap->GetVulkanDescriptorInfo();
                writeDescriptors[0].pImageInfo = &radianceMapImageInfo;

                writeDescriptors[1] = *pbrShader->GetDescriptorSet("uEnvIrradianceTex", 1);
                writeDescriptors[1].dstSet = sData->RendererDescriptorSet.DescriptorSets[0];
                const auto& irradianceMapImageInfo = irradianceMap->GetVulkanDescriptorInfo();
                writeDescriptors[1].pImageInfo = &irradianceMapImageInfo;

                writeDescriptors[2] = *pbrShader->GetDescriptorSet("uBRDFLUTTexture", 1);
                writeDescriptors[2].dstSet = sData->RendererDescriptorSet.DescriptorSets[0];
                const auto& brdfLutImageInfo = sData->BRDFLut.As<VKTexture2D>()->GetVulkanDescriptorInfo();
                writeDescriptors[2].pImageInfo = &brdfLutImageInfo;

                writeDescriptors[3] = *pbrShader->GetDescriptorSet("uShadowMapTexture", 1);
                writeDescriptors[3].dstSet = sData->RendererDescriptorSet.DescriptorSets[0];
                const auto& shadowImageInfo = shadow.As<VKImage2D>()->GetDescriptor();
                writeDescriptors[3].pImageInfo = &shadowImageInfo;

                auto vulkanDevice = VKContext::GetCurrentDevice()->GetVulkanDevice();
                vkUpdateDescriptorSets(vulkanDevice, writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
            });
    }

    void VKRenderer::BeginFrame()
    {
        Renderer::Submit([]()
            {
                Ref<VKContext> context = VKContext::Get();
                VKSwapChain& swapChain = context->GetSwapChain();

                VkCommandBufferBeginInfo cmdBufInfo = {};
                cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                cmdBufInfo.pNext = nullptr;

                VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
                sData->ActiveCommandBuffer = drawCommandBuffer;
                NR_CORE_ASSERT(sData->ActiveCommandBuffer);
                VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));
            });
    }

    void VKRenderer::EndFrame()
    {
        Renderer::Submit([]()
            {
                VK_CHECK_RESULT(vkEndCommandBuffer(sData->ActiveCommandBuffer));
                sData->ActiveCommandBuffer = nullptr;
            });
    }

    void VKRenderer::BeginRenderPass(const Ref<RenderPass>& renderPass)
    {
        Renderer::Submit([renderPass]()
            {
                NR_CORE_ASSERT(sData->ActiveCommandBuffer);

                auto fb = renderPass->GetSpecification().TargetFrameBuffer;
                Ref<VKFrameBuffer> framebuffer = fb.As<VKFrameBuffer>();
                const auto& fbSpec = framebuffer->GetSpecification();

                uint32_t width = framebuffer->GetWidth();
                uint32_t height = framebuffer->GetHeight();

                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.pNext = nullptr;
                renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
                renderPassBeginInfo.renderArea.offset.x = 0;
                renderPassBeginInfo.renderArea.offset.y = 0;
                renderPassBeginInfo.renderArea.extent.width = width;
                renderPassBeginInfo.renderArea.extent.height = height;

                const auto& clearValues = framebuffer->GetVulkanClearValues();

                renderPassBeginInfo.clearValueCount = clearValues.size();
                renderPassBeginInfo.pClearValues = clearValues.data();
                renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFrameBuffer();

                vkCmdBeginRenderPass(sData->ActiveCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                VkViewport viewport = {};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.height = (float)height;
                viewport.width = (float)width;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(sData->ActiveCommandBuffer, 0, 1, &viewport);

                // Update dynamic scissor state
                VkRect2D scissor = {};
                scissor.extent.width = width;
                scissor.extent.height = height;
                scissor.offset.x = 0;
                scissor.offset.y = 0;
                vkCmdSetScissor(sData->ActiveCommandBuffer, 0, 1, &scissor);
            });
    }

    void VKRenderer::EndRenderPass()
    {
        Renderer::Submit([]()
            {
                vkCmdEndRenderPass(sData->ActiveCommandBuffer);
            });
    }

    std::pair<Ref<TextureCube>, Ref<TextureCube>> VKRenderer::CreateEnvironmentMap(const std::string& filepath)
    {
        if (!Renderer::GetConfig().ComputeEnvironmentMaps)
        {
            return { Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture() };
        }

        const uint32_t cubemapSize = 2048;
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

                vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
                equirectangularConversionPipeline->Execute(descriptorSet.DescriptorSets.data(), descriptorSet.DescriptorSets.size(), cubemapSize / 32, cubemapSize / 32, 6);

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

                std::array<VkWriteDescriptorSet, 24> writeDescriptors;
                std::array<VkDescriptorImageInfo, 12> mipImageInfos;
                auto descriptorSet = shader->CreateDescriptorSets(0, 12);
                for (uint32_t i = 0; i < 12; ++i)
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

                vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

                environmentMipFilterPipeline->Begin(); // begin compute pass
                const float deltaRoughness = 1.0f / glm::max((float)envFiltered->GetMipLevelCount() - 1.0f, 1.0f);
                for (uint32_t i = 0, size = cubemapSize; i < 12; ++i, size /= 2)
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

                vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
                environmentIrradiancePipeline->Execute(descriptorSet.DescriptorSets.data(), descriptorSet.DescriptorSets.size(), irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);

                irradianceCubemap->GenerateMips();
            });

        return { envFiltered, irradianceMap };
    }

    Ref<TextureCube> VKRenderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
    {
        const uint32_t cubemapSize = 2048;
        const uint32_t irradianceMapSize = 32;
        Ref<TextureCube> envUnfiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);
        Ref<Shader> preethamSkyShader = Renderer::GetShaderLibrary()->Get("PreethamSky");
        Ref<VKComputePipeline> preethamSkyComputePipeline = Ref<VKComputePipeline>::Create(preethamSkyShader);
        glm::vec3 params = { turbidity, azimuth, inclination };
        Renderer::Submit([preethamSkyComputePipeline, envUnfiltered, cubemapSize, params]() mutable
            {
                VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
                Ref<VKShader> shader = preethamSkyComputePipeline->GetShader();
                std::array<VkWriteDescriptorSet, 1> writeDescriptors;
                auto descriptorSet = shader->CreateDescriptorSets();
                Ref<VKTextureCube> envUnfilteredCubemap = envUnfiltered.As<VKTextureCube>();
                writeDescriptors[0] = *shader->GetDescriptorSet("oCubeMap");
                writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0];
                writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();
                vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
                preethamSkyComputePipeline->Begin();
                preethamSkyComputePipeline->SetPushConstants(&params, sizeof(glm::vec3));
                preethamSkyComputePipeline->Dispatch(descriptorSet.DescriptorSets[0], cubemapSize / 32, cubemapSize / 32, 6);
                preethamSkyComputePipeline->End();
                envUnfilteredCubemap->GenerateMips(true);
            });
        return envUnfiltered;
    }
}