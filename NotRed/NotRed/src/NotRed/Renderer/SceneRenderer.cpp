#include "nrpch.h"
#include "SceneRenderer.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include "Renderer.h"

#include "Renderer2D.h"

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
            Light ActiveLight;
        } SceneData;

        Ref<Texture2D> BRDFLUT;
        Ref<Shader> CompositeShader;

        Ref<RenderPass> GeoPass;
        Ref<RenderPass> CompositePass;

        struct DrawCommand
        {
            Ref<Mesh> Mesh;
            Ref<MaterialInstance> Material;
            glm::mat4 Transform;
        };
        std::vector<DrawCommand> DrawList;
        std::vector<DrawCommand> SelectedMeshDrawList;

        Ref<MaterialInstance> GridMaterial;
        Ref<MaterialInstance> OutlineMaterial;

        SceneRendererOptions Options;
    };

    static SceneRendererData sData;

    void SceneRenderer::Init()
    {
        FrameBufferSpecification geoFrameBufferSpec;
        geoFrameBufferSpec.Width = 1280;
        geoFrameBufferSpec.Height = 720;
        geoFrameBufferSpec.Format = FrameBufferFormat::RGBA16F;
        geoFrameBufferSpec.Samples = 8;
        geoFrameBufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

        RenderPassSpecification geoRenderPassSpec;
        geoRenderPassSpec.TargetFrameBuffer = NR::FrameBuffer::Create(geoFrameBufferSpec);
        sData.GeoPass = RenderPass::Create(geoRenderPassSpec);

        FrameBufferSpecification compFrameBufferSpec;
        compFrameBufferSpec.Width = 1280;
        compFrameBufferSpec.Height = 720;
        compFrameBufferSpec.Format = FrameBufferFormat::RGBA8;
        compFrameBufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

        RenderPassSpecification compRenderPassSpec;
        compRenderPassSpec.TargetFrameBuffer = NR::FrameBuffer::Create(compFrameBufferSpec);
        sData.CompositePass = RenderPass::Create(compRenderPassSpec);

        sData.CompositeShader = Shader::Create("Assets/Shaders/HDR");
        sData.BRDFLUT = Texture2D::Create("Assets/Textures/BRDF_LUT.tga");

        // Grid
        auto gridShader = Shader::Create("Assets/Shaders/Grid");
        sData.GridMaterial = MaterialInstance::Create(Material::Create(gridShader));
        float gridScale = 16.025f, gridSize = 0.025f;
        sData.GridMaterial->Set("uScale", gridScale);
        sData.GridMaterial->Set("uRes", gridSize);

        // Outline
        auto outlineShader = Shader::Create("Assets/Shaders/Outline");
        sData.OutlineMaterial = MaterialInstance::Create(Material::Create(outlineShader));
        sData.OutlineMaterial->ModifyFlags(MaterialFlag::DepthTest, false);
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
        sData.SceneData.ActiveLight = scene->mLight;
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
    }

    void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform)
    {
        sData.SelectedMeshDrawList.push_back({ mesh, nullptr, transform });
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

        if (outline)
        {
            Renderer::Submit([]()
                {
                    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                });
        }

        Renderer::BeginRenderPass(sData.GeoPass);

        if (outline)
        {
            Renderer::Submit([]()
                {
                    glStencilMask(0);
                });
        }

        auto viewProjection = sData.SceneData.SceneCamera.CameraObj.GetProjectionMatrix() * sData.SceneData.SceneCamera.ViewMatrix;
        glm::vec3 cameraPosition = glm::inverse(sData.SceneData.SceneCamera.ViewMatrix)[3];

        // Skybox
        auto skyboxShader = sData.SceneData.SkyboxMaterial->GetShader();
        sData.SceneData.SkyboxMaterial->Set("uInverseVP", glm::inverse(viewProjection));
        Renderer::SubmitFullScreenQuad(sData.SceneData.SkyboxMaterial);

        // Render entities
        for (auto& dc : sData.DrawList)
        {
            auto baseMaterial = dc.Mesh->GetMaterial();
            baseMaterial->Set("uViewProjectionMatrix", viewProjection);
            baseMaterial->Set("uCameraPosition", cameraPosition);

            baseMaterial->Set("uEnvRadianceTex", sData.SceneData.SceneEnvironment.RadianceMap);
            baseMaterial->Set("uEnvIrradianceTex", sData.SceneData.SceneEnvironment.IrradianceMap);
            baseMaterial->Set("uBRDFLUTTexture", sData.BRDFLUT);

            baseMaterial->Set("lights", sData.SceneData.ActiveLight);

            auto overrideMaterial = nullptr; // dc.Material;
            Renderer::SubmitMesh(dc.Mesh, dc.Transform, overrideMaterial);
        }



        if (outline)
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
            baseMaterial->Set("uCameraPosition", cameraPosition);

            // Environment (TODO: don't do this per mesh)
            baseMaterial->Set("uEnvRadianceTex", sData.SceneData.SceneEnvironment.RadianceMap);
            baseMaterial->Set("uEnvIrradianceTex", sData.SceneData.SceneEnvironment.IrradianceMap);
            baseMaterial->Set("uBRDFLUTTexture", sData.BRDFLUT);

            // Set lights (TODO: move to light environment and don't do per mesh)
            baseMaterial->Set("lights", sData.SceneData.ActiveLight);

            auto overrideMaterial = nullptr; // dc.Material;
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

            // Draw outline here
            sData.OutlineMaterial->Set("uViewProjection", viewProjection);
            for (auto& dc : sData.SelectedMeshDrawList)
            {
                Renderer::SubmitMesh(dc.Mesh, dc.Transform, sData.OutlineMaterial);
            }

            Renderer::Submit([]()
                {
                    glPointSize(10);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                });
            for (auto& dc : sData.SelectedMeshDrawList)
            {
                Renderer::SubmitMesh(dc.Mesh, dc.Transform, sData.OutlineMaterial);
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
        Renderer::BeginRenderPass(sData.CompositePass);
        sData.CompositeShader->Bind();
        sData.CompositeShader->SetFloat("uExposure", sData.SceneData.SceneCamera.CameraObj.GetExposure());
        sData.CompositeShader->SetInt("uTextureSamples", sData.GeoPass->GetSpecification().TargetFrameBuffer->GetSpecification().Samples);
        sData.GeoPass->GetSpecification().TargetFrameBuffer->BindTexture();
        Renderer::SubmitFullScreenQuad(nullptr);
        Renderer::EndRenderPass();
    }

    void SceneRenderer::FlushDrawList()
    {
        NR_CORE_ASSERT(!sData.ActiveScene, "");

        GeometryPass();
        CompositePass();

        sData.DrawList.clear();
        sData.SelectedMeshDrawList.clear();
        sData.SceneData = {};
    }

    SceneRendererOptions& SceneRenderer::GetOptions()
    {
        return sData.Options;
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