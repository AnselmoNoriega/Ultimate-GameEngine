#include "nrpch.h"
#include "VKRenderer.h"

#include "imgui.h"

#include "Vulkan.h"
#include "VKContext.h"

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/SceneRenderer.h"

#include "VKPipeline.h"
#include "VKVertexBuffer.h"
#include "VKIndexBuffer.h"
#include "VKFrameBuffer.h"
#include "VKRenderCommandBuffer.h"

#include "VKShader.h"
#include "VKTexture.h"

#include "backends/imgui_impl_glfw.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

#include "NotRed/Core/Timer.h"
#include "NotRed/Debug/Profiler.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace NR
{
	struct VKRendererData
	{
		RendererCapabilities RenderCaps;

		Ref<Texture2D> BRDFLut;

		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;
		VKShader::ShaderMaterialDescriptorSet QuadDescriptorSet;

		std::unordered_map<SceneRenderer*, std::vector<VKShader::ShaderMaterialDescriptorSet>> RendererDescriptorSet;
		VkDescriptorSet ActiveRendererDescriptorSet = nullptr;
		std::vector<VkDescriptorPool> DescriptorPools;
		std::vector<uint32_t> DescriptorPoolAllocationCount;

		// UniformBufferSet -> Shader Hash -> Frame -> WriteDescriptor
		std::unordered_map<UniformBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> UniformBufferWriteDescriptorCache;
		std::unordered_map<StorageBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> StorageBufferWriteDescriptorCache;

		// Default samplers
		VkSampler SamplerClamp = nullptr;

		int32_t SelectedDrawCall = -1;
		int32_t DrawCallCount = 0;
	};

	static VKRendererData* sData = nullptr;

	namespace Utils 
	{
		static const char* VKVendorIDToString(uint32_t vendorID)
		{
			switch (vendorID)
			{
			case 0x10DE: return "NVIDIA";
			case 0x1002: return "AMD";
			case 0x8086: return "INTEL";
			case 0x13B5: return "ARM";
			}
			return "Unknown";
		}

	}

	void VKRenderer::Init()
	{
		sData = new VKRendererData();
		const auto& config = Renderer::GetConfig();
		sData->DescriptorPools.resize(config.FramesInFlight);
		sData->DescriptorPoolAllocationCount.resize(config.FramesInFlight);

		auto& caps = sData->RenderCaps;
		auto& properties = VKContext::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
		caps.Vendor = Utils::VKVendorIDToString(properties.vendorID);
		caps.Device = properties.deviceName;
		caps.Version = std::to_string(properties.driverVersion);

		Utils::DumpGPUInfo();

		// Create descriptor pools
		Renderer::Submit([]() mutable
			{
				// Create Descriptor Pool
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
				pool_info.maxSets = 100000;
				pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
				pool_info.pPoolSizes = pool_sizes;
				VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
				uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
				for (uint32_t i = 0; i < framesInFlight; i++)
				{
					VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &sData->DescriptorPools[i]));
					sData->DescriptorPoolAllocationCount[i] = 0;
				}
			});

		// Create fullscreen quad
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
					for (uint32_t frame = 0; frame < framesInFlight; frame++)
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
					for (uint32_t frame = 0; frame < framesInFlight; frame++)
					{
						Ref<VKStorageBuffer> storageBuffer = storageBufferSet->Get(binding, 0, frame); // set = 0 for now

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

	void VKRenderer::RT_UpdateMaterialForRendering(Ref<VKMaterial> material, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet)
	{
		NR_PROFILE_FUNC();

		if (uniformBufferSet)
		{
			auto writeDescriptors = RT_RetrieveOrCreateUniformBufferWriteDescriptors(uniformBufferSet, material);
			if (storageBufferSet)
			{
				const auto& storageBufferWriteDescriptors = RT_RetrieveOrCreateStorageBufferWriteDescriptors(storageBufferSet, material);

				const uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
				for (uint32_t frame = 0; frame < framesInFlight; frame++)
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

	VkSampler VKRenderer::GetClampSampler()
	{
		if (sData->SamplerClamp)
			return sData->SamplerClamp;

		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 100.0f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &sData->SamplerClamp));
		return sData->SamplerClamp;
	}

	int32_t& VKRenderer::GetSelectedDrawCall()
	{
		return sData->SelectedDrawCall;
	}

	void VKRenderer::RenderStaticMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<StaticMesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount)
	{
		NR_CORE_VERIFY(mesh);
		NR_CORE_VERIFY(materialTable);

		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, materialTable, transformBuffer, transformOffset, instanceCount]() mutable
			{
				NR_PROFILE_FUNC("VKRenderer::RenderMesh");
				NR_SCOPE_PERF("VKRenderer::RenderMesh");

				if (sData->SelectedDrawCall != -1 && sData->DrawCallCount > sData->SelectedDrawCall)
					return;

				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

				Ref<MeshSource> meshSource = mesh->GetMeshSource();
				Ref<VKVertexBuffer> vulkanMeshVB = meshSource->GetVertexBuffer().As<VKVertexBuffer>();
				VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
				VkDeviceSize offsets[1] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

				Ref<VKVertexBuffer> vulkanTransformBuffer = transformBuffer.As<VKVertexBuffer>();
				VkBuffer vbTransformBuffer = vulkanTransformBuffer->GetVulkanBuffer();
				VkDeviceSize instanceOffsets[1] = { transformOffset };
				vkCmdBindVertexBuffers(commandBuffer, 1, 1, &vbTransformBuffer, instanceOffsets);

				auto vulkanMeshIB = Ref<VKIndexBuffer>(meshSource->GetIndexBuffer());
				VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
				vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

				Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
				VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				std::vector<std::vector<VkWriteDescriptorSet>> writeDescriptors;

				const auto& submeshes = meshSource->GetSubmeshes();
				const Submesh& submesh = submeshes[submeshIndex];
				const auto& meshMaterialTable = mesh->GetMaterials();
				uint32_t materialCount = meshMaterialTable->GetMaterialCount();
				Ref<MaterialAsset> material = materialTable->HasMaterial(submesh.MaterialIndex) ? materialTable->GetMaterial(submesh.MaterialIndex) : meshMaterialTable->GetMaterial(submesh.MaterialIndex);
				Ref<VKMaterial> vulkanMaterial = material->GetMaterial().As<VKMaterial>();
				RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

				if (sData->SelectedDrawCall != -1 && sData->DrawCallCount > sData->SelectedDrawCall)
					return;

				// NOTE: Descriptor Set 1 is owned by the renderer
				std::array<VkDescriptorSet, 2> descriptorSets = {
					vulkanMaterial->GetDescriptorSet(frameIndex),
					sData->ActiveRendererDescriptorSet
				};

				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

				Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, uniformStorageBuffer.Size, uniformStorageBuffer.Data);
				vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, instanceCount, submesh.BaseIndex, submesh.BaseVertex, 0);
				sData->DrawCallCount++;
			});
	}

	void VKRenderer::RenderSubmeshInstanced(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, const std::vector<Ref<StorageBuffer>>& boneTransformStorageBuffers, uint32_t boneTransformsOffset, uint32_t instanceCount)
	{
		NR_CORE_VERIFY(mesh);
		NR_CORE_VERIFY(materialTable);

		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, materialTable, transformBuffer, transformOffset, boneTransformStorageBuffers, boneTransformsOffset, instanceCount]() mutable
			{
				NR_PROFILE_FUNC("VKRenderer::RenderMesh");
				NR_SCOPE_PERF("VKRenderer::RenderMesh");

				if (sData->SelectedDrawCall != -1 && sData->DrawCallCount > sData->SelectedDrawCall)
				{
					return;
				}

				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

				Ref<MeshSource> meshSource = mesh->GetMeshSource();
				Ref<VKVertexBuffer> vulkanMeshVB = meshSource->GetVertexBuffer().As<VKVertexBuffer>();
				VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
				VkDeviceSize vertexOffsets[1] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, vertexOffsets);

				Ref<VKVertexBuffer> vulkanTransformBuffer = transformBuffer.As<VKVertexBuffer>();
				VkBuffer vbTransformBuffer = vulkanTransformBuffer->GetVulkanBuffer();
				VkDeviceSize instanceOffsets[1] = { transformOffset };
				vkCmdBindVertexBuffers(commandBuffer, 1, 1, &vbTransformBuffer, instanceOffsets);

				auto vulkanMeshIB = Ref<VKIndexBuffer>(meshSource->GetIndexBuffer());
				VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
				vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

				Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
				VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				VkDescriptorSet animationDataDS = VK_NULL_HANDLE;
				if (mesh->IsRigged())
				{
					VkBuffer boneInfluenceVB = meshSource->GetBoneInfluenceBuffer().As<VKVertexBuffer>()->GetVulkanBuffer();
					vkCmdBindVertexBuffers(commandBuffer, 2, 1, &boneInfluenceVB, vertexOffsets);

					auto temp = vulkanPipeline->GetSpecification().Shader.As<VKShader>()->AllocateDescriptorSet(2); //  hard coding 2 = animation data.  Yuk.

					VkWriteDescriptorSet writeDescriptorSet;
					writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeDescriptorSet.pNext = nullptr;
					writeDescriptorSet.dstSet = temp.DescriptorSets[0];
					writeDescriptorSet.dstBinding = boneTransformStorageBuffers[frameIndex]->GetBinding();
					writeDescriptorSet.dstArrayElement = 0;
					writeDescriptorSet.descriptorCount = 1;
					writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					writeDescriptorSet.pImageInfo = nullptr;
					writeDescriptorSet.pBufferInfo = &boneTransformStorageBuffers[frameIndex].As<VKUniformBuffer>()->GetDescriptorBufferInfo();
					writeDescriptorSet.pTexelBufferView = nullptr;

					vkUpdateDescriptorSets(VKContext::GetCurrentDevice()->GetVulkanDevice(), 1, &writeDescriptorSet, 0, nullptr);
					animationDataDS = temp.DescriptorSets[0];
				}

				const auto& submeshes = meshSource->GetSubmeshes();
				const auto& submesh = submeshes[submeshIndex];
				const auto& meshMaterialTable = mesh->GetMaterials();
				uint32_t materialCount = meshMaterialTable->GetMaterialCount();
				Ref<MaterialAsset> material = materialTable->HasMaterial(submesh.MaterialIndex) ? materialTable->GetMaterial(submesh.MaterialIndex) : meshMaterialTable->GetMaterial(submesh.MaterialIndex);
				Ref<VKMaterial> vulkanMaterial = material->GetMaterial().As<VKMaterial>();
				RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

				if (sData->SelectedDrawCall != -1 && sData->DrawCallCount > sData->SelectedDrawCall)
					return;

				VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);

				// NOTE: Descriptor Set 1 is owned by the renderer
				std::vector<VkDescriptorSet> descriptorSets = {
					descriptorSet,
					sData->ActiveRendererDescriptorSet
				};
				if (animationDataDS)
					descriptorSets.emplace_back(animationDataDS);

				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

				Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
				uint32_t pushConstantOffset = 0;

				if (mesh->IsRigged())
				{
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, pushConstantOffset, sizeof(uint32_t), &boneTransformsOffset);
					pushConstantOffset += 16;
				}

				if (uniformStorageBuffer)
				{
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantOffset, uniformStorageBuffer.Size, uniformStorageBuffer.Data);
				}

				vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, instanceCount, submesh.BaseIndex, submesh.BaseVertex, 0);
				sData->DrawCallCount++;
			});
	}

	void VKRenderer::RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, const std::vector<Ref<StorageBuffer>>& boneTransformStorageBuffers, uint32_t boneTransformsOffset, uint32_t instanceCount, Buffer additionalUniforms)
	{
		NR_CORE_ASSERT(mesh);
		NR_CORE_ASSERT(mesh->GetMeshSource());

		Buffer pushConstantBuffer;
		if (additionalUniforms.Size || mesh->IsRigged())
		{
			pushConstantBuffer.Allocate(additionalUniforms.Size + (mesh->IsRigged() ? sizeof(uint32_t) : 0));
			if (additionalUniforms.Size)
			{
				pushConstantBuffer.Write(additionalUniforms.Data, additionalUniforms.Size);
			}			
			
			if (mesh->IsRigged())
			{
				pushConstantBuffer.Write(&boneTransformsOffset, sizeof(uint32_t), additionalUniforms.Size);
			}
		}

		Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, vulkanMaterial, transformBuffer, transformOffset, boneTransformStorageBuffers, instanceCount, pushConstantBuffer]() mutable
			{
				NR_PROFILE_FUNC("VKRenderer::RenderMeshWithMaterial");
				NR_SCOPE_PERF("VKRenderer::RenderMeshWithMaterial");

				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

				Ref<MeshSource> meshSource = mesh->GetMeshSource();
				VkBuffer meshVB = meshSource->GetVertexBuffer().As<VKVertexBuffer>()->GetVulkanBuffer();
				VkDeviceSize vertexOffsets[1] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshVB, vertexOffsets);

				VkBuffer transformVB = transformBuffer.As<VKVertexBuffer>()->GetVulkanBuffer();
				VkDeviceSize instanceOffsets[1] = { transformOffset };
				vkCmdBindVertexBuffers(commandBuffer, 1, 1, &transformVB, instanceOffsets);


				VkBuffer meshIB = meshSource->GetIndexBuffer().As<VKIndexBuffer>()->GetVulkanBuffer();
				vkCmdBindIndexBuffer(commandBuffer, meshIB, 0, VK_INDEX_TYPE_UINT32);

				RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

				Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
				VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				VkDescriptorSet animationDataDS = VK_NULL_HANDLE;
				if (mesh->IsRigged())
				{
					Ref<VKVertexBuffer> vulkanBoneInfluencesVB = meshSource->GetBoneInfluenceBuffer().As<VKVertexBuffer>();
					VkBuffer vbBoneInfluencesBuffer = vulkanBoneInfluencesVB->GetVulkanBuffer();
					vkCmdBindVertexBuffers(commandBuffer, 2, 1, &vbBoneInfluencesBuffer, vertexOffsets);

					auto temp = vulkanPipeline->GetSpecification().Shader.As<VKShader>()->AllocateDescriptorSet(1); //  hard coding 1 = animation data.  Yuk.

					VkWriteDescriptorSet writeDescriptorSet;
					writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeDescriptorSet.pNext = nullptr;
					writeDescriptorSet.dstSet = temp.DescriptorSets[0];
					writeDescriptorSet.dstBinding = boneTransformStorageBuffers[frameIndex]->GetBinding();
					writeDescriptorSet.dstArrayElement = 0;
					writeDescriptorSet.descriptorCount = 1;
					writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					writeDescriptorSet.pImageInfo = nullptr;
					writeDescriptorSet.pBufferInfo = &boneTransformStorageBuffers[frameIndex].As<VKUniformBuffer>()->GetDescriptorBufferInfo();
					writeDescriptorSet.pTexelBufferView = nullptr;

					vkUpdateDescriptorSets(VKContext::GetCurrentDevice()->GetVulkanDevice(), 1, &writeDescriptorSet, 0, nullptr);
					animationDataDS = temp.DescriptorSets[0];
				}

				float lineWidth = vulkanPipeline->GetSpecification().LineWidth;
				if (lineWidth != 1.0f)
				{
					vkCmdSetLineWidth(commandBuffer, lineWidth);
				}

				// Bind descriptor sets describing shader binding points
				std::vector<VkDescriptorSet> descriptorSets;
				VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);
				if (descriptorSet)
				{
					descriptorSets.emplace_back(descriptorSet);
				}
				if (animationDataDS)
				{
					descriptorSets.emplace_back(animationDataDS);
				}

				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

				Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
				uint32_t pushConstantOffset = 0;
				if (pushConstantBuffer.Size)
				{
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, pushConstantOffset, pushConstantBuffer.Size, pushConstantBuffer.Data);
					pushConstantOffset += 16;
				}

				if (uniformStorageBuffer)
				{
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantOffset, uniformStorageBuffer.Size, uniformStorageBuffer.Data);
				}

				const auto& submeshes = meshSource->GetSubmeshes();
				const auto& submesh = submeshes[submeshIndex];

				vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, instanceCount, submesh.BaseIndex, submesh.BaseVertex, 0);

				pushConstantBuffer.Release();
			});
	}

	void VKRenderer::RenderStaticMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<StaticMesh> staticMesh, uint32_t submeshIndex, Ref<Material> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Buffer additionalUniforms /*= Buffer()*/)
	{
		NR_CORE_ASSERT(staticMesh);
		NR_CORE_ASSERT(staticMesh->GetMeshSource());

		Buffer pushConstantBuffer;
		if (additionalUniforms.Size)
		{
			pushConstantBuffer.Allocate(additionalUniforms.Size);
			if (additionalUniforms.Size)
				pushConstantBuffer.Write(additionalUniforms.Data, additionalUniforms.Size);
		}

		Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, staticMesh, submeshIndex, vulkanMaterial, transformBuffer, transformOffset, instanceCount, pushConstantBuffer]() mutable
			{
				NR_PROFILE_FUNC("VKRenderer::RenderMeshWithMaterial");
				NR_SCOPE_PERF("VKRenderer::RenderMeshWithMaterial");

				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

				Ref<MeshSource> meshSource = staticMesh->GetMeshSource();
				auto vulkanMeshVB = meshSource->GetVertexBuffer().As<VKVertexBuffer>();
				VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
				VkDeviceSize vertexOffsets[1] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, vertexOffsets);

				Ref<VKVertexBuffer> vulkanTransformBuffer = transformBuffer.As<VKVertexBuffer>();
				VkBuffer vbTransformBuffer = vulkanTransformBuffer->GetVulkanBuffer();
				VkDeviceSize instanceOffsets[1] = { transformOffset };
				vkCmdBindVertexBuffers(commandBuffer, 1, 1, &vbTransformBuffer, instanceOffsets);

				auto vulkanMeshIB = Ref<VKIndexBuffer>(meshSource->GetIndexBuffer());
				VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
				vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

				RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

				Ref<VKPipeline> vulkanPipeline = pipeline.As<VKPipeline>();
				VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				float lineWidth = vulkanPipeline->GetSpecification().LineWidth;
				if (lineWidth != 1.0f)
					vkCmdSetLineWidth(commandBuffer, lineWidth);

				// Bind descriptor sets describing shader binding points
				VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);
				if (descriptorSet)
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

				Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
				uint32_t pushConstantOffset = 0;
				if (pushConstantBuffer.Size)
				{
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, pushConstantOffset, pushConstantBuffer.Size, pushConstantBuffer.Data);
					pushConstantOffset += 16;
				}

				if (uniformStorageBuffer)
				{
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantOffset, uniformStorageBuffer.Size, uniformStorageBuffer.Data);
					pushConstantOffset += uniformStorageBuffer.Size;
				}

				const auto& submeshes = meshSource->GetSubmeshes();
				const auto& submesh = submeshes[submeshIndex];

				vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, instanceCount, submesh.BaseIndex, submesh.BaseVertex, 0);

				pushConstantBuffer.Release();
			});
	}

	void VKRenderer::GenerateParticles(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups)
	{
		//TODO FIX
		auto vulkanMaterial = material.As<VKMaterial>();
		auto vulkanpipeline = pipeline.As<VKComputePipeline>();
		Renderer::Submit([renderCommandBuffer, vulkanpipeline, vulkanMaterial, uniformBufferSet, storageBufferSet, workGroups]() mutable
			{
				const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

				const VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);

				vulkanpipeline->Begin(renderCommandBuffer);
				//pipeline->SetPushConstants(glm::value_ptr(screenSize), sizeof(glm::ivec2));
				vulkanpipeline->Dispatch(descriptorSet, workGroups.x, workGroups.y, workGroups.z);
				vulkanpipeline->End();
			});
	}

	void VKRenderer::RenderParticles(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Particles> particles, Ref<Material> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset)
	{
		NR_CORE_ASSERT(particles);

		Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, vulkanMaterial, particles, transformBuffer, transformOffset]() mutable
			{
				NR_PROFILE_FUNC("VKRenderer::RenderParticles");
				NR_SCOPE_PERF("VKRenderer::RenderMeshParticles");

				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

				auto vulkanMeshVB = particles->GetVertexBuffer().As<VKVertexBuffer>();
				VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
				VkDeviceSize vertexOffsets[1] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, vertexOffsets);

				Ref<VKVertexBuffer> vulkanTransformBuffer = transformBuffer.As<VKVertexBuffer>();
				VkBuffer vbTransformBuffer = vulkanTransformBuffer->GetVulkanBuffer();
				VkDeviceSize instanceOffsets[1] = { transformOffset };
				vkCmdBindVertexBuffers(commandBuffer, 1, 1, &vbTransformBuffer, instanceOffsets);

				auto vulkanMeshIB = Ref<VKIndexBuffer>(particles->GetIndexBuffer());
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
				uint32_t pushConstantOffset = 0;

				if (uniformStorageBuffer)
				{
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantOffset, uniformStorageBuffer.Size, uniformStorageBuffer.Data);
					pushConstantOffset += uniformStorageBuffer.Size;
				}

				vkCmdDrawIndexed(commandBuffer, particles->GetIndexCount(), 1, 0, 0, 0);
			});
	}

	void VKRenderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::mat4& transform)
	{
		Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, vulkanMaterial, transform]() mutable
			{
				NR_PROFILE_FUNC("VKRenderer::RenderQuad");

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

	void VKRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount /*= 0*/)
	{
		Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
		if (indexCount == 0)
			indexCount = indexBuffer->GetCount();

		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, vulkanMaterial, vertexBuffer, indexBuffer, transform, indexCount]() mutable
			{
				NR_PROFILE_FUNC("VKRenderer::RenderGeometry");

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

				const auto& writeDescriptors = RT_RetrieveOrCreateUniformBufferWriteDescriptors(uniformBufferSet, vulkanMaterial);
				vulkanMaterial->RT_UpdateForRendering(writeDescriptors);

				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
				if (descriptorSet)
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
				Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
				if (uniformStorageBuffer)
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);

				vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
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

