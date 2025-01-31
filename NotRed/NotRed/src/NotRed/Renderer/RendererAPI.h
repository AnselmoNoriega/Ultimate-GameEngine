#pragma once

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Scene/Scene.h"

#include "RendererTypes.h"
#include "RendererCapabilities.h"
#include "RenderCommandBuffer.h"
#include "StorageBufferSet.h"
#include "UniformBufferSet.h"

#include "NotRed/Renderer/UniformBufferSet.h"

namespace NR
{
	class SceneRenderer;
	class VKComputePipeline;

	using RendererID = uint32_t;

	enum class RendererAPIType
	{
		None,
		OpenGL,
		Vulkan
	};

	enum class PrimitiveType
	{
		None, 
		Lines,
		Triangles
	};

	struct Particles
	{
		glm::vec4 position;
		glm::vec4 velocity;
		float mass;
		int id = -1;
	};

	class RendererAPI
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, const Ref<RenderPass>& renderPass, bool explicitClear = false) = 0;
		virtual void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer) = 0;

		virtual void SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow, Ref<Image2D> linearDepth) = 0;
		virtual std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath) = 0;
		virtual void GenerateParticles(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups) = 0;
		virtual Ref<TextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) = 0;

		virtual void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform) = 0;
		virtual void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer()) = 0;
		virtual void RenderParticles(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform) = 0;
		virtual void RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::mat4& transform) = 0;
		virtual void LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups) = 0;
		virtual void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref< StorageBufferSet> storageBufferSet, Ref<Material> material) = 0;
		virtual void SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref< StorageBufferSet> storageBufferSet, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides) = 0;

		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBuffer, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount = 0) = 0;

		virtual void DispatchComputeShader(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups) = 0;
		virtual void ClearImage(Ref<RenderCommandBuffer> commandBuffer, Ref<Image2D> image) = 0;

		virtual RendererCapabilities& GetCapabilities() = 0;

		static RendererAPIType Current() { return sCurrentRendererAPI; }

		static void SetAPI(RendererAPIType api);

	private:
		inline static RendererAPIType sCurrentRendererAPI = RendererAPIType::Vulkan;
	};
}