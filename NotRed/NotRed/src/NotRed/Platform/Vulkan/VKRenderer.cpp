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
#include "VKUniformBuffer.h"

#include "VKShader.h"
#include "VKTexture.h"

#define IMGUI_IMPL_API
#include "backends/imgui_impl_glfw.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

#include "imgui.h"

#include "NotRed/Core/Timer.h"

namespace NR
{
	struct VKRendererData
	{
		RendererCapabilities RenderCaps;

		VkCommandBuffer ActiveCommandBuffer = nullptr;
		Ref<Texture2D> BRDFLut;
		VKShader::ShaderMaterialDescriptorSet RendererDescriptorSet;

		VKShader::ShaderMaterialDescriptorSet ParticleDescriptorSet;

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
				NR_SCOPE_PERF("VulkanRenderer::RenderMesh");

				Ref<VKVertexBuffer> vulkanMeshVB = mesh->GetVertexBuffer().As<VKVertexBuffer>();

				VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
				VkDeviceSize offsets[1] = { 0 };
				vkCmdBindVertexBuffers(sData->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

				auto vulkanMeshIB = Ref<VKIndexBuffer>(mesh->GetIndexBuffer());
				VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
				vkCmdBindIndexBuffer(sData->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

				Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
				VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
				vkCmdBindPipeline(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				auto& submeshes = mesh->GetSubmeshes();
				for (Submesh& submesh : submeshes)
				{
					auto material = mesh->GetMaterials()[submesh.MaterialIndex].As<VKMaterial>();
					material->RT_UpdateForRendering();

					VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
					VkDescriptorSet descriptorSet = material->GetDescriptorSet();

					// Bind descriptor sets describing shader binding points
					std::array<VkDescriptorSet, 2> descriptorSets = {
						descriptorSet,
						sData->RendererDescriptorSet.DescriptorSets[0]
					};
					//dewd;
					vkCmdBindDescriptorSets(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

					glm::mat4 worldTransform = transform * submesh.Transform;

					Buffer uniformStorageBuffer = material->GetUniformStorageBuffer();
					vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
					vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
					vkCmdDrawIndexed(sData->ActiveCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
				}
			});
	}

	void VKRenderer::RenderParticles(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform)
	{
		Renderer::Submit([pipeline, mesh, transform]() mutable
			{
				NR_SCOPE_PERF("VulkanRenderer::RenderMesh");

				Ref<VKVertexBuffer> vulkanMeshVB = mesh->GetVertexBuffer().As<VKVertexBuffer>();

				VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
				VkDeviceSize offsets[1] = { 0 };
				vkCmdBindVertexBuffers(sData->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

				auto vulkanMeshIB = Ref<VKIndexBuffer>(mesh->GetIndexBuffer());
				VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
				vkCmdBindIndexBuffer(sData->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

				Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
				VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
				vkCmdBindPipeline(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				auto& submeshes = mesh->GetSubmeshes();
				for (Submesh& submesh : submeshes)
				{
					auto material = mesh->GetMaterials()[submesh.MaterialIndex].As<VKMaterial>();
					material->RT_UpdateForRendering();

					VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
					VkDescriptorSet descriptorSet = material->GetDescriptorSet();

					auto descriptorSetCompute = Renderer::GetShaderLibrary()->Get("ParticleGen").As<VKShader>()->GetDescriptorSet();

					// Bind descriptor sets describing shader binding points
					std::array<VkDescriptorSet, 3> descriptorSets = {
						descriptorSet,
						sData->RendererDescriptorSet.DescriptorSets[0]
					};
					//dewd;
					vkCmdBindDescriptorSets(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

					glm::mat4 worldTransform = transform * submesh.Transform;

					Buffer uniformStorageBuffer = material->GetUniformStorageBuffer();
					vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
					vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
					vkCmdDrawIndexed(sData->ActiveCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
				}
			});
	}

	void VKRenderer::RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms)
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

		Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
		vulkanMaterial->UpdateForRendering();

		Buffer pushConstantBuffer;
		pushConstantBuffer.Allocate(sizeof(glm::mat4) + additionalUniforms.Size);
		if (additionalUniforms.Size)
		{
			pushConstantBuffer.Write(additionalUniforms.Data, additionalUniforms.Size, sizeof(glm::mat4));
		}

		Renderer::Submit([pipeline, mesh, vulkanMaterial, transform, pushConstantBuffer]() mutable
			{
				Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
				VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

				vkCmdBindPipeline(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				// Bind descriptor sets describing shader binding points
				VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet();
				if (descriptorSet)
				{
					vkCmdBindDescriptorSets(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
				}

				auto& submeshes = mesh->GetSubmeshes();
				for (Submesh& submesh : submeshes)
				{
					glm::mat4 worldTransform = transform * submesh.Transform;
					pushConstantBuffer.Write(&worldTransform, sizeof(glm::mat4));
					vkCmdPushConstants(sData->ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, pushConstantBuffer.Size, pushConstantBuffer.Data);
					vkCmdDrawIndexed(sData->ActiveCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
				}

				pushConstantBuffer.Release();
			});
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
				VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet();
				if (descriptorSet)
				{
					vkCmdBindDescriptorSets(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
				}

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
				VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet();
				if (descriptorSet)
				{
					vkCmdBindDescriptorSets(sData->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
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

				vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

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

				vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

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

		Renderer::Submit([particleGenPipeline]() mutable
			{
				VKAllocator allocator("Particles");

				size_t dataCount = 200;  // Example number of elements
				VkDeviceSize bufferSize = sizeof(Particles) * dataCount;

				// Create staging buffer
				VkBufferCreateInfo bufferCreateInfo{};
				bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferCreateInfo.size = bufferSize;
				bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;  // Staging buffer usage
				bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VkBuffer stagingBuffer;
				VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);

				Particles* srcData = new Particles[dataCount];
				uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
				memcpy(destData, srcData, bufferSize);
				allocator.UnmapMemory(stagingBufferAllocation);

				// Copy data to staging buffer
				bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				VkBuffer dataBuffer;
				VmaAllocation dataBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, dataBuffer);

				// Copy data from staging buffer to GPU buffer (dataBuffer)
				Utils::CopyBuffer(stagingBuffer, dataBuffer, bufferSize);

				// Destroy staging buffer after copying
				allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);

				// Create descriptor set and bind buffer to the compute shader
				sData->ParticleDescriptorSet = particleGenPipeline->GetShader()->CreateDescriptorSets();

				VkDescriptorBufferInfo bufferInfo = {};
				bufferInfo.buffer = dataBuffer;
				bufferInfo.offset = 0;
				bufferInfo.range = bufferSize;

				// Write the descriptor set to bind the storage buffer
				VkWriteDescriptorSet descriptorWrite = {};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = sData->ParticleDescriptorSet.DescriptorSets[0];
				descriptorWrite.dstBinding = 0;  // Binding index 0 (match the shader binding)
				descriptorWrite.dstArrayElement = 0;
				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrite.descriptorCount = 1;
				descriptorWrite.pBufferInfo = &bufferInfo;

				vkUpdateDescriptorSets(VKContext::GetCurrentDevice()->GetVulkanDevice(), 1, &descriptorWrite, 0, nullptr);

				particleGenPipeline->Begin();
				particleGenPipeline->Dispatch(sData->ParticleDescriptorSet.DescriptorSets[0], dataCount / 32, 1, 1);
				particleGenPipeline->End();

				//--------------------------------------------------
				//--------------Test-----------------------------
				//--------------------------------------------------
				VkBufferCreateInfo readbackBufferInfo{};
				readbackBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				readbackBufferInfo.size = bufferSize;
				readbackBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				readbackBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VkBuffer readbackBuffer;
				VmaAllocation readbackBufferAllocation = allocator.AllocateBuffer(readbackBufferInfo, VMA_MEMORY_USAGE_CPU_ONLY, readbackBuffer);

				Utils::CopyBuffer(dataBuffer, readbackBuffer, bufferSize);

				uint8_t* readbackData = allocator.MapMemory<uint8_t>(readbackBufferAllocation);
				Particles* particleData = reinterpret_cast<Particles*>(readbackData);

				// You can now inspect the `particleData` array to check if the memory was changed
				for (size_t i = 0; i < dataCount; i++) 
				{
					// Debugging or validation code here
					int j = particleData[i].id;
				}

				allocator.UnmapMemory(readbackBufferAllocation);
			});
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

				vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

				preethamSkyComputePipeline->Begin();
				preethamSkyComputePipeline->SetPushConstants(&params, sizeof(glm::vec3));
				preethamSkyComputePipeline->Dispatch(descriptorSet.DescriptorSets[0], cubemapSize / 32, cubemapSize / 32, 6);
				preethamSkyComputePipeline->End();

				envUnfilteredCubemap->GenerateMips(true);
			});

		return environmentMap;
	}
}