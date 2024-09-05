#include "nrpch.h"
#include "SceneRenderer.h"

#include <limits>

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include "Renderer.h"
#include "Renderer2D.h"

#include "SceneEnvironment.h"

#include "NotRed/Renderer/MeshFactory.h"

#include "NotRed/ImGui/ImGui.h"

#include "NotRed/Core/Timer.h"

namespace NR
{
    struct SceneRendererData
    {
        const Scene* ActiveScene = nullptr;
        struct SceneInfo
        {
            SceneRendererCamera SceneCamera;

            Ref<MaterialInstance> SkyboxMaterial;
            Environment SceneEnvironment;
            float SceneEnvironmentIntensity;
            LightEnvironment SceneLightEnvironment;
            Light ActiveLight;
        } SceneData;

        Ref<Texture2D> BRDFLUT;
        Ref<Shader> CompositeShader;
        Ref<Shader> BloomBlurShader;
        Ref<Shader> BloomBlendShader;

        Ref<RenderPass> GeoPass;
        Ref<RenderPass> CompositePass;
        Ref<RenderPass> BloomBlurPass[2];
        Ref<RenderPass> BloomBlendPass;

        Ref<Shader> ShadowMapShader, ShadowMapAnimShader;
        Ref<RenderPass> ShadowMapRenderPass[4];
        float ShadowMapSize = 20.0f;
        float LightDistance = 0.1f;
        glm::mat4 LightMatrices[4];
        glm::mat4 LightViewMatrix;
        float CascadeSplitLambda = 0.91f;
        glm::vec4 CascadeSplits;
        float CascadeFarPlaneOffset = 15.0f, CascadeNearPlaneOffset = -15.0f;
        bool ShowCascades = false;
        bool SoftShadows = true;
        float LightSize = 0.25f;
        float MaxShadowDistance = 200.0f;
        float ShadowFade = 25.0f;
        float CascadeTransitionFade = 1.0f;
        bool CascadeFading = true;

        bool EnableBloom = false;
        float BloomThreshold = 1.5f;

        glm::vec2 FocusPoint = { 0.5f, 0.5f };

        RendererID ShadowMapSampler;

        struct DrawCommand
        {
            Ref<Mesh> Mesh;
            Ref<MaterialInstance> Material;
            glm::mat4 Transform;
        };
        std::vector<DrawCommand> DrawList;
        std::vector<DrawCommand> SelectedMeshDrawList;
        std::vector<DrawCommand> ColliderDrawList;
        std::vector<DrawCommand> ShadowPassDrawList;

        Ref<MaterialInstance> GridMaterial;
        Ref<MaterialInstance> OutlineMaterial, OutlineAnimMaterial;
        Ref<MaterialInstance> ColliderMaterial;

        SceneRendererOptions Options;
    };

    struct SceneRendererStats
    {
        float ShadowPass = 0.0f;
        float GeometryPass = 0.0f;
        float CompositePass = 0.0f;

        Timer ShadowPassTimer;
        Timer GeometryPassTimer;
        Timer CompositePassTimer;
    };

    static SceneRendererData sData;
    static SceneRendererStats sStats;

