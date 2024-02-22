#include <NotRed.h>

class ExampleLayer : public NR::Layer
{
public:
    ExampleLayer()
        :Layer("Example")
    {
    }

    void Update() override
    {

    }

    void OnEvent(NR::Event& myEvent) override
    {
        //NR_TRACE("{0}", myEvent);
    }
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