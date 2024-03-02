#include <NotRed.h>

#include "imgui/imgui.h"

#include "Platform/OpenGL/GLShader.h"

class ExampleLayer : public NR::Layer
{
public:
    ExampleLayer()
        :Layer("Example"), mCameraController(1280.0f / 720.0f)
    {
        mVertexArray.reset(NR::VertexArray::Create());

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

    void Update(float deltaTime) override
    {
        mCameraController.Update(deltaTime);

        NR::RenderCommand::SetClearColor({ 0.05f, 0, 0, 1 });

        NR::Renderer::BeginScene(mCameraController.GetCamera());

        mTexture->Bind();
        NR::Renderer::Submit(mShader, mVertexArray);

        NR::Renderer::EndScene();
    }

    void OnEvent(NR::Event& myEvent) override
    {
        mCameraController.OnEvent(myEvent);
    }

    virtual void ImGuiRender() override
    {
       ImGui::Begin("Settings");

      ImGui::End();
    }

private:
    NR::OrthographicCameraController mCameraController;

    NR::Ref<NR::Shader> mShader;
    NR::Ref<NR::VertexArray> mVertexArray;
    NR::Ref<NR::Texture> mTexture;
};

class Sandbox : public NR::Application
{
public:
    Sandbox()
    {
        PushOverlay(new ExampleLayer());
    }

    ~Sandbox() override
    {

    }
};

NR::Application* NR::CreateApplication()
{
    return new Sandbox();
}