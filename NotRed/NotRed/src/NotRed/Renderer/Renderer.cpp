#include "nrpch.h"
#include "Renderer.h"

namespace NR
{
    Renderer::SceneData* Renderer::sSceneData = new Renderer::SceneData;

    void Renderer::BeginScene(OrthographicCamera& camera)
    {
        sSceneData->ViewProjectionMatrix = camera.GetVPMatrix();
    }

    void Renderer::EndScene()
    {
    }

    void Renderer::Submit(
        const std::shared_ptr<Shader>& shader,
        const std::shared_ptr<VertexArray>& vertexArray
    )
    {
        shader->Bind();
        shader->SetUniformMat4("uViewProjection", sSceneData->ViewProjectionMatrix);
        RenderCommand::DrawIndexed(vertexArray);
    }
}