#include <NotRed.h>

class ExampleLayer : public NR::Layer
{
public:
    ExampleLayer()
        :Layer("Example"), mCameraPos(mCamera.GetPosition())
    {
        mVertexArray.reset(NR::VertexArray::Create());

        float vertices[] =
        {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f,
        };

        std::shared_ptr<NR::VertexBuffer> vertexBuffer;
        vertexBuffer.reset(NR::VertexBuffer::Create(vertices, sizeof(vertices)));

        NR::BufferLayout layout =
        {
            {NR::ShaderDataType::Float3, "aPosition"}
        };

        vertexBuffer->SetLayout(layout);
        mVertexArray->AddVertexBuffer(vertexBuffer);

        uint32_t indices[3] = { 0, 1, 2 };
        std::shared_ptr<NR::IndexBuffer>indexBuffer;
        indexBuffer.reset(NR::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
        mVertexArray->SetIndexBuffer(indexBuffer);

        std::string vertexSrc = R"(
        #version 330 core
        
        layout(location = 0) in vec3 aPosition;
        out vec3 vPosition;

        uniform mat4 uViewProjection;
        
        void main()
        {
            vPosition = aPosition;
            gl_Position = uViewProjection * vec4(aPosition, 1.0);
        }
        )";

        std::string fragmentSrc = R"(
        #version 330 core
        
        out vec4 color;
        in vec3 vPosition;
        
        void main()
        {
            color = vec4(vPosition.xy + 0.3, vPosition.z, 1.0);
        }
        )";

        mShader.reset(new NR::Shader(vertexSrc, fragmentSrc));
    }

    void Update() override
    {
        if (NR::Input::IsKeyPressed(NR_KEY_A))
        {
            mCameraPos.x -= mCamMoveSpeed;
            mCamera.SetPosition(mCameraPos);
        }
        else if (NR::Input::IsKeyPressed(NR_KEY_D))
        {
            mCameraPos.x += mCamMoveSpeed;
            mCamera.SetPosition(mCameraPos);
        }

        if (NR::Input::IsKeyPressed(NR_KEY_S))
        {
            mCameraPos.y -= mCamMoveSpeed;
            mCamera.SetPosition(mCameraPos);
        }
        else if (NR::Input::IsKeyPressed(NR_KEY_W))
        {
            mCameraPos.y += mCamMoveSpeed;
            mCamera.SetPosition(mCameraPos);
        }

        if (NR::Input::IsKeyPressed(NR_KEY_RIGHT))
        {
            mCameraRot += mCamRotSpeed;
            mCamera.SetRotation(mCameraRot);
        }
        else if (NR::Input::IsKeyPressed(NR_KEY_LEFT))
        {
            mCameraRot -= mCamRotSpeed;
            mCamera.SetRotation(mCameraRot);
        }

        NR::RenderCommand::SetClearColor({ 0.05f, 0, 0, 1 });

        NR::Renderer::BeginScene(mCamera);

        NR::Renderer::Submit(mShader, mVertexArray);

        NR::Renderer::EndScene();
    }

    void OnEvent(NR::Event& myEvent) override
    {
        //NR_TRACE("{0}", myEvent);
    }

private:
    NR::OrthographicCamera mCamera;
    glm::vec3 mCameraPos;
    float mCameraRot = 0.0f;

    std::shared_ptr<NR::Shader> mShader;
    std::shared_ptr<NR::VertexArray> mVertexArray;
    
    float mCamMoveSpeed = 0.1f;
    float mCamRotSpeed = 1.0f;
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