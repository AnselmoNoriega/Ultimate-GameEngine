#pragma once

#include "NotRed/Scene/Scene.h"

#include "RenderPass.h"

namespace NR
{
	struct SceneRendererOptions
	{
		bool ShowGrid = true;
		bool ShowBoundingBoxes = false;
	};

	class SceneRenderer
	{
	public:
		static void Init();

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(const Scene* scene);
		static void EndScene();

		static void SubmitEntity(Entity* entity);

		static std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath);

		static SceneRendererOptions& GetOptions();

		static Ref<Texture2D> GetFinalColorBuffer();

		static uint32_t GetFinalColorBufferRendererID();

		static Ref<RenderPass> GetFinalRenderPass();

	private:
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();
	};
}