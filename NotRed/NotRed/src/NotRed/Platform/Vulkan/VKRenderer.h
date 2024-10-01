#pragma once

#include "NotRed/Renderer/RendererAPI.h"

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

		void BeginRenderPass(const Ref<RenderPass>& renderPass) override;
		void EndRenderPass() override;
		void SubmitFullScreenQuad(Ref<Pipeline> pipeline, Ref<Material> material) override;

		void SetSceneEnvironment(Ref<Environment> environment, Ref<Image2D> shadow) override;
		std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath) override;
		void GenerateParticles() override;
		Ref<TextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) override;

		void RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform) override;
		void RenderParticles(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform) override;
		void RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer()) override;
		void RenderQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform) override;
	};
}