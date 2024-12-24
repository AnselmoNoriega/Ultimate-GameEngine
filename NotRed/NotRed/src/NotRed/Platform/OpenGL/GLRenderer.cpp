#include "nrpch.h"
#include "GLRenderer.h"

#include <glad/glad.h>

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/SceneRenderer.h"

#include "GLMaterial.h"
#include "GLShader.h"
#include "GLTexture.h"
#include "GLImage.h"

namespace NR
{
    struct GLRendererData
    {
        RendererCapabilities RenderCaps;

        Ref<VertexBuffer> mFullscreenQuadVertexBuffer;
        Ref<IndexBuffer> mFullscreenQuadIndexBuffer;
        PipelineSpecification mFullscreenQuadPipelineSpec;

        Ref<RenderPass> ActiveRenderPass;

        Ref<Texture2D> BRDFLut;
    };

    static GLRendererData* sData = nullptr;

    namespace Utils
    {
        static void Clear(float r, float g, float b, float a)
        {
            glClearColor(r, g, b, a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }

        static void SetClearColor(float r, float g, float b, float a)
        {
            glClearColor(r, g, b, a);
        }

        static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
        {
            if (!depthTest)
            {
                glDisable(GL_DEPTH_TEST);
            }

            GLenum glPrimitiveType = 0;
            switch (type)
            {
            case PrimitiveType::Triangles:
                glPrimitiveType = GL_TRIANGLES;
                break;
            case PrimitiveType::Lines:
                glPrimitiveType = GL_LINES;
                break;
            }

            glDrawElements(glPrimitiveType, count, GL_UNSIGNED_INT, nullptr);

            if (!depthTest)
            {
                glEnable(GL_DEPTH_TEST);
            }
        }

        static void SetLineThickness(float thickness)
        {
            glLineWidth(thickness);
        }

        static void GLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
        {
            switch (severity)
            {
            case GL_DEBUG_SEVERITY_HIGH:
            {
                NR_CORE_ERROR("[GL Debug HIGH] {0}", message);
                NR_CORE_ASSERT(false, "GL_DEBUG_SEVERITY_HIGH");
                break;
            }
            case GL_DEBUG_SEVERITY_MEDIUM:
            {
                NR_CORE_WARN("[GL Debug MEDIUM] {0}", message);
                break;
            }
            case GL_DEBUG_SEVERITY_LOW:
            {
                break;
            }
            case GL_DEBUG_SEVERITY_NOTIFICATION:
            {
                break;
            }
            }
        }

    }

    void GLRenderer::Init()
    {
        sData = new GLRendererData();
        Renderer::Submit([]()
            {
                glDebugMessageCallback(Utils::GLLogMessage, nullptr);
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

                auto& caps = sData->RenderCaps;
                caps.Vendor = (const char*)glGetString(GL_VENDOR);
                caps.Device = (const char*)glGetString(GL_RENDERER);
                caps.Version = (const char*)glGetString(GL_VERSION);
                NR_CORE_TRACE("OpenGLRendererData::Init");
                Utils::DumpGPUInfo();

                unsigned int va;
                glGenVertexArrays(1, &va);
                glBindVertexArray(va);

                glEnable(GL_DEPTH_TEST);
                //glEnable(GL_CULL_FACE);
                glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
                glFrontFace(GL_CCW);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glEnable(GL_MULTISAMPLE);
                glEnable(GL_STENCIL_TEST);

                glGetIntegerv(GL_MAX_SAMPLES, &caps.MaxSamples);
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.MaxAnisotropy);

                glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &caps.MaxTextureUnits);

                GLenum error = glGetError();
                while (error != GL_NO_ERROR)
                {
                    NR_CORE_ERROR("GL Error {0}", error);
                    error = glGetError();
                }
            });

        // Create fullscreen quad
        float x = -1;
        float y = -1;
        float width = 2, height = 2;
        struct QuadVertex
        {
            glm::vec3 Position;
            glm::vec2 TexCoord;
        };

        QuadVertex* data = new QuadVertex[4];

        data[0].Position = glm::vec3(x, y, 0.1f);
        data[0].TexCoord = glm::vec2(0, 0);

        data[1].Position = glm::vec3(x + width, y, 0.1f);
        data[1].TexCoord = glm::vec2(1, 0);

        data[2].Position = glm::vec3(x + width, y + height, 0.1f);
        data[2].TexCoord = glm::vec2(1, 1);

        data[3].Position = glm::vec3(x, y + height, 0.1f);
        data[3].TexCoord = glm::vec2(0, 1);

        sData->mFullscreenQuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
        uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
        sData->mFullscreenQuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

