#include "NotRed.h"

#include "NotRed/ImGui/ImGuiLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

class EditorLayer : public NR::Layer
{
public:
    EditorLayer()
        : mClearColor{ 0.1f, 0.1f, 0.1f, 1.0f }, mScene(Scene::Spheres),
        mCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 10000.0f))
    {
    }

    virtual ~EditorLayer()
    {
    }

    virtual void Attach() override
    {
        mSimplePBRShader.reset(NR::Shader::Create("Assets/Shaders/SimplePBR"));
        mQuadShader.reset(NR::Shader::Create("Assets/Shaders/Quad"));
        mHDRShader.reset(NR::Shader::Create("Assets/Shaders/HDR"));
        mMesh.reset(new NR::Mesh("Assets/Meshes/ZIS101Sport.fbx"));
        mSphereMesh.reset(new NR::Mesh("Assets/Models/Sphere.fbx"));

        // Editor
        mCheckerboardTex.reset(NR::Texture2D::Create("Assets/Editor/Checkerboard.tga"));

        // Environment
        mEnvironmentCubeMap.reset(NR::TextureCube::Create("Assets/Textures/Environments/Arches_E_PineTree_Radiance.tga"));
        mEnvironmentIrradiance.reset(NR::TextureCube::Create("Assets/Textures/Environments/Arches_E_PineTree_Irradiance.tga"));
        mBRDFLUT.reset(NR::Texture2D::Create("Assets/Textures/BRDF_LUT.tga"));

        mFramebuffer.reset(NR::FrameBuffer::Create(1280, 720, NR::FrameBufferFormat::RGBA16F));
        mFinalPresentBuffer.reset(NR::FrameBuffer::Create(1280, 720, NR::FrameBufferFormat::RGBA8));

        // Create Quad
        float x = -1, y = -1;
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
        mIndexBuffer->SetData(indices, 6 * sizeof(unsigned int));

        mLight.Direction = { -0.5f, -0.5f, 1.0f };
        mLight.Radiance = { 1.0f, 1.0f, 1.0f };
    }

    virtual void Detach() override
    {
    }

    virtual void Update() override
    {
        using namespace NR;
        using namespace glm;

        mCamera.Update();
        auto viewProjection = mCamera.GetProjectionMatrix() * mCamera.GetViewMatrix();

        mFramebuffer->Bind();
        Renderer::Clear(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);

        NR::UniformBufferDeclaration<sizeof(mat4), 1> quadShaderUB;
        quadShaderUB.Push("uInverseVP", inverse(viewProjection));
        mQuadShader->UploadUniformBuffer(quadShaderUB);

        mQuadShader->Bind();
        mEnvironmentIrradiance->Bind(0);
        mVertexBuffer->Bind();
        mIndexBuffer->Bind();
        Renderer::DrawIndexed(mIndexBuffer->GetCount(), false);

        NR::UniformBufferDeclaration<sizeof(mat4) * 2 + sizeof(vec3) * 4 + sizeof(float) * 8, 14> simplePbrShaderUB;
        simplePbrShaderUB.Push("uViewProjectionMatrix", viewProjection);
        simplePbrShaderUB.Push("uModelMatrix", mat4(1.0f));
        simplePbrShaderUB.Push("uAlbedoColor", mAlbedoInput.Color);
        simplePbrShaderUB.Push("uMetalness", mMetalnessInput.Value);
        simplePbrShaderUB.Push("uRoughness", mRoughnessInput.Value);
        simplePbrShaderUB.Push("lights.Direction", mLight.Direction);
        simplePbrShaderUB.Push("lights.Radiance", mLight.Radiance * mLightMultiplier);
        simplePbrShaderUB.Push("uCameraPosition", mCamera.GetPosition());
        simplePbrShaderUB.Push("uRadiancePrefilter", mRadiancePrefilter ? 1.0f : 0.0f);
        simplePbrShaderUB.Push("uAlbedoTexToggle", mAlbedoInput.UseTexture ? 1.0f : 0.0f);
        simplePbrShaderUB.Push("uNormalTexToggle", mNormalInput.UseTexture ? 1.0f : 0.0f);
        simplePbrShaderUB.Push("uMetalnessTexToggle", mMetalnessInput.UseTexture ? 1.0f : 0.0f);
        simplePbrShaderUB.Push("uRoughnessTexToggle", mRoughnessInput.UseTexture ? 1.0f : 0.0f);
        simplePbrShaderUB.Push("uEnvMapRotation", mEnvMapRotation);
        mSimplePBRShader->UploadUniformBuffer(simplePbrShaderUB);

        mEnvironmentCubeMap->Bind(10);
        mEnvironmentIrradiance->Bind(11);
        mBRDFLUT->Bind(15);

        mSimplePBRShader->Bind();
        if (mAlbedoInput.TextureMap)
        {
            mAlbedoInput.TextureMap->Bind(1);
        }
        if (mNormalInput.TextureMap)
        {
            mNormalInput.TextureMap->Bind(2);
        }
        if (mMetalnessInput.TextureMap)
        {
            mMetalnessInput.TextureMap->Bind(3);
        }
        if (mRoughnessInput.TextureMap)
        {
            mRoughnessInput.TextureMap->Bind(4);
        }

        if (mScene == Scene::Spheres)
        {
            // Metals
            float roughness = 0.0f;
            float x = -88.0f;
            for (int i = 0; i < 8; i++)
            {
                mSimplePBRShader->SetMat4("uModelMatrix", translate(mat4(1.0f), vec3(x, 0.0f, 0.0f)));
                mSimplePBRShader->SetFloat("uRoughness", roughness);
                mSimplePBRShader->SetFloat("uMetalness", 1.0f);
                mSphereMesh->Render();

                roughness += 0.15f;
                x += 22.0f;
            }

            // Dielectrics
            roughness = 0.0f;
            x = -88.0f;
            for (int i = 0; i < 8; i++)
            {
                mSimplePBRShader->SetMat4("uModelMatrix", translate(mat4(1.0f), vec3(x, 22.0f, 0.0f)));
                mSimplePBRShader->SetFloat("uRoughness", roughness);
                mSimplePBRShader->SetFloat("uMetalness", 0.0f);
                mSphereMesh->Render();

                roughness += 0.15f;
                x += 22.0f;
            }

        }
        else if (mScene == Scene::Model)
        {
            mMesh->Render();
        }

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
        ImGui::Begin("Settings");

        ImGui::RadioButton("Spheres", (int*)&mScene, (int)Scene::Spheres);
        ImGui::SameLine();
        ImGui::RadioButton("Model", (int*)&mScene, (int)Scene::Model);

        ImGui::ColorEdit4("Clear Color", mClearColor);

        ImGui::SliderFloat3("Light Dir", glm::value_ptr(mLight.Direction), -1, 1);
        ImGui::ColorEdit3("Light Radiance", glm::value_ptr(mLight.Radiance));
        ImGui::SliderFloat("Light Multiplier", &mLightMultiplier, 0.0f, 5.0f);
        ImGui::SliderFloat("Exposure", &mExposure, 0.0f, 10.0f);
        auto cameraForward = mCamera.GetForwardDirection();
        ImGui::Text("Camera Forward: %.2f, %.2f, %.2f", cameraForward.x, cameraForward.y, cameraForward.z);

        ImGui::Separator();

        ImGui::Text("Shader Parameters");
        ImGui::Checkbox("Radiance Prefiltering", &mRadiancePrefilter);
        ImGui::SliderFloat("Env Map Rotation", &mEnvMapRotation, -360.0f, 360.0f);

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
    }

    virtual void OnEvent(NR::Event& event) override
    {
    }

