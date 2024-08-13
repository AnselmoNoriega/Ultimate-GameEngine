#include "NotRed.h"

class Sandbox : public NR::Application
{
public:
	Sandbox()
	{
		NR_TRACE("Hello!");
	}
};

NR::Application* NR::CreateApplication()
{
	return new Sandbox();
}