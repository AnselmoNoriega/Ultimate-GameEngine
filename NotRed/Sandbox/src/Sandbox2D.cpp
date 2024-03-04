#include "Sandbox2D.h"

#include "imgui/imgui.h"

Sandbox2D::Sandbox2D()
    :Layer("SandBox2D"), mCameraController(1280.0f / 720.0f)
{
}

void Sandbox2D::Attach()
{
}

void Sandbox2D::Detach()
{
}

void Sandbox2D::Update(float deltaTime)
{
    mCameraController.Update(deltaTime);

    NR::RenderCommand::SetClearColor({ 0.05f, 0, 0, 1 });

    NR::Renderer2D::BeginScene(mCameraController.GetCamera());

    NR::Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, {0.8f, 0.2f, 0.3f, 1.0f});

    NR::Renderer2D::EndScene();
}

void Sandbox2D::OnEvent(NR::Event& myEvent)
{
    mCameraController.OnEvent(myEvent);
}

void Sandbox2D::ImGuiRender()
{
    ImGui::Begin("Settings");

    ImGui::End();
}
