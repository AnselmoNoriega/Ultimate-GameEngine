#include "nrpch.h"
#include "Renderer.h"

#include <glad/glad.h>

#include "Shader.h"

namespace NR
{
    Renderer* Renderer::sInstance = new Renderer();
    RendererAPIType RendererAPI::sCurrentRendererAPI = RendererAPIType::OpenGL;

    void Renderer::Init()
    {
        sInstance->mShaderLibrary = std::make_unique<ShaderLibrary>();

        Renderer::Submit([]() { RendererAPI::Init(); });

        Renderer::GetShaderLibrary()->Load("Assets/Shaders/PBR_Static");
        Renderer::GetShaderLibrary()->Load("Assets/Shaders/PBR_Anim");
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
        sInstance->mCommandQueue.Execute();
    }

    void Renderer::IBeginRenderPass(const Ref<RenderPass>& renderPass)
    {
        mActiveRenderPass = renderPass;

        renderPass->GetSpecification().TargetFrameBuffer->Bind();
        const glm::vec4& clearColor = renderPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor;
        Renderer::Submit([=]() {
            RendererAPI::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
            });
    }

    void Renderer::IEndRenderPass()
    {
        NR_CORE_ASSERT(mActiveRenderPass, "No active render pass! Have you called Renderer::EndRenderPass twice?");
        mActiveRenderPass->GetSpecification().TargetFrameBuffer->Unbind();
        mActiveRenderPass = nullptr;
    }

    void Renderer::ISubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform, const Ref<MaterialInstance>& overrideMaterial)
    {
        if (overrideMaterial)
        {
            overrideMaterial->Bind();
        }
        else
        {
            // Bind mesh material here
        }

        mesh->mVertexArray->Bind();

        Renderer::Submit([=]() {
            for (Submesh& submesh : mesh->mSubmeshes)
            {
                if (mesh->mIsAnimated)
                {
                    for (size_t i = 0; i < mesh->mBoneTransforms.size(); i++)
                    {
                        std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
                        mesh->mMeshShader->SetMat4FromRenderThread(uniformName, mesh->mBoneTransforms[i]);
                    }
                }

                glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
            }
            });
    }
}