#if 0
	void VKRenderer::SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t set)
	{
		Renderer::Submit([uniformBuffer, set]()
			{


				NR_CORE_ASSERT(set == 1); // Currently we only bind to Renderer-maintaned UBs, which are in descriptor set 1

				Ref<VKUniformBuffer> vulkanUniformBuffer = uniformBuffer.As<VKUniformBuffer>();

				VkWriteDescriptorSet writeDescriptorSet = {};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSet.pBufferInfo = &vulkanUniformBuffer->GetDescriptorBufferInfo();
				writeDescriptorSet.dstBinding = uniformBuffer->GetBinding();
				writeDescriptorSet.dstSet = sData->RendererDescriptorSet.DescriptorSets[0];

				NR_CORE_WARN("VKRenderer - Updating descriptor set (VKRenderer::SetUniformBuffer)");
				VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
				vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
			});
	}
#endif

	void VKRenderer::ClearImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image)
	{
		Renderer::Submit([commandBuffer, image = image.As<VKImage2D>()]
			{
				const auto vulkanCommandBuffer = commandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(Renderer::GetCurrentFrameIndex());
				VkImageSubresourceRange subresourceRange{};
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.levelCount = image->GetSpecification().Mips;
				subresourceRange.layerCount = image->GetSpecification().Layers;

				VkClearColorValue clearColor{ 0.f, 0.f, 0.f, 0.f };
				vkCmdClearColorImage(vulkanCommandBuffer, image->GetImageInfo().Image, image->GetSpecification().Usage == ImageUsage::Storage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &clearColor, 1, &subresourceRange);
			});
	}

	void VKRenderer::DispatchComputeShader(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups)
	{
		auto vulkanMaterial = material.As<VKMaterial>();
		auto vulkanPipeline = pipeline.As<VKComputePipeline>();
		Renderer::Submit([renderCommandBuffer, vulkanPipeline, vulkanMaterial, uniformBufferSet, storageBufferSet, workGroups]() mutable
			{
				const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

				RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

				const VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);

				vulkanPipeline->Begin(renderCommandBuffer);
				vulkanPipeline->Dispatch(descriptorSet, workGroups.x, workGroups.y, workGroups.z);
				vulkanPipeline->End();
			});
	}

	void VKRenderer::LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> pipelineCompute, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups)
	{
		auto vulkanMaterial = material.As<VKMaterial>();
		auto pipeline = pipelineCompute.As<VKComputePipeline>();
		Renderer::Submit([renderCommandBuffer, pipeline, vulkanMaterial, uniformBufferSet, storageBufferSet, screenSize, workGroups]() mutable
			{
				const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

				RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

				const VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);

				pipeline->Begin(renderCommandBuffer);
				pipeline->SetPushConstants(glm::value_ptr(screenSize), sizeof(glm::ivec2));
				pipeline->Dispatch(descriptorSet, workGroups.x, workGroups.y, workGroups.z);

				VkMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex),
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					0,
					1, &barrier,
					0, nullptr,
					0, nullptr);

				pipeline->End();
			});
	}

	void VKRenderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material)
	{
		SubmitFullscreenQuad(renderCommandBuffer, pipeline, uniformBufferSet, nullptr, material);
	}

	void VKRenderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet,
		Ref<StorageBufferSet> storageBufferSet, Ref<Material> material)
	{
		Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, vulkanMaterial]() mutable
			{
				NR_PROFILE_FUNC("VKRenderer::SubmitFullscreenQuad");

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
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

				Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
				if (uniformStorageBuffer.Size)
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, uniformStorageBuffer.Size, uniformStorageBuffer.Data);

				vkCmdDrawIndexed(commandBuffer, sData->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
			});
	}

	void VKRenderer::SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides)
	{
		Buffer vertexPushConstantBuffer;
		if (vertexShaderOverrides)
		{
			vertexPushConstantBuffer.Allocate(vertexShaderOverrides.Size);
			vertexPushConstantBuffer.Write(vertexShaderOverrides.Data, vertexShaderOverrides.Size);
		}

		Buffer fragmentPushConstantBuffer;
		if (fragmentPushConstantBuffer)
		{
			fragmentPushConstantBuffer.Allocate(fragmentShaderOverrides.Size);
			fragmentPushConstantBuffer.Write(fragmentShaderOverrides.Data, fragmentShaderOverrides.Size);
		}

		Ref<VKMaterial> vulkanMaterial = material.As<VKMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, vulkanMaterial, vertexPushConstantBuffer, fragmentPushConstantBuffer]() mutable
			{
				NR_PROFILE_FUNC("VKRenderer::SubmitFullscreenQuad");

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

				if (uniformBufferSet)
				{
					const auto& writeDescriptors = RT_RetrieveOrCreateUniformBufferWriteDescriptors(uniformBufferSet, vulkanMaterial);
					vulkanMaterial->RT_UpdateForRendering(writeDescriptors);
				}
				else
				{
					vulkanMaterial->RT_UpdateForRendering();
				}

				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
				if (descriptorSet)
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

				if (vertexPushConstantBuffer)
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, vertexPushConstantBuffer.Size, vertexPushConstantBuffer.Data);
				if (fragmentPushConstantBuffer)
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, vertexPushConstantBuffer.Size, fragmentPushConstantBuffer.Size, fragmentPushConstantBuffer.Data);

				vkCmdDrawIndexed(commandBuffer, sData->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);

				vertexPushConstantBuffer.Release();
				fragmentPushConstantBuffer.Release();
			});
	}

	void VKRenderer::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow, Ref<Image2D> linearDepth)
	{
		if (!environment)
			environment = Renderer::GetEmptyEnvironment();

		Renderer::Submit([sceneRenderer, environment, shadow, linearDepth]() mutable
			{
				NR_PROFILE_FUNC("VKRenderer::SetSceneEnvironment");

				const auto shader = Renderer::GetShaderLibrary()->Get("PBR_Static");
				Ref<VKShader> pbrShader = shader.As<VKShader>();
				const uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();

				if (sData->RendererDescriptorSet.find(sceneRenderer.Raw()) == sData->RendererDescriptorSet.end())
				{
					const uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
					sData->RendererDescriptorSet[sceneRenderer.Raw()].resize(framesInFlight);
					for (uint32_t i = 0; i < framesInFlight; i++)
						sData->RendererDescriptorSet.at(sceneRenderer.Raw())[i] = pbrShader->CreateDescriptorSets(1);

				}

				VkDescriptorSet descriptorSet = sData->RendererDescriptorSet.at(sceneRenderer.Raw())[bufferIndex].DescriptorSets[0];
				sData->ActiveRendererDescriptorSet = descriptorSet;

				std::array<VkWriteDescriptorSet, 5> writeDescriptors;

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

				writeDescriptors[4] = *pbrShader->GetDescriptorSet("uLinearDepthTex", 1);
				writeDescriptors[4].dstSet = descriptorSet;
				const auto& linearDepthImageInfo = linearDepth.As<VKImage2D>()->GetDescriptor();
				writeDescriptors[4].pImageInfo = &linearDepthImageInfo;

				const auto vulkanDevice = VKContext::GetCurrentDevice()->GetVulkanDevice();
				vkUpdateDescriptorSets(vulkanDevice, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
			});
	}

	void VKRenderer::BeginFrame()
	{
		Renderer::Submit([]()
			{
				NR_PROFILE_FUNC("VKRenderer::BeginFrame");

				VKSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();

				// Reset descriptor pools here
				VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();
				uint32_t bufferIndex = swapChain.GetCurrentBufferIndex();
				vkResetDescriptorPool(device, sData->DescriptorPools[bufferIndex], 0);
				memset(sData->DescriptorPoolAllocationCount.data(), 0, sData->DescriptorPoolAllocationCount.size() * sizeof(uint32_t));

				sData->DrawCallCount = 0;

#if 0
				VkCommandBufferBeginInfo cmdBufInfo = {};
				cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				cmdBufInfo.pNext = nullptr;

				VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
				commandBuffer = drawCommandBuffer;
				NR_CORE_ASSERT(commandBuffer);
				VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));
#endif
			});
	}

	void VKRenderer::EndFrame()
	{
#if 0
		Renderer::Submit([]()
			{
				VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
				commandBuffer = nullptr;
			});
#endif
	}

	void VKRenderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, const Ref<RenderPass>& renderPass, bool explicitClear)
	{
		Renderer::Submit([renderCommandBuffer, renderPass, explicitClear]()
			{
				NR_PROFILE_FUNC(fmt::format("VKRenderer::BeginRenderPass ({})", renderPass->GetSpecification().DebugName).c_str());

				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

				auto fb = renderPass->GetSpecification().TargetFrameBuffer;
				Ref<VKFrameBuffer> frameBuffer = fb.As<VKFrameBuffer>();
				const auto& fbSpec = frameBuffer->GetSpecification();

				uint32_t width = frameBuffer->GetWidth();
				uint32_t height = frameBuffer->GetHeight();

				VkViewport viewport = {};
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				VkRenderPassBeginInfo renderPassBeginInfo = {};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.pNext = nullptr;
				renderPassBeginInfo.renderPass = frameBuffer->GetRenderPass();
				renderPassBeginInfo.renderArea.offset.x = 0;
				renderPassBeginInfo.renderArea.offset.y = 0;
				renderPassBeginInfo.renderArea.extent.width = width;
				renderPassBeginInfo.renderArea.extent.height = height;
				if (frameBuffer->GetSpecification().SwapChainTarget)
				{
					VKSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();
					width = swapChain.GetWidth();
					height = swapChain.GetHeight();
					renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassBeginInfo.pNext = nullptr;
					renderPassBeginInfo.renderPass = frameBuffer->GetRenderPass();
					renderPassBeginInfo.renderArea.offset.x = 0;
					renderPassBeginInfo.renderArea.offset.y = 0;
					renderPassBeginInfo.renderArea.extent.width = width;
					renderPassBeginInfo.renderArea.extent.height = height;
					renderPassBeginInfo.framebuffer = swapChain.GetCurrentFrameBuffer();

					viewport.x = 0.0f;
					viewport.y = (float)height;
					viewport.width = (float)width;
					viewport.height = -(float)height;
				}
				else
				{
					width = frameBuffer->GetWidth();
					height = frameBuffer->GetHeight();
					renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassBeginInfo.pNext = nullptr;
					renderPassBeginInfo.renderPass = frameBuffer->GetRenderPass();
					renderPassBeginInfo.renderArea.offset.x = 0;
					renderPassBeginInfo.renderArea.offset.y = 0;
					renderPassBeginInfo.renderArea.extent.width = width;
					renderPassBeginInfo.renderArea.extent.height = height;
					renderPassBeginInfo.framebuffer = frameBuffer->GetVulkanFrameBuffer();

					viewport.x = 0.0f;
					viewport.y = 0.0f;
					viewport.width = (float)width;
					viewport.height = (float)height;
				}

				// TODO: Does our frameBuffer have a depth attachment?
				const auto& clearValues = frameBuffer->GetVulkanClearValues();
				renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
				renderPassBeginInfo.pClearValues = clearValues.data();

				vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				if (explicitClear)
				{
					const uint32_t colorAttachmentCount = (uint32_t)frameBuffer->GetColorAttachmentCount();
					const uint32_t totalAttachmentCount = colorAttachmentCount + (frameBuffer->HasDepthAttachment() ? 1 : 0);
					NR_CORE_ASSERT(clearValues.size() == totalAttachmentCount);

					std::vector<VkClearAttachment> attachments(totalAttachmentCount);
					std::vector<VkClearRect> clearRects(totalAttachmentCount);
					for (uint32_t i = 0; i < colorAttachmentCount; i++)
					{
						attachments[i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						attachments[i].colorAttachment = i;
						attachments[i].clearValue = clearValues[i];

						clearRects[i].rect.offset = { (int32_t)0, (int32_t)0 };
						clearRects[i].rect.extent = { width, height };
						clearRects[i].baseArrayLayer = 0;
						clearRects[i].layerCount = 1;
					}

					if (frameBuffer->HasDepthAttachment())
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

				// Update dynamic viewport state
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
				NR_PROFILE_FUNC("VKRenderer::EndRenderPass");

				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VKRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

				vkCmdEndRenderPass(commandBuffer);
			});
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> VKRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		if (!Renderer::GetConfig().ComputeEnvironmentMaps)
			return { Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture() };

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
				writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
				writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

				Ref<VKTexture2D> envEquirectVK = envEquirect.As<VKTexture2D>();
				writeDescriptors[1] = *shader->GetDescriptorSet("uEquirectangularTex");
				writeDescriptors[1].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
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
				for (uint32_t i = 0; i < mipCount; i++)
				{
					VkDescriptorImageInfo& mipImageInfo = mipImageInfos[i];
					mipImageInfo = imageInfo;
					mipImageInfo.imageView = envFilteredCubemap->CreateImageViewSingleMip(i);

					writeDescriptors[i * 2 + 0] = *shader->GetDescriptorSet("outputTexture");
					writeDescriptors[i * 2 + 0].dstSet = descriptorSet.DescriptorSets[i]; // Should this be set inside the shader?
					writeDescriptors[i * 2 + 0].pImageInfo = &mipImageInfo;

					Ref<VKTextureCube> envUnfilteredCubemap = envUnfiltered.As<VKTextureCube>();
					writeDescriptors[i * 2 + 1] = *shader->GetDescriptorSet("inputTexture");
					writeDescriptors[i * 2 + 1].dstSet = descriptorSet.DescriptorSets[i]; // Should this be set inside the shader?
					writeDescriptors[i * 2 + 1].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();
				}

				vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

				environmentMipFilterPipeline->Begin(); // begin compute pass
				const float deltaRoughness = 1.0f / glm::max((float)envFiltered->GetMipLevelCount() - 1.0f, 1.0f);
				for (uint32_t i = 0, size = cubemapSize; i < mipCount; i++, size /= 2)
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
				writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
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

	uint32_t VKRenderer::GetDescriptorAllocationCount(uint32_t frameIndex)
	{
		return sData->DescriptorPoolAllocationCount[frameIndex];
	}

	namespace Utils {

		void InsertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			imageMemoryBarrier.srcAccessMask = srcAccessMask;
			imageMemoryBarrier.dstAccessMask = dstAccessMask;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;
			SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
		}

	}

}