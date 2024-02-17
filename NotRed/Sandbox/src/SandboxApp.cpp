#include <NotRed.h>

class Sandbox : public NR::Application
{
public:
	Sandbox()
	{

	}

	~Sandbox() override
	{

	}
};

NR::Application* NR::CreateApplication()
{
	return new Sandbox();
}