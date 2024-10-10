#pragma once

#include "NotRed/Renderer/RendererAPI.h"

#include "Vulkan/vulkan.h"

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

		void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, const Ref<RenderPass>& renderPass) override;
		void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer) override;
		void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material) override;

		void DispatchComputeShader(const glm::ivec3& workGroups, Ref<Material> material) override;

		void SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow) override;
		std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath) override;
		void GenerateParticles() override;
		Ref<TextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) override;

		void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, const glm::mat4& transform) override;
		void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer()) override;
		void RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, const glm::mat4& transform) override;
		void RenderParticles(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform) override;

		static VkDescriptorSet RT_AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo);
	};
}