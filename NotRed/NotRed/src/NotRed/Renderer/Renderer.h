#pragma once

#include "RendererContext.h"
#include "RenderCommandQueue.h"
#include "RenderPass.h"

#include "Mesh.h"

#include "NotRed/Core/Application.h"

#include "RendererCapabilities.h"
#include "RenderCommandBuffer.h"
#include "UniformBufferSet.h"
#include "StorageBufferSet.h"

#include "NotRed/Scene/Scene.h"

namespace NR
{
    class VKComputePipeline;
    class ShaderLibrary;

    struct RendererConfig
    {
        uint32_t FramesInFlight = 3;

        bool ComputeEnvironmentMaps = true;

        //Tiering Settings
        uint32_t EnvironmentMapResolution = 1024;
        uint32_t IrradianceMapComputeSamples = 512;
    };

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

        template<typename FuncT>
        static void SubmitResourceFree(FuncT&& func)
        {
            auto renderCmd = [](void* ptr) {
                auto pFunc = (FuncT*)ptr;
                (*pFunc)();
                pFunc->~FuncT();
                };

            Submit([renderCmd, func]()
                {
                    uint32_t index = Renderer::GetCurrentFrameIndex();
                    auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
                    new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
                });
        }

        static void WaitAndRender();

        static void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false);
        static void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer);

        static void BeginFrame();
        static void EndFrame();

        static void SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow);
        static std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath);
        static void GenerateParticles();
        static Ref<TextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination);

        static void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, const glm::mat4& transform);
        static void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> material, Buffer additionalUniforms = Buffer());
        static void RenderParticles(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, const glm::mat4& transform);
        static void RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::mat4& transform);
        static void LightCulling(Ref<VKComputePipeline> computePipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups);

        static void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount = 0);

        static void SubmitQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, const glm::mat4& transform = glm::mat4(1.0f));
        static void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material);
        
        static Ref<Texture2D> GetWhiteTexture();
        static Ref<Texture2D> GetBlackTexture();
        static Ref<TextureCube> GetBlackCubeTexture();
        static Ref<Environment> GetEmptyEnvironment();

        static void RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline);
        static void RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material);
        static void OnShaderReloaded(size_t hash);

        static uint32_t GetCurrentFrameIndex();

        static RendererConfig& GetConfig();
        
        static RenderCommandQueue& GetRenderResourceReleaseQueue(uint32_t index);

    private:
        static RenderCommandQueue& GetRenderCommandQueue();
    };

    namespace Utils
    {
        inline void DumpGPUInfo()
        {
            auto& caps = Renderer::GetCapabilities();
            NR_CORE_TRACE("GPU Info:");
            NR_CORE_TRACE("  Vendor: {0}", caps.Vendor);
            NR_CORE_TRACE("  Device: {0}", caps.Device);
            NR_CORE_TRACE("  Version: {0}", caps.Version);
        }
    }
}