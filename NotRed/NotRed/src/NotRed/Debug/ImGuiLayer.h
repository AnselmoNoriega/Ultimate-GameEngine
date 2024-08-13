#pragma once

#include "NotRed/Core/Layer.h"

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
	};
}