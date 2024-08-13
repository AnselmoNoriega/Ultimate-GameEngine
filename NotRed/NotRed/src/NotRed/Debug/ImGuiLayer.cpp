#include "nrpch.h"
#include "ImGuiLayer.h"

#include "NotRed/Core/Events/KeyEvent.h"

namespace NR
{
	ImGuiLayer::ImGuiLayer()
		: Layer("ImGUI Layer")
	{

	}

	ImGuiLayer::ImGuiLayer(const std::string& name)
		: Layer(name)
	{

	}

	ImGuiLayer::~ImGuiLayer()
	{

	}

	void ImGuiLayer::Attach()
	{

	}

	void ImGuiLayer::Detach()
	{

	}

	void ImGuiLayer::Update()
	{

	}

	void ImGuiLayer::OnEvent(Event& event)
	{
		if (GetName() == "Second Layer" && event.IsInCategory(EventCategoryKeyboard))
		{
			event.Handled = true;
		}
		if (GetName() == "Second Layer")
		{
			NR_INFO("{0}: {1}", GetName(), event);
		}
		else if (GetName() == "ImGUI Layer")
		{
			NR_WARN("{0}: {1}", GetName(), event);
		}

	}
}