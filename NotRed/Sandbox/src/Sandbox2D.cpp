#include "Sandbox2D.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"

Sandbox2D::Sandbox2D()
    :Layer("SandBox2D"), mCameraController(1280.0f / 720.0f)
{
}

void Sandbox2D::Attach()
{
    mTexture = NR::Texture2D::Create("Assets/Textures/Image_Two.png");
}

void Sandbox2D::Detach()
{
}

void Sandbox2D::Update(float deltaTime)
{
    NR_PROFILE_FUNCTION();

    {
        NR_PROFILE_SCOPE("Camera Update");
        mCameraController.Update(deltaTime);
    }

    {
        NR_PROFILE_SCOPE("Render Start");
        NR::RenderCommand::SetClearColor({ 0.05f, 0, 0, 1 });
        NR::RenderCommand::Clear();
    }

    {
        NR_PROFILE_SCOPE("Rendering");
        NR::Renderer2D::BeginScene(mCameraController.GetCamera());

        NR::Renderer2D::DrawQuad(objPositions[0], { 1.0f, 1.0f }, mSquareColor);
        NR::Renderer2D::DrawQuad(objPositions[1], { 1.0f, 1.0f }, mTexture);

        NR::Renderer2D::EndScene();
    }
}

void Sandbox2D::OnEvent(NR::Event& myEvent)
{
    mCameraController.OnEvent(myEvent);
}

void Sandbox2D::ImGuiRender()
{
    NR_PROFILE_FUNCTION();

    ImGui::Begin("Settings");

    ImGui::ColorEdit4("Square Color", glm::value_ptr(mSquareColor));
    ImGui::DragFloat2("Obj pos 1", &objPositions[0].x, 0.1f, -10.0f, 10.0f);
    ImGui::DragFloat2("Obj pos 2", &objPositions[1].x, 0.1f, -10.0f, 10.0f);

    ImGui::End();
}
