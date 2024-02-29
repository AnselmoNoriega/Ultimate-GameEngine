#include "nrpch.h"
#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Platform/OpenGL/GLShader.h"

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
        const std::shared_ptr<VertexArray>& vertexArray,
        const glm::mat4& transform
    )
    {
        shader->Bind();
        std::dynamic_pointer_cast<GLShader>(shader)->SetUniformMat4("uViewProjection", sSceneData->ViewProjectionMatrix);
        std::dynamic_pointer_cast<GLShader>(shader)->SetUniformMat4("uTransform", transform);

        RenderCommand::DrawIndexed(vertexArray);
    }
}