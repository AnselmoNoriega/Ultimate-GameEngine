#pragma once

namespace NR
{
	class PhysicsSettingsWindow
	{
	public:
		static void RenderLayerList();
		static void RenderSelectedLayer();

		static void ImGuiRender(bool* show);
	};
}