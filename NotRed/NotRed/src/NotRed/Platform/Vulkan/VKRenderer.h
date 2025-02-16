#pragma once

#include "NotRed/Platform/Vulkan/VKComputePipeline.h"
#include "NotRed/Renderer/RendererAPI.h"

#include "Vulkan/vulkan.h"

#include "VKMaterial.h"
#include "VKUniformBuffer.h"
#include "VKStorageBuffer.h"

namespace NR
{
	class VKRenderer : public RendererAPI
	{
	public:
		void Init() override;
		void Shutdown() override;

		RendererCapabilities& GetCapabilities() override;

		void BeginFrame() override;
		void EndFrame() override;

		void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, const Ref<RenderPass>& renderPass, bool explicitClear = false) override;
		void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer) override;

		void SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow, Ref<Image2D> linearDepth) override;
		std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath) override;
		Ref<TextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) override;
		
		void GenerateParticles(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups) override;

		void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform) override;
		void RenderParticles(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform) override;
		void RenderSubmesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, uint32_t index, Ref<MaterialTable> materialTable, const glm::mat4& transform) override;
		void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer()) override;
		void RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::mat4& transform) override; 
		void LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups) override;
		void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material) override;
		void SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides) override;
		
		void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount = 0) override;
		
		void DispatchComputeShader(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups) override;
		void ClearImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image) override;
		
		static VkDescriptorSet RT_AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo);
		static const std::vector<std::vector<VkWriteDescriptorSet>>& RT_RetrieveOrCreateStorageBufferWriteDescriptors(Ref<StorageBufferSet> storageBufferSet, Ref<VKMaterial> vulkanMaterial);
		static const std::vector<std::vector<VkWriteDescriptorSet>>& RT_RetrieveOrCreateUniformBufferWriteDescriptors(Ref<UniformBufferSet> uniformBufferSet, Ref<VKMaterial> vulkanMaterial);

		static void RT_UpdateMaterialForRendering(Ref<VKMaterial> material, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet);

		static VkSampler GetClampSampler();
		static int32_t& GetSelectedDrawCall();

		static uint32_t GetDescriptorAllocationCount(uint32_t frameIndex = 0);
	};

	namespace Utils 
	{
		void InsertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange);

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	}
}