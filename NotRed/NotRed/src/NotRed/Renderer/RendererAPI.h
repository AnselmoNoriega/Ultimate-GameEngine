#pragma once

#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Scene/Scene.h"

#include "RendererTypes.h"
#include "RendererCapabilities.h"

namespace NR
{
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
		int id;
	};

	class RendererAPI
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void SubmitFullScreenQuad(Ref<Pipeline> pipeline, Ref<Material> material) = 0;
		virtual void BeginRenderPass(const Ref<RenderPass>& renderPass) = 0;
		virtual void EndRenderPass() = 0;

		virtual void SetSceneEnvironment(Ref<Environment> environment, Ref<Image2D> shadow) = 0;
		virtual std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath) = 0;
		virtual void GenerateParticles() = 0;
		virtual Ref<TextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) = 0;

		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform) = 0;
		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer()) = 0;
		virtual void RenderQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform) = 0;

		virtual RendererCapabilities& GetCapabilities() = 0;

		static RendererAPIType Current() { return sCurrentRendererAPI; }

		static void SetAPI(RendererAPIType api);

	private:
		inline static RendererAPIType sCurrentRendererAPI = RendererAPIType::Vulkan;
	};
}