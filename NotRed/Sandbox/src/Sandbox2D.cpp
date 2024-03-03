#include "Sandbox2D.h"

#include "imgui/imgui.h"
#include "Platform/OpenGL/GLShader.h"

Sandbox2D::Sandbox2D()
    :Layer("SandBox2D"), mCameraController(1280.0f / 720.0f)
{
}

void Sandbox2D::Attach()
{
    mVertexArray = NR::VertexArray::Create();

    float vertices[5 * 4] =
    {
        -0.5f, -0.5f, 0.0f,    0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,    1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,    1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,    0.0f, 1.0f
    };

    NR::Ref<NR::VertexBuffer> vertexBuffer;
    vertexBuffer.reset(NR::VertexBuffer::Create(vertices, sizeof(vertices)));

    NR::BufferLayout layout =
    {
        {NR::ShaderDataType::Float3, "aPosition"},
        {NR::ShaderDataType::Float2, "aTexCoord"}
    };

    vertexBuffer->SetLayout(layout);
    mVertexArray->AddVertexBuffer(vertexBuffer);

    uint32_t indices[2 * 3] = { 0, 1, 2, 2, 3, 0 };
    NR::Ref<NR::IndexBuffer>indexBuffer;
    indexBuffer.reset(NR::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
    mVertexArray->SetIndexBuffer(indexBuffer);

    mShader = NR::Shader::Create("Assets/Shaders/Texture");
    mTexture = NR::Texture2D::Create("Assets/Textures/Image_Two.png");

    std::dynamic_pointer_cast<NR::GLShader>(mShader)->Bind();
    std::dynamic_pointer_cast<NR::GLShader>(mShader)->SetUniformInt("uTexture", 0);
}

void Sandbox2D::Detach()
{
}

void Sandbox2D::Update(float deltaTime)
{
    mCameraController.Update(deltaTime);

    NR::RenderCommand::SetClearColor({ 0.05f, 0, 0, 1 });

    NR::Renderer::BeginScene(mCameraController.GetCamera());

    mTexture->Bind();
    NR::Renderer::Submit(mShader, mVertexArray);

    NR::Renderer::EndScene();
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
