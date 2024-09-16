#pragma once

#include "NotRed/ImGui/ImGuiLayer.h"

namespace NR
{
	class GLImGuiLayer : public ImGuiLayer
	{
	public:
		GLImGuiLayer();
		GLImGuiLayer(const std::string& name);
		~GLImGuiLayer() override;

		void Begin() override;
		void End() override;

		void Attach() override;
		void Detach() override;
		void ImGuiRender() override;
	};
}