#pragma once

#include "RenderCommandQueue.h"
#include "RendererAPI.h"
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

		static void DrawIndexed(uint32_t count, bool depthTestActive = true);

		static void ClearMagenta();

		static const Scope<ShaderLibrary>& GetShaderLibrary() { return Get().mShaderLibrary; }

		template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();
				pFunc->~FuncT();
				};
			auto storageBuffer = sInstance->mCommandQueue.Allocate(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
		}

		void WaitAndRender();

		inline static Renderer& Get() { return *sInstance; }

		static void BeginRenderPass(const Ref<RenderPass>& renderPass) { sInstance->IBeginRenderPass(renderPass); }
		static void EndRenderPass() { sInstance->IEndRenderPass(); }

		static void SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform, const Ref<MaterialInstance>& overrideMaterial = nullptr) { sInstance->ISubmitMesh(mesh, transform, overrideMaterial); }

	private:
		void IBeginRenderPass(const Ref<RenderPass>& renderPass);
		void IEndRenderPass();

		void ISubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform, const Ref<MaterialInstance>& overrideMaterial);

	private:
		static Renderer* sInstance;

	private:
		Ref<RenderPass> mActiveRenderPass;

		RenderCommandQueue mCommandQueue;
		Scope<ShaderLibrary> mShaderLibrary;
	};
}