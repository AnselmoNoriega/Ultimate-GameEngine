#pragma once

#include "NotRed/ImGui/ImGuiLayer.h"
#include "NotRed/Renderer/RenderCommandBuffer.h"

namespace NR
{
	class VKImGuiLayer : public ImGuiLayer
	{
	public:
		VKImGuiLayer();
		VKImGuiLayer(const std::string& name);
		~VKImGuiLayer() override;

		void Begin() override;
		void End() override;

		void Attach() override;
		void Detach() override;
		void ImGuiRender() override;

	private:
		Ref<RenderCommandBuffer> mRenderCommandBuffer;
	};
}