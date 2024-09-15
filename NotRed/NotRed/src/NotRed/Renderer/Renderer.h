#pragma once

#include "RendererContext.h"
#include "RenderCommandQueue.h"
#include "RenderPass.h"

#include "Mesh.h"

#include "NotRed/Core/Application.h"

#include "RendererCapabilities.h"

#include "NotRed/Scene/Scene.h"

namespace NR
{
    class ShaderLibrary;

    class Renderer
    {
    public:
        typedef void(*RenderCommandFn)(void*);

        static Ref<RendererContext> GetContext()
        {
            return Application::Get().GetWindow().GetRenderContext();
        }

    public:

        static void Init();
        static void Shutdown();

        static RendererCapabilities& GetCapabilities();
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

        static void BeginFrame();
        static void EndFrame();

        static void SetSceneEnvironment(Ref<Environment> environment, Ref<Image2D> shadow);
        static std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath);

        static void RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform);
        static void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform);
        static void RenderQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform);
        static void SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<Material> material);

        static void SubmitQuad(Ref<Material> material, const glm::mat4& transform = glm::mat4(1.0f));
        static void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> overrideMaterial = nullptr);

        static void DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));
        static void DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

        static Ref<Texture2D> GetWhiteTexture();
        static Ref<TextureCube> GetBlackCubeTexture();
        static Ref<Environment> GetEmptyEnvironment();

        static void RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline);
        static void RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material);
        static void OnShaderReloaded(size_t hash);

    private:
        static RenderCommandQueue& GetRenderCommandQueue();
    };
}