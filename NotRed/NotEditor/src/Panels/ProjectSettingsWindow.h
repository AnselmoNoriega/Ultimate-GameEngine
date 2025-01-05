#pragma once

namespace NR
{
	class PhysicsSettingsWindow
	{
	public:
		void ImGuiRender(bool& show);

	private:
		void RenderWorldSettings();

		void RenderLayerList();
		void RenderSelectedLayer();
	
	private:
		int32_t mSelectedLayer = -1;
		char mNewLayerNameBuffer[255];
	};
}