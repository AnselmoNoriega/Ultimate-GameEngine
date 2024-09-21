#pragma once

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
	class GLRenderer : public RendererAPI
	{
	public:
		void Init() override;
		void Shutdown() override;

		RendererCapabilities& GetCapabilities() override;

		void BeginFrame() override;
		void EndFrame() override;

		void BeginRenderPass(const Ref<RenderPass>& renderPass) override;
		void EndRenderPass() override;
		void SubmitFullScreenQuad(Ref<Pipeline> pipeline, Ref<Material> material) override;

		void SetSceneEnvironment(Ref<Environment> environment, Ref<Image2D> shadow) override;
		virtual Ref<TextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) override { NR_CORE_ASSERT(false); return nullptr; }
		std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath) override;

		void RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform) override;
		void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform) override;
		void RenderQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform) override;
	};
}