private:
    float mClearColor[4];

    std::unique_ptr<NR::Shader> mShader;
    std::unique_ptr<NR::Shader> mPBRShader;
    std::unique_ptr<NR::Shader> mSimplePBRShader;
    std::unique_ptr<NR::Shader> mQuadShader;
    std::unique_ptr<NR::Shader> mHDRShader;
    std::unique_ptr<NR::Mesh> mMesh;
    std::unique_ptr<NR::Mesh> mSphereMesh;
    std::unique_ptr<NR::Texture2D> mBRDFLUT;

    struct AlbedoInput
    {
        glm::vec3 Color = { 0.972f, 0.96f, 0.915f };
        std::unique_ptr<NR::Texture2D> TextureMap;
        bool SRGB = true;
        bool UseTexture = false;
    };
    AlbedoInput mAlbedoInput;

    struct NormalInput
    {
        std::unique_ptr<NR::Texture2D> TextureMap;
        bool UseTexture = false;
    };
    NormalInput mNormalInput;

    struct MetalnessInput
    {
        float Value = 1.0f;
        std::unique_ptr<NR::Texture2D> TextureMap;
        bool UseTexture = false;
    };
    MetalnessInput mMetalnessInput;

    struct RoughnessInput
    {
        float Value = 0.5f;
        std::unique_ptr<NR::Texture2D> TextureMap;
        bool UseTexture = false;
    };
    RoughnessInput mRoughnessInput;

    std::unique_ptr<NR::FrameBuffer> mFramebuffer, mFinalPresentBuffer;

    std::unique_ptr<NR::VertexBuffer> mVertexBuffer;
    std::unique_ptr<NR::IndexBuffer> mIndexBuffer;
    std::unique_ptr<NR::TextureCube> mEnvironmentCubeMap, mEnvironmentIrradiance;

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
    std::unique_ptr<NR::Texture2D> mCheckerboardTex;
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