#include "nrpch.h"
#include "Renderer.h"

#include <glad/glad.h>

#include "SceneRenderer.h"

#include "Shader.h"

namespace NR
{
    RendererAPIType RendererAPI::sCurrentRendererAPI = RendererAPIType::OpenGL;

    struct RendererData
    {
        Ref<RenderPass> mActiveRenderPass;
        RenderCommandQueue mCommandQueue;
        Scope<ShaderLibrary> mShaderLibrary;
        Ref<VertexArray> mFullscreenQuadVertexArray;
    };

    static RendererData sData;

    void Renderer::Init()
    {
        sData.mShaderLibrary = std::make_unique<ShaderLibrary>();

        Renderer::Submit([]() { RendererAPI::Init(); });

        Renderer::GetShaderLibrary()->Load("Assets/Shaders/PBR_Static");
        Renderer::GetShaderLibrary()->Load("Assets/Shaders/PBR_Anim");

        SceneRenderer::Init();

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

        data[0].Position = glm::vec3(x, y, 0);
        data[0].TexCoord = glm::vec2(0, 0);

        data[1].Position = glm::vec3(x + width, y, 0);
        data[1].TexCoord = glm::vec2(1, 0);

        data[2].Position = glm::vec3(x + width, y + height, 0);
        data[2].TexCoord = glm::vec2(1, 1);

        data[3].Position = glm::vec3(x, y + height, 0);
        data[3].TexCoord = glm::vec2(0, 1);

        sData.mFullscreenQuadVertexArray = VertexArray::Create();
        auto quadVB = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
        quadVB->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float2, "a_TexCoord" }
            });

        uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
        auto quadIB = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

        sData.mFullscreenQuadVertexArray->AddVertexBuffer(quadVB);
        sData.mFullscreenQuadVertexArray->SetIndexBuffer(quadIB);
    }

    const Scope<ShaderLibrary>& Renderer::GetShaderLibrary()
    {
        return sData.mShaderLibrary;
    }

    void Renderer::Clear()
    {
        Renderer::Submit([]() {
            RendererAPI::Clear(0.0f, 0.0f, 0.0f, 1.0f);
            });
    }

    void Renderer::Clear(float r, float g, float b, float a)
    {
        Renderer::Submit([=]() {
            RendererAPI::Clear(r, g, b, a);
            });
    }

    void Renderer::ClearMagenta()
    {
        Clear(1, 0, 1);
    }

    void Renderer::SetClearColor(float r, float g, float b, float a)
    {

    }

    void Renderer::DrawIndexed(uint32_t count, bool depthTestActive)
    {
        Renderer::Submit([=]() {
            RendererAPI::DrawIndexed(count, depthTestActive);
            });
    }

    void Renderer::WaitAndRender()
    {
        sData.mCommandQueue.Execute();
    }

    void Renderer::BeginRenderPass(const Ref<RenderPass>& renderPass)
    {
        NR_CORE_ASSERT(renderPass, "Render pass cannot be null!");

        sData.mActiveRenderPass = renderPass;

        renderPass->GetSpecification().TargetFrameBuffer->Bind();
        const glm::vec4& clearColor = renderPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor;
        Renderer::Submit([=]() {
            RendererAPI::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
            });
    }

    void Renderer::EndRenderPass()
    {
        NR_CORE_ASSERT(sData.mActiveRenderPass, "No active render pass! Have you called Renderer::EndRenderPass twice?");
        sData.mActiveRenderPass->GetSpecification().TargetFrameBuffer->Unbind();
        sData.mActiveRenderPass = nullptr;
    }

    void Renderer::SubmitQuad(const Ref<MaterialInstance>& material, const glm::mat4& transform)
    {
        bool depthTest = true;
        if (material)
        {
            material->Bind();
            depthTest = material->IsFlagActive(MaterialFlag::DepthTest);

            auto shader = material->GetShader();
            shader->SetMat4("uTransform", transform);
        }

        sData.mFullscreenQuadVertexArray->Bind();
        Renderer::DrawIndexed(6, depthTest);
    }

    void Renderer::SubmitFullScreenQuad(const Ref<MaterialInstance>& material)
    {
        bool depthTest = true;
        if (material)
        {
            material->Bind();
            depthTest = material->IsFlagActive(MaterialFlag::DepthTest);
        }

        sData.mFullscreenQuadVertexArray->Bind();
        Renderer::DrawIndexed(6, depthTest);
    }

    void Renderer::SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform, const Ref<MaterialInstance>& overrideMaterial)
    {
        mesh->mVertexArray->Bind();

        const auto& materials = mesh->GetMaterials();
        for (Submesh& submesh : mesh->mSubmeshes)
        {
            auto material = materials[submesh.MaterialIndex];
            auto shader = material->GetShader();
            material->Bind();

            if (mesh->mIsAnimated)
            {
                for (size_t i = 0; i < mesh->mBoneTransforms.size(); ++i)
                {
                    std::string uniformName = std::string("uBoneTransforms[") + std::to_string(i) + std::string("]");
                    mesh->mMeshShader->SetMat4(uniformName, mesh->mBoneTransforms[i]);
                }
            }
            shader->SetMat4("uTransform", transform * submesh.Transform);

            Renderer::Submit([submesh, material]()
                {
                    if (material->IsFlagActive(MaterialFlag::DepthTest))
                    {
                        glEnable(GL_DEPTH_TEST);
                    }
                    else
                    {
                        glDisable(GL_DEPTH_TEST);
                    }

                    glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t)* submesh.BaseIndex), submesh.BaseVertex);
                });
        }
    }

    RenderCommandQueue& Renderer::GetRenderCommandQueue()
    {
        return sData.mCommandQueue;
    }
}