#pragma once

#include "RenderCommandQueue.h"
#include "RenderPass.h"

#include "Mesh.h"

namespace NR
{
	class ShaderLibrary;

	class Renderer
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		static void Init();

		static void Clear();
		static void Clear(float r, float g, float b, float a = 1.0f);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest = true);

		static void SetLineThickness(float thickness);

		static void ClearMagenta();

		static Ref<ShaderLibrary> GetShaderLibrary();

		template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();
				pFunc->~FuncT();
				};
			auto storageBuffer = GetRenderCommandQueue().Allocate(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
		}

		static void WaitAndRender();

		static void BeginRenderPass(Ref<RenderPass> renderPass, bool clear = true);
		static void EndRenderPass();

		static void SubmitQuad(Ref<MaterialInstance> material, const glm::mat4& transform = glm::mat4(1.0f));
		static void SubmitFullScreenQuad(Ref<MaterialInstance> material);
		static void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<MaterialInstance> overrideMaterial = nullptr);

		static void DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

	private:
		static RenderCommandQueue& GetRenderCommandQueue();
	};
}