#pragma once

#include "NotRed/Core/Log.h"
#include "NotRed/Project/Project.h"
#include "NotRed/Scene/Scene.h"

namespace NR
{
	class ProjectSettingsWindow
	{
	public:
		ProjectSettingsWindow(const Ref<Project>& project);
		~ProjectSettingsWindow();
		
		void ImGuiRender(bool& show);

	private:
		void RenderGeneralSettings();
		void RenderScriptingSettings();
		void RenderPhysicsSettings();

	private:
		Ref<Project> mProject;
		AssetHandle mDefaultScene;
		int32_t mSelectedLayer = -1;
		char mNewLayerNameBuffer[255];
	};
}