    void SceneRenderer::Init()
    {
        {
            FrameBufferSpecification geoFrameBufferSpec;
            geoFrameBufferSpec.Attachments = { FrameBufferTextureFormat::RGBA16F, FrameBufferTextureFormat::RGBA16F, FrameBufferTextureFormat::Depth };
            geoFrameBufferSpec.Samples = 8;
            geoFrameBufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

            RenderPassSpecification geoRenderPassSpec;
            geoRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(geoFrameBufferSpec);
            sData.GeoPass = RenderPass::Create(geoRenderPassSpec);
        }
        {
            FrameBufferSpecification compFrameBufferSpec;
            compFrameBufferSpec.Attachments = { FrameBufferTextureFormat::RGBA8 };
            compFrameBufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

            RenderPassSpecification compRenderPassSpec;
            compRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(compFrameBufferSpec);
            sData.CompositePass = RenderPass::Create(compRenderPassSpec);
        }
        {
            FrameBufferSpecification bloomBlurFrameBufferSpec;
            bloomBlurFrameBufferSpec.Attachments = { FrameBufferTextureFormat::RGBA16F };
            bloomBlurFrameBufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

            RenderPassSpecification bloomBlurRenderPassSpec;
            bloomBlurRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(bloomBlurFrameBufferSpec);
            sData.BloomBlurPass[0] = RenderPass::Create(bloomBlurRenderPassSpec);
            bloomBlurRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(bloomBlurFrameBufferSpec);
            sData.BloomBlurPass[1] = RenderPass::Create(bloomBlurRenderPassSpec);
        }
        {
            FrameBufferSpecification bloomBlendFramebufferSpec;
            bloomBlendFramebufferSpec.Attachments = { FrameBufferTextureFormat::RGBA8 };
            bloomBlendFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

            RenderPassSpecification bloomBlendRenderPassSpec;
            bloomBlendRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(bloomBlendFramebufferSpec);
            sData.BloomBlendPass = RenderPass::Create(bloomBlendRenderPassSpec);
        }

        sData.CompositeShader = Shader::Create("Assets/Shaders/HDR");
        sData.BloomBlurShader = Shader::Create("Assets/Shaders/BloomBlur");
        sData.BloomBlendShader = Shader::Create("Assets/Shaders/BloomBlend");
        sData.BRDFLUT = Texture2D::Create("Assets/Textures/BRDF_LUT.tga");

        // Grid
        auto gridShader = Shader::Create("Assets/Shaders/Grid");
        sData.GridMaterial = MaterialInstance::Create(Material::Create(gridShader));
        sData.GridMaterial->ModifyFlags(MaterialFlag::TwoSided);
        float gridScale = 16.025f, gridSize = 0.025f;
        sData.GridMaterial->Set("uScale", gridScale);
        sData.GridMaterial->Set("uRes", gridSize);

        // Outline
        auto outlineShader = Shader::Create("Assets/Shaders/Outline");
        sData.OutlineMaterial = MaterialInstance::Create(Material::Create(outlineShader));
        sData.OutlineMaterial->ModifyFlags(MaterialFlag::DepthTest, false);

        // Collider
        auto colliderShader = Shader::Create("Assets/Shaders/Collider");
        sData.ColliderMaterial = MaterialInstance::Create(Material::Create(colliderShader));
        sData.ColliderMaterial->ModifyFlags(MaterialFlag::DepthTest, false);

        auto outlineAnimShader = Shader::Create("Assets/Shaders/OutlineAnim");
        sData.OutlineAnimMaterial = MaterialInstance::Create(Material::Create(outlineAnimShader));
        sData.OutlineAnimMaterial->ModifyFlags(MaterialFlag::DepthTest, false);

        // Shadow Map
        sData.ShadowMapShader = Shader::Create("Assets/Shaders/ShadowMap");
        sData.ShadowMapAnimShader = Shader::Create("Assets/Shaders/ShadowMapAnim");

        FrameBufferSpecification shadowMapFramebufferSpec;
        shadowMapFramebufferSpec.Width = 4096;
        shadowMapFramebufferSpec.Height = 4096;
        shadowMapFramebufferSpec.Attachments = { FrameBufferTextureFormat::DEPTH32F };
        shadowMapFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
        shadowMapFramebufferSpec.Resizable = true;

        // 4 cascades
        for (int i = 0; i < 4; ++i)
        {
            RenderPassSpecification shadowMapRenderPassSpec;
            shadowMapRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(shadowMapFramebufferSpec);
            sData.ShadowMapRenderPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
        }

        Renderer::Submit([]()
            {
                glGenSamplers(1, &sData.ShadowMapSampler);

                glSamplerParameteri(sData.ShadowMapSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glSamplerParameteri(sData.ShadowMapSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glSamplerParameteri(sData.ShadowMapSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glSamplerParameteri(sData.ShadowMapSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            });
    }

    void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
    {
        sData.GeoPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
        sData.CompositePass->GetSpecification().TargetFrameBuffer->Resize(width, height);
    }

    void SceneRenderer::BeginScene(const Scene* scene, const SceneRendererCamera& camera)
    {
        NR_CORE_ASSERT(!sData.ActiveScene, "");

        sData.ActiveScene = scene;

        sData.SceneData.SceneCamera = camera;
        sData.SceneData.SkyboxMaterial = scene->mSkyboxMaterial;
        sData.SceneData.SceneEnvironment = scene->mEnvironment;
        sData.SceneData.SceneEnvironmentIntensity = scene->mEnvironmentIntensity;
        sData.SceneData.ActiveLight = scene->mLight;
        sData.SceneData.SceneLightEnvironment = scene->mLightEnvironment;
    }

    void SceneRenderer::EndScene()
    {
        NR_CORE_ASSERT(sData.ActiveScene, "");

        sData.ActiveScene = nullptr;

        FlushDrawList();
    }

    void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<MaterialInstance> overrideMaterial)
    {
        sData.DrawList.push_back({ mesh, overrideMaterial, transform });
        sData.ShadowPassDrawList.push_back({ mesh, overrideMaterial, transform });
    }

    void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform)
    {
        sData.SelectedMeshDrawList.push_back({ mesh, nullptr, transform });
        sData.ShadowPassDrawList.push_back({ mesh, nullptr, transform });
    }

    void SceneRenderer::SubmitColliderMesh(const BoxColliderComponent& component, const glm::mat4& parentTransform)
    {
        sData.ColliderDrawList.push_back({ component.DebugMesh, nullptr, glm::translate(parentTransform, component.Offset) });
    }

    void SceneRenderer::SubmitColliderMesh(const SphereColliderComponent& component, const glm::mat4& parentTransform)
    {
        sData.ColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
    }

    void SceneRenderer::SubmitColliderMesh(const CapsuleColliderComponent& component, const glm::mat4& parentTransform)
    {
        sData.ColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
    }

    void SceneRenderer::SubmitColliderMesh(const MeshColliderComponent& component, const glm::mat4& parentTransform)
    {
        for (auto debugMesh : component.ProcessedMeshes)
        {
            sData.ColliderDrawList.push_back({ debugMesh, nullptr, parentTransform });
        }
    }

    static Ref<Shader> equirectangularConversionShader, envFilteringShader, envIrradianceShader;

    std::pair<Ref<TextureCube>, Ref<TextureCube>> SceneRenderer::CreateEnvironmentMap(const std::string& filepath)
    {
        const uint32_t cubemapSize = 2048;
        const uint32_t irradianceMapSize = 32;

        Ref<TextureCube> envUnfiltered = TextureCube::Create(TextureFormat::Float16, cubemapSize, cubemapSize);
        if (!equirectangularConversionShader)
        {
            equirectangularConversionShader = Shader::Create("Assets/Shaders/EquirectangularToCubeMap");
        }

        Ref<Texture2D> envEquirect = Texture2D::Create(filepath);
        NR_CORE_ASSERT(envEquirect->GetFormat() == TextureFormat::Float16, "Texture is not HDR!");

        equirectangularConversionShader->Bind();
        envEquirect->Bind();
        Renderer::Submit([envUnfiltered, cubemapSize, envEquirect]()
            {
                glBindImageTexture(0, envUnfiltered->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
                glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
                glGenerateTextureMipmap(envUnfiltered->GetRendererID());
            });

        if (!envFilteringShader)
        {
            envFilteringShader = Shader::Create("Assets/Shaders/EnvironmentMipFilter");
        }

        Ref<TextureCube> envFiltered = TextureCube::Create(TextureFormat::Float16, cubemapSize, cubemapSize);

        Renderer::Submit([envUnfiltered, envFiltered]()
            {
                glCopyImageSubData(envUnfiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
                    envFiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
                    envFiltered->GetWidth(), envFiltered->GetHeight(), 6);
            });

        envFilteringShader->Bind();
        envUnfiltered->Bind();

        Renderer::Submit([envUnfiltered, envFiltered, cubemapSize]()
            {
                const float deltaRoughness = 1.0f / glm::max((float)(envFiltered->GetMipLevelCount() - 1.0f), 1.0f);
                for (int level = 1, size = cubemapSize / 2; level < envFiltered->GetMipLevelCount(); ++level, size /= 2) // <= ?
                {
                    const GLuint numGroups = glm::max(1, size / 32);
                    glBindImageTexture(0, envFiltered->GetRendererID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
                    glProgramUniform1f(envFilteringShader->GetRendererID(), 0, level * deltaRoughness);
                    glDispatchCompute(numGroups, numGroups, 6);
                }
            });

        if (!envIrradianceShader)
        {
            envIrradianceShader = Shader::Create("Assets/Shaders/EnvironmentIrradiance");
        }

        Ref<TextureCube> irradianceMap = TextureCube::Create(TextureFormat::Float16, irradianceMapSize, irradianceMapSize);
        envIrradianceShader->Bind();
        envFiltered->Bind();
        Renderer::Submit([irradianceMap]()
            {
                glBindImageTexture(0, irradianceMap->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
                glDispatchCompute(irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
                glGenerateTextureMipmap(irradianceMap->GetRendererID());
            });

        return { envFiltered, irradianceMap };
    }

    void SceneRenderer::GeometryPass()
    {
        bool outline = sData.SelectedMeshDrawList.size() > 0;
        bool collider = sData.ColliderDrawList.size() > 0;

        if (outline || collider)
        {
            Renderer::Submit([]()
                {
                    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                });
        }

        Renderer::BeginRenderPass(sData.GeoPass);

        if (outline || collider)
        {
            Renderer::Submit([]()
                {
                    glStencilMask(0);
                });
        }

        auto& sceneCamera = sData.SceneData.SceneCamera;

        auto viewProjection = sceneCamera.CameraObj.GetProjectionMatrix() * sceneCamera.ViewMatrix;
        glm::vec3 cameraPosition = glm::inverse(sData.SceneData.SceneCamera.ViewMatrix)[3];

        // Skybox
        auto skyboxShader = sData.SceneData.SkyboxMaterial->GetShader();
        sData.SceneData.SkyboxMaterial->Set("uInverseVP", glm::inverse(viewProjection));
        Renderer::SubmitFullScreenQuad(sData.SceneData.SkyboxMaterial);

        float aspectRatio = (float)sData.GeoPass->GetSpecification().TargetFrameBuffer->GetWidth() / (float)sData.GeoPass->GetSpecification().TargetFrameBuffer->GetHeight();
        float frustumSize = 2.0f * sceneCamera.Near * glm::tan(sceneCamera.FOV * 0.5f) * aspectRatio;

        // Render entities
        for (auto& dc : sData.DrawList)
        {
            auto baseMaterial = dc.Mesh->GetMaterial();
            baseMaterial->Set("uViewProjectionMatrix", viewProjection);
            baseMaterial->Set("uViewMatrix", sceneCamera.ViewMatrix);
            sData.SceneData.SkyboxMaterial->Set("uSkyIntensity", sData.SceneData.SceneEnvironmentIntensity);
            baseMaterial->Set("uCameraPosition", cameraPosition);
            baseMaterial->Set("uLightMatrixCascade0", sData.LightMatrices[0]);
            baseMaterial->Set("uLightMatrixCascade1", sData.LightMatrices[1]);
            baseMaterial->Set("uLightMatrixCascade2", sData.LightMatrices[2]);
            baseMaterial->Set("uLightMatrixCascade3", sData.LightMatrices[3]);
            baseMaterial->Set("uShowCascades", sData.ShowCascades);
            baseMaterial->Set("uLightView", sData.LightViewMatrix);
            baseMaterial->Set("uCascadeSplits", sData.CascadeSplits);
            baseMaterial->Set("uSoftShadows", sData.SoftShadows);
            baseMaterial->Set("uLightSize", sData.LightSize);
            baseMaterial->Set("uMaxShadowDistance", sData.MaxShadowDistance);
            baseMaterial->Set("uShadowFade", sData.ShadowFade);
            baseMaterial->Set("uCascadeFading", sData.CascadeFading);
            baseMaterial->Set("uCascadeTransitionFade", sData.CascadeTransitionFade);
            baseMaterial->Set("uIBLContribution", sData.SceneData.SceneEnvironmentIntensity);

            baseMaterial->Set("uEnvRadianceTex", sData.SceneData.SceneEnvironment.RadianceMap);
            baseMaterial->Set("uEnvIrradianceTex", sData.SceneData.SceneEnvironment.IrradianceMap);
            baseMaterial->Set("uBRDFLUTTexture", sData.BRDFLUT);

            auto directionalLight = sData.SceneData.SceneLightEnvironment.DirectionalLights[0];
            baseMaterial->Set("uDirectionalLights", directionalLight);

            auto resDec = baseMaterial->FindResourceDeclaration("uShadowMapTexture");
            if (resDec)
            {
                auto reg = resDec->GetRegister();

                auto tex0 = sData.ShadowMapRenderPass[0]->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID();
                auto tex1 = sData.ShadowMapRenderPass[1]->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID();
                auto tex2 = sData.ShadowMapRenderPass[2]->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID();
                auto tex3 = sData.ShadowMapRenderPass[3]->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID();

                Renderer::Submit([reg, tex0, tex1, tex2, tex3]() mutable
                    {
                        // 4 cascades
                        glBindTextureUnit(reg, tex0);
                        glBindSampler(reg++, sData.ShadowMapSampler);

                        glBindTextureUnit(reg, tex1);
                        glBindSampler(reg++, sData.ShadowMapSampler);

                        glBindTextureUnit(reg, tex2);
                        glBindSampler(reg++, sData.ShadowMapSampler);

                        glBindTextureUnit(reg, tex3);
                        glBindSampler(reg++, sData.ShadowMapSampler);
                    });
            }

            Ref<MaterialInstance> overrideMaterial = nullptr;
            Renderer::SubmitMesh(dc.Mesh, dc.Transform, overrideMaterial);
        }

        if (outline || collider)
        {
            Renderer::Submit([]()
                {
                    glStencilFunc(GL_ALWAYS, 1, 0xff);
                    glStencilMask(0xff);
                });
        }

        for (auto& dc : sData.SelectedMeshDrawList)
        {
            auto baseMaterial = dc.Mesh->GetMaterial();
            baseMaterial->Set("uViewProjectionMatrix", viewProjection);
            baseMaterial->Set("uViewMatrix", sceneCamera.ViewMatrix);
            baseMaterial->Set("uCameraPosition", cameraPosition);
            baseMaterial->Set("uCascadeSplits", sData.CascadeSplits);
            baseMaterial->Set("uShowCascades", sData.ShowCascades);
            baseMaterial->Set("uSoftShadows", sData.SoftShadows);
            baseMaterial->Set("uLightSize", sData.LightSize);
            baseMaterial->Set("uMaxShadowDistance", sData.MaxShadowDistance);
            baseMaterial->Set("uShadowFade", sData.ShadowFade);
            baseMaterial->Set("uCascadeFading", sData.CascadeFading);
            baseMaterial->Set("uCascadeTransitionFade", sData.CascadeTransitionFade);
            baseMaterial->Set("uIBLContribution", sData.SceneData.SceneEnvironmentIntensity);

            // Environment
            baseMaterial->Set("uEnvRadianceTex", sData.SceneData.SceneEnvironment.RadianceMap);
            baseMaterial->Set("uEnvIrradianceTex", sData.SceneData.SceneEnvironment.IrradianceMap);
            baseMaterial->Set("uBRDFLUTTexture", sData.BRDFLUT);

            baseMaterial->Set("uLightMatrixCascade0", sData.LightMatrices[0]);
            baseMaterial->Set("uLightMatrixCascade1", sData.LightMatrices[1]);
            baseMaterial->Set("uLightMatrixCascade2", sData.LightMatrices[2]);
            baseMaterial->Set("uLightMatrixCascade3", sData.LightMatrices[3]);

            // Set lights
            baseMaterial->Set("uDirectionalLights", sData.SceneData.SceneLightEnvironment.DirectionalLights[0]);

            auto rd = baseMaterial->FindResourceDeclaration("uShadowMapTexture");
            if (rd)
            {
                auto reg = rd->GetRegister();

                auto tex0 = sData.ShadowMapRenderPass[0]->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID();
                auto tex1 = sData.ShadowMapRenderPass[1]->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID();
                auto tex2 = sData.ShadowMapRenderPass[2]->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID();
                auto tex3 = sData.ShadowMapRenderPass[3]->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID();

                Renderer::Submit([reg, tex0, tex1, tex2, tex3]() mutable
                    {
                        // 4 cascades
                        glBindTextureUnit(reg, tex0);
                        glBindSampler(reg++, sData.ShadowMapSampler);

                        glBindTextureUnit(reg, tex1);
                        glBindSampler(reg++, sData.ShadowMapSampler);

                        glBindTextureUnit(reg, tex2);
                        glBindSampler(reg++, sData.ShadowMapSampler);

                        glBindTextureUnit(reg, tex3);
                        glBindSampler(reg++, sData.ShadowMapSampler);
                    });
            }

            Ref<MaterialInstance> overrideMaterial = nullptr;
            Renderer::SubmitMesh(dc.Mesh, dc.Transform, overrideMaterial);
        }

        if (outline)
        {
            Renderer::Submit([]()
                {
                    glStencilFunc(GL_NOTEQUAL, 1, 0xff);
                    glStencilMask(0);

                    glLineWidth(10);
                    glEnable(GL_LINE_SMOOTH);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    glDisable(GL_DEPTH_TEST);
                });

            sData.OutlineMaterial->Set("uViewProjection", viewProjection);
            sData.OutlineAnimMaterial->Set("uViewProjection", viewProjection);
            for (auto& dc : sData.SelectedMeshDrawList)
            {
                Renderer::SubmitMesh(dc.Mesh, dc.Transform, dc.Mesh->IsAnimated() ? sData.OutlineAnimMaterial : sData.OutlineMaterial);
            }

            Renderer::Submit([]()
                {
                    glPointSize(10);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                });

            for (auto& dc : sData.SelectedMeshDrawList)
            {
                Renderer::SubmitMesh(dc.Mesh, dc.Transform, dc.Mesh->IsAnimated() ? sData.OutlineAnimMaterial : sData.OutlineMaterial);
            }

            Renderer::Submit([]()
                {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    glStencilMask(0xff);
                    glStencilFunc(GL_ALWAYS, 1, 0xff);
                    glEnable(GL_DEPTH_TEST);
                });
        }

        if (collider)
        {
            Renderer::Submit([]()
                {
                    glStencilFunc(GL_NOTEQUAL, 1, 0xff);
                    glStencilMask(0);

                    glLineWidth(1);
                    glEnable(GL_LINE_SMOOTH);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    glDisable(GL_DEPTH_TEST);
                });

            sData.ColliderMaterial->Set("uViewProjection", viewProjection);
            for (auto& dc : sData.ColliderDrawList)
            {
                if (dc.Mesh)
                {
                    Renderer::SubmitMesh(dc.Mesh, dc.Transform, sData.ColliderMaterial);
                }
            }

            Renderer::Submit([]()
                {
                    glPointSize(1);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                });

            for (auto& dc : sData.ColliderDrawList)
            {
                if (dc.Mesh)
                {
                    Renderer::SubmitMesh(dc.Mesh, dc.Transform, sData.ColliderMaterial);
                }
            }

            Renderer::Submit([]()
                {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    glStencilMask(0xff);
                    glStencilFunc(GL_ALWAYS, 1, 0xff);
                    glEnable(GL_DEPTH_TEST);
                });
        }

        // Grid
        if (GetOptions().ShowGrid)
        {
            sData.GridMaterial->Set("uViewProjection", viewProjection);
            Renderer::SubmitQuad(sData.GridMaterial, glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f)));
        }

        if (GetOptions().ShowBoundingBoxes)
        {
            Renderer2D::BeginScene(viewProjection);
            for (auto& dc : sData.DrawList)
            {
                Renderer::DrawAABB(dc.Mesh, dc.Transform);
            }
            Renderer2D::EndScene();
        }

        Renderer::EndRenderPass();
    }

    void SceneRenderer::CompositePass()
    {
        auto& compositeBuffer = sData.CompositePass->GetSpecification().TargetFrameBuffer;

        Renderer::BeginRenderPass(sData.CompositePass);
        sData.CompositeShader->Bind();
        sData.CompositeShader->SetFloat("uExposure", sData.SceneData.SceneCamera.CameraObj.GetExposure());
        sData.CompositeShader->SetInt("uTextureSamples", sData.GeoPass->GetSpecification().TargetFrameBuffer->GetSpecification().Samples);
        sData.CompositeShader->SetFloat2("uViewportSize", glm::vec2(compositeBuffer->GetWidth(), compositeBuffer->GetHeight()));
        sData.CompositeShader->SetFloat2("uFocusPoint", sData.FocusPoint);
        sData.CompositeShader->SetInt("uTextureSamples", sData.GeoPass->GetSpecification().TargetFrameBuffer->GetSpecification().Samples);
        sData.CompositeShader->SetFloat("uBloomThreshold", sData.BloomThreshold);
        sData.GeoPass->GetSpecification().TargetFrameBuffer->BindTexture();

        Renderer::Submit([]()
            {
                glBindTextureUnit(1, sData.GeoPass->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID());
            });

        Renderer::SubmitFullScreenQuad(nullptr);
        Renderer::EndRenderPass();
    }

    void SceneRenderer::BloomBlurPass()
    {
        int amount = 10;
        int index = 0;

        int horizontalCounter = 0, verticalCounter = 0;
        for (int i = 0; i < amount; i++)
        {
            index = i % 2;
            Renderer::BeginRenderPass(sData.BloomBlurPass[index]);
            sData.BloomBlurShader->Bind();
            sData.BloomBlurShader->SetBool("uHorizontal", index);
            if (index)
                horizontalCounter++;
            else
                verticalCounter++;
            if (i > 0)
            {
                auto fb = sData.BloomBlurPass[1 - index]->GetSpecification().TargetFrameBuffer;
                fb->BindTexture();
            }
            else
            {
                auto fb = sData.CompositePass->GetSpecification().TargetFrameBuffer;
                auto id = fb->GetColorAttachmentRendererID(1);
                Renderer::Submit([id]()
                    {
                        glBindTextureUnit(0, id);
                    });
            }
            Renderer::SubmitFullScreenQuad(nullptr);
            Renderer::EndRenderPass();
        }

        // Composite bloom
        {
            Renderer::BeginRenderPass(sData.BloomBlendPass);
            sData.BloomBlendShader->Bind();
            sData.BloomBlendShader->SetFloat("uExposure", sData.SceneData.SceneCamera.CameraObj.GetExposure());
            sData.BloomBlendShader->SetBool("uEnableBloom", sData.EnableBloom);

            sData.CompositePass->GetSpecification().TargetFrameBuffer->BindTexture(0);
            sData.BloomBlurPass[index]->GetSpecification().TargetFrameBuffer->BindTexture(1);

            Renderer::SubmitFullScreenQuad(nullptr);
            Renderer::EndRenderPass();
        }
    }

    struct FrustumBounds
    {
        float r, l, b, t, f, n;
    };

    struct CascadeData
    {
        glm::mat4 ViewProj;
        glm::mat4 View;
        float SplitDepth;
    };

    static void CalculateCascades(CascadeData* cascades, const glm::vec3& lightDirection)
    {
        FrustumBounds frustumBounds[3];

        auto& sceneCamera = sData.SceneData.SceneCamera;
        auto viewProjection = sceneCamera.CameraObj.GetProjectionMatrix() * sceneCamera.ViewMatrix;

        constexpr int SHADOW_MAP_CASCADE_COUNT = 4;
        float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

        float nearClip = 0.1f;
        float farClip = 1000.0f;
        float clipRange = farClip - nearClip;

        float minZ = nearClip;
        float maxZ = nearClip + clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        // Calculate split depths based on view camera frustum
        // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i)
        {
            float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = sData.CascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }

        cascadeSplits[3] = 0.3f;

        // Calculate orthographic projection matrix for each cascade
        float lastSplitDist = 0.0;
        for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i)
        {
            float splitDist = cascadeSplits[i];

            glm::vec3 frustumCorners[8] =
            {
                glm::vec3(-1.0f,  1.0f, -1.0f),
                glm::vec3(1.0f,  1.0f, -1.0f),
                glm::vec3(1.0f, -1.0f, -1.0f),
                glm::vec3(-1.0f, -1.0f, -1.0f),
                glm::vec3(-1.0f,  1.0f,  1.0f),
                glm::vec3(1.0f,  1.0f,  1.0f),
                glm::vec3(1.0f, -1.0f,  1.0f),
                glm::vec3(-1.0f, -1.0f,  1.0f),
            };

            // Project frustum corners into world space
            glm::mat4 invCam = glm::inverse(viewProjection);
            for (uint32_t i = 0; i < 8; ++i)
            {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
                frustumCorners[i] = invCorner / invCorner.w;
            }

            for (uint32_t i = 0; i < 4; ++i)
            {
                glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
                frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
                frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
            }

            // Get frustum center
            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for (uint32_t i = 0; i < 8; ++i)
            {
                frustumCenter += frustumCorners[i];
            }

            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for (uint32_t i = 0; i < 8; ++i)
            {
                float distance = glm::length(frustumCorners[i] - frustumCenter);
                radius = glm::max(radius, distance);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;

            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;

            glm::vec3 lightDir = -lightDirection;
            glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + sData.CascadeNearPlaneOffset, maxExtents.z - minExtents.z + sData.CascadeFarPlaneOffset);

            glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
            const float ShadowMapResolution = 4096.0f;
            glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
            glm::vec4 roundedOrigin = glm::round(shadowOrigin);
            glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
            roundOffset = roundOffset * 2.0f / ShadowMapResolution;
            roundOffset.z = 0.0f;
            roundOffset.w = 0.0f;

            lightOrthoMatrix[3] += roundOffset;

            // Store split distance and matrix in cascade
            cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
            cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
            cascades[i].View = lightViewMatrix;

            lastSplitDist = cascadeSplits[i];
        }
    }

    void SceneRenderer::ShadowMapPass()
    {
        auto& directionalLights = sData.SceneData.SceneLightEnvironment.DirectionalLights;
        if (directionalLights[0].Multiplier == 0.0f || !directionalLights[0].CastShadows)
        {
            for (int i = 0; i < 4; ++i)
            {
                // Clear shadow maps
                Renderer::BeginRenderPass(sData.ShadowMapRenderPass[i]);
                Renderer::EndRenderPass();
            }
            return;
        }

        CascadeData cascades[4];
        CalculateCascades(cascades, directionalLights[0].Direction);
        sData.LightViewMatrix = cascades[0].View;

        Renderer::Submit([]()
            {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            });

        for (int i = 0; i < 4; ++i)
        {
            sData.CascadeSplits[i] = cascades[i].SplitDepth;

            Renderer::BeginRenderPass(sData.ShadowMapRenderPass[i]);

            glm::mat4 shadowMapVP = cascades[i].ViewProj;

            static glm::mat4 scaleBiasMatrix = glm::scale(glm::mat4(1.0f), { 0.5f, 0.5f, 0.5f }) * glm::translate(glm::mat4(1.0f), { 1, 1, 1 });
            sData.LightMatrices[i] = scaleBiasMatrix * cascades[i].ViewProj;

            for (auto& dc : sData.ShadowPassDrawList)
            {
                Ref<Shader> shader = dc.Mesh->IsAnimated() ? sData.ShadowMapAnimShader : sData.ShadowMapShader;
                shader->SetMat4("uViewProjection", shadowMapVP);
                Renderer::SubmitMesh(dc.Mesh, dc.Transform, shader);
            }

            Renderer::EndRenderPass();
        }
    }

    void SceneRenderer::FlushDrawList()
    {
        NR_CORE_ASSERT(!sData.ActiveScene, "");

        memset(&sStats, 0, sizeof(SceneRendererStats));
        {
            Renderer::Submit([]()
                {
                    sStats.ShadowPassTimer.Reset();
                });
            ShadowMapPass();
            Renderer::Submit([]
                {
                    sStats.ShadowPass = sStats.ShadowPassTimer.ElapsedMillis();
                });
        }
        {
            Renderer::Submit([]()
                {
                    sStats.GeometryPassTimer.Reset();
                });
            GeometryPass();
            Renderer::Submit([]
                {
                    sStats.GeometryPass = sStats.GeometryPassTimer.ElapsedMillis();
                });
        }
        {
            Renderer::Submit([]()
                {
                    sStats.CompositePassTimer.Reset();
                });

            CompositePass();
            Renderer::Submit([]
                {
                    sStats.CompositePass = sStats.CompositePassTimer.ElapsedMillis();
                });
        }

        sData.DrawList.clear();
        sData.SelectedMeshDrawList.clear();
        sData.ColliderDrawList.clear();
        sData.ShadowPassDrawList.clear();
        sData.SceneData = {};
    }

    void SceneRenderer::SetFocusPoint(const glm::vec2& point)
    {
        sData.FocusPoint = point;
    }

    SceneRendererOptions& SceneRenderer::GetOptions()
    {
        return sData.Options;
    }

    void SceneRenderer::ImGuiRender()
    {
        ImGui::Begin("Scene Renderer");

        if (UI::BeginTreeNode("Shadows"))
        {
            UI::BeginPropertyGrid();
            UI::Property("Soft Shadows", sData.SoftShadows);
            UI::Property("Light Size", sData.LightSize, 0.01f);
            UI::Property("Max Shadow Distance", sData.MaxShadowDistance, 1.0f);
            UI::Property("Shadow Fade", sData.ShadowFade, 5.0f);
            UI::EndPropertyGrid();
            if (UI::BeginTreeNode("Cascade Settings"))
            {
                UI::BeginPropertyGrid();
                UI::Property("Show Cascades", sData.ShowCascades);
                UI::Property("Cascade Fading", sData.CascadeFading);
                UI::Property("Cascade Transition Fade", sData.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
                UI::Property("Cascade Split", sData.CascadeSplitLambda, 0.01f);
                UI::Property("CascadeNearPlaneOffset", sData.CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
                UI::Property("CascadeFarPlaneOffset", sData.CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
                UI::EndPropertyGrid();
                UI::EndTreeNode();
            }
            if (UI::BeginTreeNode("Shadow Map", false))
            {
                static int cascadeIndex = 0;
                auto fb = sData.ShadowMapRenderPass[cascadeIndex]->GetSpecification().TargetFrameBuffer;
                auto id = fb->GetDepthAttachmentRendererID();

                float size = ImGui::GetContentRegionAvail().x; // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
                UI::BeginPropertyGrid();
                UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
                UI::EndPropertyGrid();
                ImGui::Image((ImTextureID)id, { size, size }, { 0, 1 }, { 1, 0 });
                UI::EndTreeNode();
            }

            UI::EndTreeNode();
        }

        if (UI::BeginTreeNode("Bloom"))
        {
            UI::BeginPropertyGrid();
            UI::Property("Bloom", sData.EnableBloom);
            UI::Property("Bloom threshold", sData.BloomThreshold, 0.05f);
            UI::EndPropertyGrid();

            auto fb = sData.BloomBlurPass[0]->GetSpecification().TargetFrameBuffer;
            auto id = fb->GetColorAttachmentRendererID();

            float size = ImGui::GetContentRegionAvail().x; // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
            float w = size;
            float h = w / ((float)fb->GetWidth() / (float)fb->GetHeight());
            ImGui::Image((ImTextureID)id, { w, h }, { 0, 1 }, { 1, 0 });
            UI::EndTreeNode();
        }

        ImGui::End();
    }

    Ref<Texture2D> SceneRenderer::GetFinalColorBuffer()
    {
        NR_CORE_ASSERT(false, "Not implemented");
        return nullptr;
    }

    uint32_t SceneRenderer::GetFinalColorBufferRendererID()
    {
        return sData.CompositePass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID();
    }

    Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
    {
        return sData.CompositePass;
    }

}