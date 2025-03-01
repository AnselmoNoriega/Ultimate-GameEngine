#pragma once

#include "NotRed/Core/Log.h"
#include "NotRed/Project/Project.h"
#include "NotRed/Scene/Scene.h"

#include "NotRed/Editor/EditorPanel.h"

namespace NR
{
	class ProjectSettingsWindow : public EditorPanel
	{
	public:
		ProjectSettingsWindow();
		~ProjectSettingsWindow();

		void ImGuiRender(bool& isOpen) override;
		void ProjectChanged(const Ref<Project>& project) override;

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