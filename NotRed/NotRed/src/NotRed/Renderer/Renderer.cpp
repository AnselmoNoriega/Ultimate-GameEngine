#include "nrpch.h"
#include "Renderer.h"

#include <glad/glad.h>
#include <map>

#include "RendererAPI.h"
#include "SceneRenderer.h"
#include "Renderer2D.h"

#include "Shader.h"

#include "NotRed/Platform/OpenGL/GLRenderer.h"
#include "NotRed/Platform/Vulkan/VkRenderer.h"

#include "NotRed/Core/Timer.h"

namespace NR
{
    static std::unordered_map<size_t, Ref<Pipeline>> sPipelineCache;

    static RendererAPI* sRendererAPI = nullptr;

    struct ShaderDependencies
    {
        std::vector<Ref<Pipeline>> Pipelines;
        std::vector<Ref<Material>> Materials;
    };

    static std::unordered_map<size_t, ShaderDependencies> sShaderDependencies;

    void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline)
    {
        sShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
    }

    void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material)
    {
        sShaderDependencies[shader->GetHash()].Materials.push_back(material);
    }

    void Renderer::OnShaderReloaded(size_t hash)
    {
        if (sShaderDependencies.find(hash) != sShaderDependencies.end())
        {
            auto& dependencies = sShaderDependencies.at(hash);
            for (auto& pipeline : dependencies.Pipelines)
            {
                pipeline->Invalidate();
            }

            for (auto& material : dependencies.Materials)
            {
                material->Invalidate();
            }
        }
    }

    void RendererAPI::SetAPI(RendererAPIType api)
    {
        sCurrentRendererAPI = api;
    }

    struct RendererData
    {
        RendererConfig Config;

        Ref<ShaderLibrary> mShaderLibrary;

        Ref<Texture2D> WhiteTexture;
        Ref<TextureCube> BlackCubeTexture;
        Ref<Environment> EmptyEnvironment;
        std::map<uint32_t, std::map<uint32_t, Ref<UniformBuffer>>> UniformBuffers;
    };

    static RendererData* sData = nullptr;
    static RenderCommandQueue* sCommandQueue = nullptr;

    static RendererAPI* InitRendererAPI()
    {
        switch (RendererAPI::Current())
        {
        case RendererAPIType::OpenGL: return new GLRenderer();
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
        sRendererAPI = InitRendererAPI();

        sData->mShaderLibrary = Ref<ShaderLibrary>::Create();

        Renderer::GetShaderLibrary()->Load("Assets/Shaders/EnvironmentMipFilter");
        Renderer::GetShaderLibrary()->Load("Assets/Shaders/EquirectangularToCubeMap");
        Renderer::GetShaderLibrary()->Load("Assets/Shaders/EnvironmentIrradiance");
        Renderer::GetShaderLibrary()->Load("Assets/Shaders/PreethamSky");

        Renderer::GetShaderLibrary()->Load("Assets/Shaders/Grid");
        Renderer::GetShaderLibrary()->Load("Assets/Shaders/HDR");
        Renderer::GetShaderLibrary()->Load("Assets/Shaders/PBR_Static", true);
        Renderer::GetShaderLibrary()->Load("Assets/Shaders/Skybox");
        Renderer::GetShaderLibrary()->Load("Assets/Shaders/ShadowMap");

        // Compile shaders
        Renderer::WaitAndRender();

        uint32_t whiteTextureData = 0xffffffff;
        sData->WhiteTexture = Texture2D::Create(ImageFormat::RGBA, 1, 1, &whiteTextureData);

        uint32_t blackTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
        sData->BlackCubeTexture = TextureCube::Create(ImageFormat::RGBA, 1, 1, &blackTextureData);

        sData->EmptyEnvironment = Ref<Environment>::Create(sData->BlackCubeTexture, sData->BlackCubeTexture);

        sRendererAPI->Init();
        SceneRenderer::Init();
    }

    void Renderer::Shutdown()
    {
        sShaderDependencies.clear();
        SceneRenderer::Shutdown();
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
        NR_SCOPE_PERF("Renderer::WaitAndRender");
        sCommandQueue->Execute();
    }

    void Renderer::BeginRenderPass(Ref<RenderPass> renderPass, bool clear)
    {
        NR_CORE_ASSERT(renderPass, "Render pass cannot be null!");

        sRendererAPI->BeginRenderPass(renderPass);
    }

    void Renderer::EndRenderPass()
    {
        sRendererAPI->EndRenderPass();
    }

    void Renderer::BeginFrame()
    {
        sRendererAPI->BeginFrame();
    }

    void Renderer::EndFrame()
    {
        sRendererAPI->EndFrame();
    }

    void Renderer::SetSceneEnvironment(Ref<Environment> environment, Ref<Image2D> shadow)
    {
        sRendererAPI->SetSceneEnvironment(environment, shadow);
    }

    std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::string& filepath)
    {
        return sRendererAPI->CreateEnvironmentMap(filepath);
    }

    void Renderer::RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform)
    {
        sRendererAPI->RenderMesh(pipeline, mesh, transform);
    }

    void Renderer::RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms)
    {
        sRendererAPI->RenderMesh(pipeline, mesh, material, transform, additionalUniforms);
    }

    void Renderer::RenderQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform)
    {
        sRendererAPI->RenderQuad(pipeline, material, transform);
    }

    void Renderer::SubmitQuad(Ref<Material> material, const glm::mat4& transform)
    {
    }

    void Renderer::SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<Material> material)
    {
        sRendererAPI->SubmitFullScreenQuad(pipeline, material);
    }

    void Renderer::DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color)
    {
        for (Submesh& submesh : mesh->mSubmeshes)
        {
            auto& aabb = submesh.BoundingBox;
            auto aabbTransform = transform * submesh.Transform;
            DrawAABB(aabb, aabbTransform);
        }
    }

    void Renderer::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color)
    {
        glm::vec4 min = { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f };
        glm::vec4 max = { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f };

        glm::vec4 corners[8] =
        {
            transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f },
            transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f },
            transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f },
            transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f },

            transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f },
            transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f },
            transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f },
            transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f }
        };
        for (uint32_t i = 0; i < 4; ++i)
        {
            Renderer2D::DrawLine(corners[i], corners[(i + 1) % 4], color);
        }
        for (uint32_t i = 0; i < 4; ++i)
        {
            Renderer2D::DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);
        }
        for (uint32_t i = 0; i < 4; ++i)
        {
            Renderer2D::DrawLine(corners[i], corners[i + 4], color);
        }
    }

    void Renderer::SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t set)
    {
        sData->UniformBuffers[set][uniformBuffer->GetBinding()] = uniformBuffer;
    }

    Ref<UniformBuffer> Renderer::GetUniformBuffer(uint32_t binding, uint32_t set)
    {
        NR_CORE_ASSERT(sData->UniformBuffers.find(set) != sData->UniformBuffers.end());
        NR_CORE_ASSERT(sData->UniformBuffers.at(set).find(binding) != sData->UniformBuffers.at(set).end());
        return sData->UniformBuffers.at(set).at(binding);
    }

    Ref<Texture2D> Renderer::GetWhiteTexture()
    {
        return sData->WhiteTexture;
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
        return sData->Config;
    }

    Ref<TextureCube> Renderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
    {
        return sRendererAPI->CreatePreethamSky(turbidity, azimuth, inclination);
    }
}