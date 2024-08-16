#include "NotRed.h"

#include "NotRed/ImGui/ImGuiLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glm/gtx/quaternion.hpp>

class EditorLayer : public NR::Layer
{
public:
    EditorLayer()
        : mScene(Scene::Model), mCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 10000.0f))
    {
    }

    virtual ~EditorLayer()
    {
    }

    virtual void Attach() override
    {
        using namespace glm;

        mSimplePBRShader.reset(NR::Shader::Create("Assets/Shaders/SimplePBR"));
        mQuadShader.reset(NR::Shader::Create("Assets/Shaders/Quad"));
        mHDRShader.reset(NR::Shader::Create("Assets/Shaders/HDR"));
        mGridShader.reset(NR::Shader::Create("assets/shaders/Grid"));
        mMesh.reset(new NR::Mesh("Assets/Models/RevolverMagnum/CHRgEgggffN13.fbx"));

        mSphereMesh.reset(new NR::Mesh("Assets/Models/Sphere.fbx"));
        mPlaneMesh.reset(new NR::Mesh("Assets/Models/Plane1m.obj"));

        // Editor
        mCheckerboardTex.reset(NR::Texture2D::Create("Assets/Editor/Checkerboard.tga"));

        // Environment
        mEnvironmentCubeMap.reset(NR::TextureCube::Create("Assets/Textures/Environments/Arches_E_PineTree_Radiance.tga"));
        mEnvironmentIrradiance.reset(NR::TextureCube::Create("Assets/Textures/Environments/Arches_E_PineTree_Irradiance.tga"));
        mBRDFLUT.reset(NR::Texture2D::Create("Assets/Textures/BRDF_LUT.tga"));

        mFramebuffer.reset(NR::FrameBuffer::Create(1280, 720, NR::FrameBufferFormat::RGBA16F));
        mFinalPresentBuffer.reset(NR::FrameBuffer::Create(1280, 720, NR::FrameBufferFormat::RGBA8));

        mPBRMaterial.reset(new NR::Material(mSimplePBRShader));

        float x = -4.0f;
        float roughness = 0.0f;
        for (int i = 0; i < 8; ++i)
        {
            NR::Ref<NR::MaterialInstance> mi(new NR::MaterialInstance(mPBRMaterial));
            mi->Set("uMetalness", 1.0f);
            mi->Set("uRoughness", roughness);
            mi->Set("uModelMatrix", translate(mat4(1.0f), vec3(x, 0.0f, 0.0f)));
            x += 1.1f;
            roughness += 0.15f;
            mMetalSphereMaterialInstances.push_back(mi);
        }

        x = -4.0f;
        roughness = 0.0f;
        for (int i = 0; i < 8; ++i)
        {
            NR::Ref<NR::MaterialInstance> mi(new NR::MaterialInstance(mPBRMaterial));
            mi->Set("uMetalness", 0.0f);
            mi->Set("uRoughness", roughness);
            mi->Set("uModelMatrix", translate(mat4(1.0f), vec3(x, 1.2f, 0.0f)));
            x += 1.1f;
            roughness += 0.15f;
            mDielectricSphereMaterialInstances.push_back(mi);
        }

        // Create Quad
        x = -1;
        float y = -1;
        float width = 2, height = 2;
        struct QuadVertex
        {
            glm::vec3 Position;
            glm::vec2 TexCoord;
        };

        QuadVertex* data = new QuadVertex[4];

        data[0].Position = glm::vec3(x, y, 0);
        data[0].TexCoord = glm::vec2(0, 0);

        data[1].Position = glm::vec3(x + width, y, 0);
        data[1].TexCoord = glm::vec2(1, 0);

        data[2].Position = glm::vec3(x + width, y + height, 0);
        data[2].TexCoord = glm::vec2(1, 1);

        data[3].Position = glm::vec3(x, y + height, 0);
        data[3].TexCoord = glm::vec2(0, 1);

        mVertexBuffer.reset(NR::VertexBuffer::Create());
        mVertexBuffer->SetData(data, 4 * sizeof(QuadVertex));

        uint32_t* indices = new uint32_t[6]{ 0, 1, 2, 2, 3, 0, };
        mIndexBuffer.reset(NR::IndexBuffer::Create());
        mIndexBuffer->SetData(indices, 6 * sizeof(uint32_t));

        mLight.Direction = { -0.5f, -0.5f, 1.0f };
        mLight.Radiance = { 1.0f, 1.0f, 1.0f };
    }

    virtual void Detach() override
    {
    }

    virtual void Update(float dt) override
    {
        using namespace NR;
        using namespace glm;

        mCamera.Update(dt);
        auto viewProjection = mCamera.GetProjectionMatrix() * mCamera.GetViewMatrix();

        mFramebuffer->Bind();
        Renderer::Clear();

        NR::UniformBufferDeclaration<sizeof(mat4), 1> quadShaderUB;
        quadShaderUB.Push("uInverseVP", inverse(viewProjection));
        mQuadShader->UploadUniformBuffer(quadShaderUB);

        mQuadShader->Bind();
        mEnvironmentIrradiance->Bind(0);
        mVertexBuffer->Bind();
        mIndexBuffer->Bind();
        Renderer::DrawIndexed(mIndexBuffer->GetCount(), false);

        mPBRMaterial->Set("uAlbedoColor", mAlbedoInput.Color);
        mPBRMaterial->Set("uMetalness", mMetalnessInput.Value);
        mPBRMaterial->Set("uRoughness", mRoughnessInput.Value);
        mPBRMaterial->Set("uViewProjectionMatrix", viewProjection);
        mPBRMaterial->Set("uModelMatrix", scale(mat4(1.0f), vec3(mMeshScale)));
        mPBRMaterial->Set("lights", mLight);
        mPBRMaterial->Set("uCameraPosition", mCamera.GetPosition());
        mPBRMaterial->Set("uRadiancePrefilter", mRadiancePrefilter ? 1.0f : 0.0f);
        mPBRMaterial->Set("uAlbedoTexToggle", mAlbedoInput.UseTexture ? 1.0f : 0.0f);
        mPBRMaterial->Set("uNormalTexToggle", mNormalInput.UseTexture ? 1.0f : 0.0f);
        mPBRMaterial->Set("uMetalnessTexToggle", mMetalnessInput.UseTexture ? 1.0f : 0.0f);
        mPBRMaterial->Set("uRoughnessTexToggle", mRoughnessInput.UseTexture ? 1.0f : 0.0f);
        mPBRMaterial->Set("uEnvMapRotation", mEnvMapRotation);

#if 0
        // Bind default texture unit
        UploadUniformInt("uTexture", 0);

        // PBR shader textures
        UploadUniformInt("uAlbedoTexture", 1);
        UploadUniformInt("uNormalTexture", 2);
        UploadUniformInt("uMetalnessTexture", 3);
        UploadUniformInt("uRoughnessTexture", 4);

        UploadUniformInt("uEnvRadianceTex", 10);
        UploadUniformInt("uEnvIrradianceTex", 11);

        UploadUniformInt("uBRDFLUTTexture", 15);
#endif
        mPBRMaterial->Set("uEnvRadianceTex", mEnvironmentCubeMap);
        mPBRMaterial->Set("uEnvIrradianceTex", mEnvironmentIrradiance);
        mPBRMaterial->Set("uBRDFLUTTexture", mBRDFLUT);

        if (mAlbedoInput.TextureMap)
        {
            mPBRMaterial->Set("uAlbedoTexture", mAlbedoInput.TextureMap);
        }
        if (mNormalInput.TextureMap)
        {
            mPBRMaterial->Set("uNormalTexture", mNormalInput.TextureMap);
        }
        if (mMetalnessInput.TextureMap)
        {
            mPBRMaterial->Set("uMetalnessTexture", mMetalnessInput.TextureMap);
        }
        if (mRoughnessInput.TextureMap)
        {
            mPBRMaterial->Set("uRoughnessTexture", mRoughnessInput.TextureMap);
        }

        if (mScene == Scene::Spheres)
        {
            // Metals
            for (int i = 0; i < 8; i++)
            {
                mMetalSphereMaterialInstances[i]->Bind();
                mSphereMesh->Render(dt, mSimplePBRShader.get());
            }

            // Dielectrics
            for (int i = 0; i < 8; i++)
            {
                mDielectricSphereMaterialInstances[i]->Bind();
                mSphereMesh->Render(dt, mSimplePBRShader.get());
            }

        }
        else if (mScene == Scene::Model)
        {
            if (mMesh)
            {
                mPBRMaterial->Bind();
                mMesh->Render(dt, mSimplePBRShader.get());
            }
        }

        mGridShader->Bind();
        mGridShader->SetMat4("uMVP", viewProjection* glm::scale(glm::mat4(1.0f), glm::vec3(16.0f)));
        mGridShader->SetFloat("uScale", mGridScale);
        mGridShader->SetFloat("uRes", mGridSize);
        mPlaneMesh->Render(dt, mGridShader.get());

        mFramebuffer->Unbind();

        mFinalPresentBuffer->Bind();
        mHDRShader->Bind();
        mHDRShader->SetFloat("uExposure", mExposure);
        mFramebuffer->BindTexture();
        mVertexBuffer->Bind();
        mIndexBuffer->Bind();
        Renderer::DrawIndexed(mIndexBuffer->GetCount(), false);
        mFinalPresentBuffer->Unbind();
    }

    enum class PropertyFlag
    {
        None = 0, ColorProperty = 1
    };

    void Property(const std::string& name, bool& value)
    {
        ImGui::Text(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        ImGui::Checkbox(id.c_str(), &value);

        ImGui::PopItemWidth();
        ImGui::NextColumn();
    }

    void Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None)
    {
        ImGui::Text(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        ImGui::SliderFloat(id.c_str(), &value, min, max);

        ImGui::PopItemWidth();
        ImGui::NextColumn();
    }

    void Property(const std::string& name, glm::vec3& value, PropertyFlag flags)
    {
        Property(name, value, -1.0f, 1.0f, flags);
    }

    void Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None)
    {
        ImGui::Text(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        if ((int)flags & (int)PropertyFlag::ColorProperty)
        {
            ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
        }
        else
        {
            ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
    }

    void Property(const std::string& name, glm::vec4& value, PropertyFlag flags)
    {
        Property(name, value, -1.0f, 1.0f, flags);
    }

    void Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None)
    {
        ImGui::Text(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        if ((int)flags & (int)PropertyFlag::ColorProperty)
        {
            ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
        }
        else
        {
            ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
    }

    virtual void ImGuiRender() override
    {
        static bool p_open = true;

        static bool opt_fullscreen_persistant = true;
        static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
        bool opt_fullscreen = opt_fullscreen_persistant;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        // When using ImGuiDockNodeFlagsPassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (opt_flags & (ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_NoResize | ImGuiDockNodeFlags_NoDockingInCentralNode))
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &p_open, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Dockspace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
        }

        // Editor Panel ------------------------------------------------------------------------------
        ImGui::Begin("Model");

        ImGui::RadioButton("Spheres", (int*)&mScene, (int)Scene::Spheres);
        ImGui::SameLine();
        ImGui::RadioButton("Model", (int*)&mScene, (int)Scene::Model);

        ImGui::Begin("Environment");

        ImGui::Columns(2);
        ImGui::AlignTextToFramePadding();

        Property("Light Direction", mLight.Direction);
        Property("Light Radiance", mLight.Radiance, PropertyFlag::ColorProperty);
        Property("Light Multiplier", mLightMultiplier, 0.0f, 5.0f);
        Property("Exposure", mExposure, 0.0f, 5.0f);

        Property("Mesh Scale", mMeshScale, 0.0f, 2.0f);

        Property("Radiance Prefiltering", mRadiancePrefilter);
        Property("Env Map Rotation", mEnvMapRotation, -360.0f, 360.0f);

        ImGui::Columns(1);

        ImGui::End();

        ImGui::Separator();

        // Textures ------------------------------------------------------------------------------
        {
            // Albedo
            if (ImGui::CollapsingHeader("Albedo", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                ImGui::Image(mAlbedoInput.TextureMap ? (void*)mAlbedoInput.TextureMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
                ImGui::PopStyleVar();
                if (ImGui::IsItemHovered())
                {
                    if (mAlbedoInput.TextureMap)
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted(mAlbedoInput.TextureMap->GetPath().c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::Image((void*)mAlbedoInput.TextureMap->GetRendererID(), ImVec2(384, 384));
                        ImGui::EndTooltip();
                    }
                    if (ImGui::IsItemClicked())
                    {
                        std::string filename = NR::Application::Get().OpenFile("");
                        if (filename != "")
                        {
                            mAlbedoInput.TextureMap.reset(NR::Texture2D::Create(filename, mAlbedoInput.SRGB));
                        }
                    }
                }
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Checkbox("Use##AlbedoMap", &mAlbedoInput.UseTexture);
                if (ImGui::Checkbox("sRGB##AlbedoMap", &mAlbedoInput.SRGB) && mAlbedoInput.TextureMap)
                {
                    mAlbedoInput.TextureMap.reset(NR::Texture2D::Create(mAlbedoInput.TextureMap->GetPath(), mAlbedoInput.SRGB));
                }
                ImGui::EndGroup();
                ImGui::SameLine();
                ImGui::ColorEdit3("Color##Albedo", glm::value_ptr(mAlbedoInput.Color), ImGuiColorEditFlags_NoInputs);
            }
        }
        {
            // Normals
            if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                ImGui::Image(mNormalInput.TextureMap ? (void*)mNormalInput.TextureMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
                ImGui::PopStyleVar();
                if (ImGui::IsItemHovered())
                {
                    if (mNormalInput.TextureMap)
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted(mNormalInput.TextureMap->GetPath().c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::Image((void*)mNormalInput.TextureMap->GetRendererID(), ImVec2(384, 384));
                        ImGui::EndTooltip();
                    }
                    if (ImGui::IsItemClicked())
                    {
                        std::string filename = NR::Application::Get().OpenFile("");
                        if (filename != "")
                        {
                            mNormalInput.TextureMap.reset(NR::Texture2D::Create(filename));
                        }
                    }
                }
                ImGui::SameLine();
                ImGui::Checkbox("Use##NormalMap", &mNormalInput.UseTexture);
            }
        }
        { 
            // Metalness
            if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                ImGui::Image(mMetalnessInput.TextureMap ? (void*)mMetalnessInput.TextureMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
                ImGui::PopStyleVar();
                if (ImGui::IsItemHovered())
                {
                    if (mMetalnessInput.TextureMap)
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted(mMetalnessInput.TextureMap->GetPath().c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::Image((void*)mMetalnessInput.TextureMap->GetRendererID(), ImVec2(384, 384));
                        ImGui::EndTooltip();
                    }
                    if (ImGui::IsItemClicked())
                    {
                        std::string filename = NR::Application::Get().OpenFile("");
                        if (filename != "")
                        {
                            mMetalnessInput.TextureMap.reset(NR::Texture2D::Create(filename));
                        }
                    }
                }
                ImGui::SameLine();
                ImGui::Checkbox("Use##MetalnessMap", &mMetalnessInput.UseTexture);
                ImGui::SameLine();
                ImGui::SliderFloat("Value##MetalnessInput", &mMetalnessInput.Value, 0.0f, 1.0f);
            }
        }
        {
            // Roughness
            if (ImGui::CollapsingHeader("Roughness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                ImGui::Image(mRoughnessInput.TextureMap ? (void*)mRoughnessInput.TextureMap->GetRendererID() : (void*)mCheckerboardTex->GetRendererID(), ImVec2(64, 64));
                ImGui::PopStyleVar();
                if (ImGui::IsItemHovered())
                {
                    if (mRoughnessInput.TextureMap)
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted(mRoughnessInput.TextureMap->GetPath().c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::Image((void*)mRoughnessInput.TextureMap->GetRendererID(), ImVec2(384, 384));
                        ImGui::EndTooltip();
                    }
                    if (ImGui::IsItemClicked())
                    {
                        std::string filename = NR::Application::Get().OpenFile("");
                        if (filename != "")
                        {
                            mRoughnessInput.TextureMap.reset(NR::Texture2D::Create(filename));
                        }
                    }
                }
                ImGui::SameLine();
                ImGui::Checkbox("Use##RoughnessMap", &mRoughnessInput.UseTexture);
                ImGui::SameLine();
                ImGui::SliderFloat("Value##RoughnessInput", &mRoughnessInput.Value, 0.0f, 1.0f);
            }
        }

        ImGui::Separator();

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");
        auto viewportSize = ImGui::GetContentRegionAvail();
        mFramebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        mFinalPresentBuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        mCamera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 10000.0f));
        ImGui::Image((void*)mFinalPresentBuffer->GetColorAttachmentRendererID(), viewportSize, { 0, 1 }, { 1, 0 });
        ImGui::End();
        ImGui::PopStyleVar();

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Docking"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows, 
                // which we can't undo at the moment without finer window depth/z control.
                //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);
                ImGuiDockNodeFlags ImGuiDockNodeFlags_PassthruDockspace = ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_NoResize | ImGuiDockNodeFlags_NoDockingInCentralNode;
                if (ImGui::MenuItem("Flag: NoSplit", "", (opt_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 opt_flags ^= ImGuiDockNodeFlags_NoSplit;
                if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (opt_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  opt_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
                if (ImGui::MenuItem("Flag: NoResize", "", (opt_flags & ImGuiDockNodeFlags_NoResize) != 0))                opt_flags ^= ImGuiDockNodeFlags_NoResize;
                if (ImGui::MenuItem("Flag: PassthruDockspace", "", (opt_flags & ImGuiDockNodeFlags_PassthruDockspace) != 0))       opt_flags ^= ImGuiDockNodeFlags_PassthruDockspace;
                if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (opt_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          opt_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
                ImGui::Separator();
                if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
                    p_open = false;
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::End();

        if (mMesh)
        {
            mMesh->ImGuiRender();
        }
    }

    virtual void OnEvent(NR::Event& event) override
    {
    }

private:
    NR::Ref<NR::Shader> mSimplePBRShader;
    NR::Scope<NR::Shader> mQuadShader;
    NR::Scope<NR::Shader> mHDRShader;
    NR::Scope<NR::Shader> mGridShader;
    NR::Scope<NR::Mesh> mMesh;
    NR::Scope<NR::Mesh> mSphereMesh, mPlaneMesh;
    NR::Ref<NR::Texture2D> mBRDFLUT;

    NR::Ref<NR::Material> mPBRMaterial;
    std::vector<NR::Ref<NR::MaterialInstance>> mMetalSphereMaterialInstances;
    std::vector<NR::Ref<NR::MaterialInstance>> mDielectricSphereMaterialInstances;

    float mGridScale = 16.025f, mGridSize = 0.025f;
    float mMeshScale = 1.0f;

    struct AlbedoInput
    {
        glm::vec3 Color = { 0.972f, 0.96f, 0.915f };
        NR::Ref<NR::Texture2D> TextureMap;
        bool SRGB = true;
        bool UseTexture = false;
    };
    AlbedoInput mAlbedoInput;

    struct NormalInput
    {
        NR::Ref<NR::Texture2D> TextureMap;
        bool UseTexture = false;
    };
    NormalInput mNormalInput;

    struct MetalnessInput
    {
        float Value = 1.0f;
        NR::Ref<NR::Texture2D> TextureMap;
        bool UseTexture = false;
    };
    MetalnessInput mMetalnessInput;

    struct RoughnessInput
    {
        float Value = 0.5f;
        NR::Ref<NR::Texture2D> TextureMap;
        bool UseTexture = false;
    };
    RoughnessInput mRoughnessInput;

    NR::Ref<NR::FrameBuffer> mFramebuffer, mFinalPresentBuffer;

    NR::Ref<NR::VertexBuffer> mVertexBuffer;
    NR::Ref<NR::IndexBuffer> mIndexBuffer;
    NR::Ref<NR::TextureCube> mEnvironmentCubeMap, mEnvironmentIrradiance;

    NR::Camera mCamera;

    struct Light
    {
        glm::vec3 Direction;
        glm::vec3 Radiance;
    };
    Light mLight;
    float mLightMultiplier = 0.3f;

    // PBR params
    float mExposure = 1.0f;

    bool mRadiancePrefilter = false;

    float mEnvMapRotation = 0.0f;

    enum class Scene : uint32_t
    {
        Spheres, Model
    };
    Scene mScene;

    // Editor resources
    NR::Ref<NR::Texture2D> mCheckerboardTex;
};

class Sandbox : public NR::Application
{
public:
    Sandbox()
    {
        NR_TRACE("Hello!");
    }

    void Init() override
    {
        PushLayer(new EditorLayer());
    }
};

NR::Application* NR::CreateApplication()
{
    return new Sandbox();
}