#pragma once

namespace NR
{
	class PhysicsSettingsWindow
	{
	public:
		static void ImGuiRender(bool& show);

	private:
		static void RenderWorldSettings();

		static void RenderLayerList();
		static void RenderSelectedLayer();

		static bool Property(const char* label, float& value, float min = -1.0f, float max = 1.0f);
	};
}