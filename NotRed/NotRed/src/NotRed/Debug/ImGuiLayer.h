#pragma once

#include "NotRed/Core/Layer.h"

#include "NotRed/Core/Events/KeyEvent.h"
#include "NotRed/Core/Events/MouseEvent.h"
#include "NotRed/Core/Events/ApplicationEvent.h"

namespace NR
{
	class NOT_RED_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		ImGuiLayer(const std::string& name);
		virtual ~ImGuiLayer();

		void Attach() override;
		void Detach() override;
		void Update() override;
		void OnEvent(Event& event) override;

	private:
		bool OnMouseButtonPressedEvent(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e);
		bool OnMouseMovedEvent(MouseMovedEvent& e);
		bool OnMouseScrolledEvent(MouseScrolledEvent& e);
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnKeyReleasedEvent(KeyReleasedEvent& e);
		bool OnWindowResizeEvent(WindowResizeEvent& e);

	private:
		float mTime = 0.0f;
	};
}