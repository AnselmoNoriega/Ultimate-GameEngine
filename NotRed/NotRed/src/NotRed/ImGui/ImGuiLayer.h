#pragma once

#include "nrpch.h"
#include "NotRed/Core/Layer.h"

namespace NR
{
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		ImGuiLayer(const std::string& name);
		~ImGuiLayer() override;

		void Begin();
		void End();

		void Attach() override;
		void ImGuiRender() override;
		void Detach() override;

		void SetDarkThemeColors();

	private:
		float mTime = 0.0f;
	};
}