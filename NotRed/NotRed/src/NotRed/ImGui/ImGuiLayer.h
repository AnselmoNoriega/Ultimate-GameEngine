#pragma once

#include "nrpch.h"
#include "NotRed/Core/Layer.h"

namespace NR
{
	class NOT_RED_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		ImGuiLayer(const std::string& name);
		~ImGuiLayer() override;

		void Attach() override;
		void Update() override;
		void Detach() override;

	private:
		float mTime = 0.0f;
	};
}