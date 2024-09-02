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

		static bool Property(const char* label, const char** options, int32_t optionCount, int32_t* selected);
		static bool Property(const char* label, float& value, float min = -1.0f, float max = 1.0f);
		static bool Property(const char* label, uint32_t& value, uint32_t min = 0.0f, uint32_t max = 0.0f);
		static bool Property(const char* label, glm::vec3& value, float min = 0.0f, float max = 0.0f);
	};
}