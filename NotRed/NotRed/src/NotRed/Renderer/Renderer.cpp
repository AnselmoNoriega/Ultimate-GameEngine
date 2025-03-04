#include "nrpch.h"
#include "Renderer.h"

#include <map>
#include <filesystem>
#include <glad/glad.h>

#include "RendererAPI.h"
#include "SceneRenderer.h"
#include "Renderer2D.h"

#include "Shader.h"

#include "NotRed/Project/Project.h"
#include "NotRed/Platform/Vulkan/VKComputePipeline.h"
#include "NotRed/Platform/Vulkan/VkRenderer.h"

#include "NotRed/Platform/Vulkan/VKContext.h"

#include "NotRed/Core/Timer.h"
#include "NotRed/Debug/Profiler.h"

namespace NR
{
    static std::unordered_map<size_t, Ref<Pipeline>> sPipelineCache;

    static RendererAPI* sRendererAPI = nullptr;

    struct ShaderDependencies
    {
        std::vector<Ref<VKComputePipeline>> ComputePipelines;
        std::vector<Ref<Pipeline>> Pipelines;
        std::vector<Ref<Material>> Materials;
    };

    static std::unordered_map<size_t, ShaderDependencies> sShaderDependencies;

    void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<VKComputePipeline> computePipeline)
    {
        sShaderDependencies[shader->GetHash()].ComputePipelines.push_back(computePipeline);
    }

    void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline)
    {
        sShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
    }

    void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material)
    {
        sShaderDependencies[shader->GetHash()].Materials.push_back(material);
    }

    void Renderer::ShaderReloaded(size_t hash)
    {
        if (sShaderDependencies.find(hash) != sShaderDependencies.end())
        {
            auto& dependencies = sShaderDependencies.at(hash);
            for (auto& pipeline : dependencies.Pipelines)
            {
                pipeline->Invalidate();
            }

            for (auto& computePipeline : dependencies.ComputePipelines)
            {
                computePipeline->CreatePipeline();
            }

            for (auto& material : dependencies.Materials)
            {
                material->Invalidate();
            }
        }
    }

    void RendererAPI::SetAPI(RendererAPIType api)
    {
        NR_CORE_VERIFY(api == RendererAPIType::Vulkan, "Vulkan is currently the only supported Renderer API");
        sCurrentRendererAPI = api;
    }

    uint32_t Renderer::GetCurrentFrameIndex()
    {
        return Application::Get().GetWindow().GetSwapChain().GetCurrentBufferIndex();
    }

    struct RendererData
    {
        Ref<ShaderLibrary> mShaderLibrary;

        Ref<Texture2D> WhiteTexture;
        Ref<Texture2D> BRDFLutTexture;
        Ref<Texture2D> BlackTexture;
        Ref<TextureCube> BlackCubeTexture;
        Ref<Environment> EmptyEnvironment;
    };

    static RendererData* sData = nullptr;
    static RenderCommandQueue* sCommandQueue = nullptr;
    static RenderCommandQueue sResourceFreeQueue[3];

    static RendererAPI* InitRendererAPI()
    {
        switch (RendererAPI::Current())
        {
        //case RendererAPIType::OpenGL: return new GLRenderer();
        case RendererAPIType::Vulkan: return new VKRenderer();
        default:
        {
            NR_CORE_ASSERT(false, "Unknown RendererAPI");
            return nullptr;
        }
        }
    }

    void Renderer::Init()
    {
        sData = new RendererData();
        sCommandQueue = new RenderCommandQueue();

        Renderer::GetConfig().FramesInFlight = glm::min<uint32_t>(Renderer::GetConfig().FramesInFlight, Application::Get().GetWindow().GetSwapChain().GetImageCount());

        sRendererAPI = InitRendererAPI();

        sData->mShaderLibrary = Ref<ShaderLibrary>::Create();

        Renderer::GetShaderLibrary()->Load("Resources/Shaders/PBR_Static", true);
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/PBR_Anim");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Grid");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Wireframe");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Wireframe_Anim");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Skybox");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/ShadowMap");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/ShadowMap_Anim");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Particle");

        // Environment compute shaders
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentMipFilter");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/EquirectangularToCubeMap");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentIrradiance");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/PreethamSky");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/ParticleGen");

        // Post-processing
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/Bloom");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/DOF");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/SceneComposite", true);
        
        // Light-culling
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/PreDepth");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/PreDepth_Anim");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/LightCulling");
        
		//HBAO
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Deinterleaving");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Reinterleaving");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/HBAO");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/HBAOBlur");
		
		// Renderer2D Shaders
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Line");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Circle");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Text");

        // Jump Flood Shaders
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Init");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Pass");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Composite");

        // Misc
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/SelectedGeometry");
        Renderer::GetShaderLibrary()->Load("Resources/Shaders/SelectedGeometry_Anim");

        // Compile shaders
        Renderer::WaitAndRender();

        uint32_t whiteTextureData = 0xffffffff;
        sData->WhiteTexture = Texture2D::Create(ImageFormat::RGBA, 1, 1, &whiteTextureData);

        uint32_t blackTextureData = 0xff000000;
        sData->BlackTexture = Texture2D::Create(ImageFormat::RGBA, 1, 1, &blackTextureData);

        uint32_t blackCubeTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
        sData->BlackCubeTexture = TextureCube::Create(ImageFormat::RGBA, 1, 1, &blackCubeTextureData);
        
        {
            TextureProperties props;
            props.SamplerWrap = TextureWrap::Clamp;
            sData->BRDFLutTexture = Texture2D::Create("Resources/Renderer/BRDF_LUT.tga", props);
        }

        sData->EmptyEnvironment = Ref<Environment>::Create(sData->BlackCubeTexture, sData->BlackCubeTexture);

        sRendererAPI->Init();
    }

    void Renderer::Shutdown()
    {
        sShaderDependencies.clear();
        sRendererAPI->Shutdown();

        delete sData;
        delete sCommandQueue;
    }

    RendererCapabilities& Renderer::GetCapabilities()
    {
        return sRendererAPI->GetCapabilities();
    }

    Ref<ShaderLibrary> Renderer::GetShaderLibrary()
    {
        return sData->mShaderLibrary;
    }

    void Renderer::WaitAndRender()
    {
        NR_PROFILE_FUNC();
        NR_SCOPE_PERF("Renderer::WaitAndRender");
        sCommandQueue->Execute();
    }

    void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
    {
        NR_CORE_ASSERT(renderPass, "Render pass cannot be null!");

        sRendererAPI->BeginRenderPass(renderCommandBuffer, renderPass, explicitClear);
    }

    void Renderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
    {
        sRendererAPI->EndRenderPass(renderCommandBuffer);
    }

    void Renderer::BeginFrame()
    {
        sRendererAPI->BeginFrame();
    }

    void Renderer::EndFrame()
    {
        sRendererAPI->EndFrame();
    }

    void Renderer::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow, Ref<Image2D> linearDepth)
    {
        sRendererAPI->SetSceneEnvironment(sceneRenderer, environment, shadow, linearDepth);
    }

    std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::string& filepath)
    {
        return sRendererAPI->CreateEnvironmentMap(filepath);
    }

    void Renderer::GenerateParticles(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups)
    {
        sRendererAPI->GenerateParticles(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, material, workGroups);
    }

    void Renderer::DispatchComputeShader(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> computePipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups)
    {
        sRendererAPI->DispatchComputeShader(renderCommandBuffer, computePipeline, uniformBufferSet, storageBufferSet, material, workGroups);
    }

    void Renderer::ClearImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image)
    {
        sRendererAPI->ClearImage(renderCommandBuffer, image);
    }

    Ref<TextureCube> Renderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
    {
        return sRendererAPI->CreatePreethamSky(turbidity, azimuth, inclination);
    }

    void Renderer::RenderStaticMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<StaticMesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount)
    {
        sRendererAPI->RenderStaticMesh(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, materialTable, transformBuffer, transformOffset, instanceCount);
    }

    void Renderer::RenderSubmesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, const glm::mat4& transform)
    {
        sRendererAPI->RenderSubmesh(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, materialTable, transform);
    }

    void Renderer::RenderSubmeshInstanced(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount)
    {
        sRendererAPI->RenderSubmeshInstanced(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, materialTable, transformBuffer, transformOffset, instanceCount);
    }

    void Renderer::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Ref<Material> material, Buffer additionalUniforms)
    {
        sRendererAPI->RenderMesh(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, material, transformBuffer, transformOffset, instanceCount, additionalUniforms);
    }

    void Renderer::RenderStaticMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<StaticMesh> mesh, uint32_t submeshIndex, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Ref<Material> material, Buffer additionalUniforms)
    {
        sRendererAPI->RenderStaticMesh(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, material, transformBuffer, transformOffset, instanceCount, additionalUniforms);
    }

    void Renderer::RenderParticles(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform)
    {
        sRendererAPI->RenderParticles(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, materialTable, transform);
    }

    void Renderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::mat4& transform)
    {
        sRendererAPI->RenderQuad(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, material, transform);
    }

    void Renderer::SubmitQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, const glm::mat4& transform)
    {
        NR_CORE_ASSERT(false, "Not Implemented");
    }

    void Renderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material)
    {
        sRendererAPI->SubmitFullscreenQuad(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, material);
    }

    void Renderer::SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides)
    {
        sRendererAPI->SubmitFullscreenQuadWithOverrides(renderCommandBuffer, pipeline, uniformBufferSet, nullptr, material, vertexShaderOverrides, fragmentShaderOverrides);
    }

    void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount)
    {
        sRendererAPI->RenderGeometry(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, material, vertexBuffer, indexBuffer, transform, indexCount);
    }

    Ref<Texture2D> Renderer::GetWhiteTexture()
    {
        return sData->WhiteTexture;
    }

    Ref<Texture2D> Renderer::GetBRDFLutTexture()
    {
        return sData->BRDFLutTexture;
    }

    Ref<Texture2D> Renderer::GetBlackTexture()
    {
        return sData->BlackTexture;
    }

    Ref<TextureCube> Renderer::GetBlackCubeTexture()
    {
        return sData->BlackCubeTexture;
    }

    Ref<Environment> Renderer::GetEmptyEnvironment()
    {
        return sData->EmptyEnvironment;
    }

    RenderCommandQueue& Renderer::GetRenderCommandQueue()
    {
        return *sCommandQueue;
    }

    RendererConfig& Renderer::GetConfig()
    {
        static RendererConfig config;
        return config;
    }

    void Renderer::LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> computePipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups)
    {
        sRendererAPI->LightCulling(renderCommandBuffer, computePipeline, uniformBufferSet, storageBufferSet, material, screenSize, workGroups);
    }

    RenderCommandQueue& Renderer::GetRenderResourceReleaseQueue(uint32_t index)
    {
        return sResourceFreeQueue[index];
    }
}