        {
            TextureProperties props;
            props.SamplerWrap = TextureWrap::Clamp;
            sData->BRDFLut = Texture2D::Create("Assets/Textures/BRDF_LUT.tga", props);
        }
    }

    void GLRenderer::Shutdown()
    {
        GLShader::ClearUniformBuffers();
        delete sData;
    }

    RendererCapabilities& GLRenderer::GetCapabilities()
    {
        return sData->RenderCaps;
    }

    void GLRenderer::BeginFrame()
    {
    }

    void GLRenderer::EndFrame()
    {
    }

    void GLRenderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, const Ref<RenderPass>& renderPass)
    {
        sData->ActiveRenderPass = renderPass;

        renderPass->GetSpecification().TargetFrameBuffer->Bind();
        bool clear = true;
        if (clear)
        {
            const glm::vec4& clearColor = renderPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor;
            Renderer::Submit([=]() {
                Utils::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
                });
        }
    }

    void GLRenderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
    {
        sData->ActiveRenderPass = nullptr;
    }

    void GLRenderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material)
    {
        const auto& shader = material->GetShader();

        bool depthTest = true;
        Ref<GLMaterial> glMaterial = material.As<GLMaterial>();
        if (material)
        {
            glMaterial->UpdateForRendering();
            depthTest = material->GetFlag(MaterialFlag::DepthTest);
        }

        sData->mFullscreenQuadVertexBuffer->Bind();
        pipeline->Bind();
        sData->mFullscreenQuadIndexBuffer->Bind();
        Renderer::Submit([depthTest]()
            {
                Utils::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
            });

    }

    void GLRenderer::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow)
    {
        if (!environment)
        {
            environment = Renderer::GetEmptyEnvironment();
        }

        Renderer::Submit([environment, shadow]() mutable
            {
                auto shader = Renderer::GetShaderLibrary()->Get("PBR_Static");
                Ref<GLShader> pbrShader = shader.As<GLShader>();

                if (auto resource = pbrShader->GetShaderResource("uEnvRadianceTex"))
                {
                    Ref<GLTextureCube> radianceMap = environment->RadianceMap.As<GLTextureCube>();
                    glBindTextureUnit(resource->GetRegister(), radianceMap->GetRendererID());
                }

                if (auto resource = pbrShader->GetShaderResource("uEnvIrradianceTex"))
                {
                    Ref<GLTextureCube> irradianceMap = environment->IrradianceMap.As<GLTextureCube>();
                    glBindTextureUnit(resource->GetRegister(), irradianceMap->GetRendererID());
                }

                if (auto resource = pbrShader->GetShaderResource("uBRDFLUTTexture"))
                {
                    Ref<GLImage2D> brdfLUTImage = sData->BRDFLut->GetImage();
                    glBindSampler(resource->GetRegister(), brdfLUTImage->GetSamplerRendererID());
                    glBindTextureUnit(resource->GetRegister(), brdfLUTImage->GetRendererID());
                }

                if (auto resource = pbrShader->GetShaderResource("uShadowMapTexture"))
                {
                    Ref<GLImage2D> shadowMapTexture = shadow.As<GLTexture2D>();
                    glBindSampler(resource->GetRegister(), shadowMapTexture->GetSamplerRendererID());
                    glBindTextureUnit(resource->GetRegister(), shadowMapTexture->GetRendererID());
                }
            });
    }

    std::pair<Ref<TextureCube>, Ref<TextureCube>> GLRenderer::CreateEnvironmentMap(const std::string& filepath)
    {
        if (!Renderer::GetConfig().ComputeEnvironmentMaps)
        {
            return { Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture() };
        }

        const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
        const uint32_t irradianceMapSize = 32;

        Ref<GLTextureCube> envUnfiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize).As<GLTextureCube>();
        Ref<GLShader> equirectangularConversionShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap").As<GLShader>();
        TextureProperties props;

        Ref<Texture2D> envEquirect = Texture2D::Create(filepath, props);
        NR_CORE_ASSERT(envEquirect->GetFormat() == ImageFormat::RGBA32F, "Texture is not HDR!");

        equirectangularConversionShader->Bind();
        envEquirect->Bind(1);
        Renderer::Submit([envUnfiltered, cubemapSize, envEquirect]()
            {
                glBindImageTexture(0, envUnfiltered->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
                glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                glGenerateTextureMipmap(envUnfiltered->GetRendererID());
            });

        Ref<GLShader> envFilteringShader = Renderer::GetShaderLibrary()->Get("EnvironmentMipFilter").As<GLShader>();

        Ref<GLTextureCube> envFiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize).As<GLTextureCube>();

        Renderer::Submit([envUnfiltered, envFiltered]()
            {
                glCopyImageSubData(envUnfiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
                    envFiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
                    envFiltered->GetWidth(), envFiltered->GetHeight(), 6);
            });

        envFilteringShader->Bind();
        envUnfiltered->Bind(1);

        Renderer::Submit([envFilteringShader, envUnfiltered, envFiltered, cubemapSize]() {
            const float deltaRoughness = 1.0f / glm::max((float)(envFiltered->GetMipLevelCount() - 1.0f), 1.0f);
            for (uint32_t level = 1, size = cubemapSize / 2; level < envFiltered->GetMipLevelCount(); ++level, size /= 2)
            {
                glBindImageTexture(0, envFiltered->GetRendererID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                const GLint roughnessUniformLocation = glGetUniformLocation(envFilteringShader->GetRendererID(), "uUniforms.Roughness");
                NR_CORE_ASSERT(roughnessUniformLocation != -1);
                glUniform1f(roughnessUniformLocation, (float)level * deltaRoughness);

                const GLuint numGroups = glm::max(1u, size / 32);
                glDispatchCompute(numGroups, numGroups, 6);
                glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            }
            });

        Ref<GLShader> envIrradianceShader = Renderer::GetShaderLibrary()->Get("EnvironmentIrradiance").As<GLShader>();

        Ref<GLTextureCube> irradianceMap = TextureCube::Create(ImageFormat::RGBA32F, irradianceMapSize, irradianceMapSize).As<GLTextureCube>();
        envIrradianceShader->Bind();
        envFiltered->Bind(1);
        Renderer::Submit([irradianceMap, envIrradianceShader]()
            {
                glBindImageTexture(0, irradianceMap->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

                const GLint samplesUniformLocation = glGetUniformLocation(envIrradianceShader->GetRendererID(), "uUniforms.Samples");
                NR_CORE_ASSERT(samplesUniformLocation != -1);
                const uint32_t samples = Renderer::GetConfig().IrradianceMapComputeSamples;
                glUniform1ui(samplesUniformLocation, samples);

                glDispatchCompute(irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
                glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                glGenerateTextureMipmap(irradianceMap->GetRendererID());
            });

        return { envFiltered, irradianceMap };
    }

    void GLRenderer::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform)
    {
#if 0
        mesh->mVertexBuffer->Bind();
        pipeline->Bind();
        mesh->mIndexBuffer->Bind();

        auto& materials = mesh->GetMaterials();
        for (Submesh& submesh : mesh->mSubmeshes)
        {
            auto material = materials[submesh.MaterialIndex].As<GLMaterial>();
            auto shader = material->GetShader().As<GLShader>();
            material->UpdateForRendering();

            auto transformUniform = transform * submesh.Transform;
            shader->SetUniform("uRenderer.Transform", transformUniform);

            Renderer::Submit([submesh, material]()
                {
                    if (material->GetFlag(MaterialFlag::DepthTest))
                    {
                        glEnable(GL_DEPTH_TEST);
                    }
                    else
                    {
                        glDisable(GL_DEPTH_TEST);
                    }

                    glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
                });
        }
#endif
    }

    void GLRenderer::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms)
    {
#if 0
        mesh->mVertexBuffer->Bind();
        pipeline->Bind();
        mesh->mIndexBuffer->Bind();

        auto shader = pipeline->GetSpecification().Shader.As<GLShader>();
        shader->Bind();

        for (Submesh& submesh : mesh->mSubmeshes)
        {
            auto transformUniform = transform * submesh.Transform;
            shader->SetUniform("uRenderer.Transform", transformUniform);

            Renderer::Submit([submesh]()
                {
                    glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
                });
        }
#endif
    }

    void GLRenderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, const glm::mat4& transform)
    {
        sData->mFullscreenQuadVertexBuffer->Bind();
        pipeline->Bind();
        sData->mFullscreenQuadIndexBuffer->Bind();
        Ref<GLMaterial> glMaterial = material.As<GLMaterial>();
        glMaterial->UpdateForRendering();

        auto shader = material->GetShader().As<GLShader>();
        shader->SetUniform("uRenderer.Transform", transform);

        Renderer::Submit([material]()
            {
                if (material->GetFlag(MaterialFlag::DepthTest))
                {
                    glEnable(GL_DEPTH_TEST);
                }
                else
                {
                    glDisable(GL_DEPTH_TEST);
                }

                glDrawElements(GL_TRIANGLES, sData->mFullscreenQuadIndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);
            });
    }

    void GLRenderer::LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<VKComputePipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups)
    {
    }
}