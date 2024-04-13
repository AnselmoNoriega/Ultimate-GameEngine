#include "Sandbox2D.h"

#include <glm/gtc/type_ptr.hpp>

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

}

void Sandbox2D::OnEvent(NR::Event& myEvent)
{
    mCameraController.OnEvent(myEvent);
}

void Sandbox2D::ImGuiRender()
{

}
