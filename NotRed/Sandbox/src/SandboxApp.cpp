#include <NotRed.h>

#include "imgui/imgui.h"

#include "Platform/OpenGL/GLShader.h"

class ExampleLayer : public NR::Layer
{
public:
    ExampleLayer()
        :Layer("Example"), mCameraPos(mCamera.GetPosition())
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

        std::string vertexSrc = R"(
        #version 330 core
        
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec2 aTexCoord;

        uniform mat4 uViewProjection;
        uniform mat4 uTransform;

        out vec2 vTexCoord;
        
        void main()
        {
            vTexCoord = aTexCoord;
            gl_Position = uViewProjection * uTransform * vec4(aPosition, 1.0);
        }
        )";

        std::string fragmentSrc = R"(
        #version 330 core

        uniform sampler2D uTexture;
        
        in vec2 vTexCoord;

        out vec4 color;
        
        void main()
        {
            color = texture(uTexture, vTexCoord);
        }
        )";

        mShader.reset(NR::Shader::Create(vertexSrc, fragmentSrc));
        mTexture = NR::Texture2D::Create("Assets/Textures/Image_Two.png");

        std::dynamic_pointer_cast<NR::GLShader>(mShader)->Bind();
        std::dynamic_pointer_cast<NR::GLShader>(mShader)->SetUniformInt("uTexture", 0);
    }

    void Update(float deltaTime) override
    {
        if (NR::Input::IsKeyPressed(NR_KEY_A))
        {
            mCameraPos.x -= mCamMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPos);
        }
        else if (NR::Input::IsKeyPressed(NR_KEY_D))
        {
            mCameraPos.x += mCamMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPos);
        }

        if (NR::Input::IsKeyPressed(NR_KEY_S))
        {
            mCameraPos.y -= mCamMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPos);
        }
        else if (NR::Input::IsKeyPressed(NR_KEY_W))
        {
            mCameraPos.y += mCamMoveSpeed * deltaTime;
            mCamera.SetPosition(mCameraPos);
        }

        if (NR::Input::IsKeyPressed(NR_KEY_RIGHT))
        {
            mCameraRot += mCamRotSpeed * deltaTime;
            mCamera.SetRotation(mCameraRot);
        }
        else if (NR::Input::IsKeyPressed(NR_KEY_LEFT))
        {
            mCameraRot -= mCamRotSpeed * deltaTime;
            mCamera.SetRotation(mCameraRot);
        }

        NR::RenderCommand::SetClearColor({ 0.05f, 0, 0, 1 });

        NR::Renderer::BeginScene(mCamera);

        mTexture->Bind();
        NR::Renderer::Submit(mShader, mVertexArray);

        NR::Renderer::EndScene();
    }

    void OnEvent(NR::Event& myEvent) override
    {
        //NR_TRACE("{0}", myEvent);
    }

    virtual void ImGuiRender() override
    {
       ImGui::Begin("Settings");

      ImGui::End();
    }

private:
    NR::OrthographicCamera mCamera;
    glm::vec3 mCameraPos;
    float mCameraRot = 0.0f;

    NR::Ref<NR::Shader> mShader;
    NR::Ref<NR::VertexArray> mVertexArray;
    NR::Ref<NR::Texture> mTexture;
    
    float mCamMoveSpeed = 0.8f;
    float mCamRotSpeed = 15.0f;
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