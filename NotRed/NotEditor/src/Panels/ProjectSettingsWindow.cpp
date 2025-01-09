#include "ProjectSettingsWindow.h"

#include "NotRed/ImGui/ImGui.h"

#include "NotRed/Project/ProjectSerializer.h"
#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Physics/PhysicsLayer.h"

namespace NR
{
	ImFont* gBoldFont = nullptr;

	ProjectSettingsWindow::ProjectSettingsWindow(const Ref<Project>& project)
		: mProject(project)
	{
		mDefaultScene = AssetManager::GetAsset<Asset>(project->GetConfig().StartScene);
		memset(mNewLayerNameBuffer, 0, 255);
	}

	ProjectSettingsWindow::~ProjectSettingsWindow()
	{
	}

	void ProjectSettingsWindow::ImGuiRender(bool& show)
	{
		if (!show)
		{
			return;
		}

		bool serializeProject = false;

		ImGui::Begin("Project Settings", &show);

		ImGui::PushID("General");
		if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
		{
			UI::BeginPropertyGrid();
			UI::PushItemDisabled();

			UI::Property("Name", mProject->mConfig.Name);
			UI::Property("Asset Directory", mProject->mConfig.AssetDirectory);
			UI::Property("Asset Registry Path", mProject->mConfig.AssetRegistryPath);
			UI::Property("Audio Commands Registry Path", mProject->mConfig.AudioCommandsRegistryPath);
			UI::Property("Mesh Path", mProject->mConfig.MeshPath);
			UI::Property("Mesh Source Path", mProject->mConfig.MeshSourcePath);
			UI::Property("Script Module Path", mProject->mConfig.ScriptModulePath);
			UI::Property("Project Directory", mProject->mConfig.ProjectDirectory);

			UI::PopItemDisabled();

			if (UI::PropertyAssetReference("Startup Scene", mDefaultScene))
			{
				const auto& metadata = AssetManager::GetMetadata(mDefaultScene);
				if (metadata.IsValid())
				{
					mProject->mConfig.StartScene = metadata.FilePath.string();
					serializeProject = true;
				}
			}

			UI::EndPropertyGrid();
		}
		ImGui::PopID();

		ImGui::PushID("Scripting");
		if (ImGui::CollapsingHeader("Scripting", ImGuiTreeNodeFlags_Framed))
		{
			UI::BeginPropertyGrid();
			if (UI::Property("Default Namespace", mProject->mConfig.DefaultNamespace))
			{
				serializeProject = true;
			}
			if (UI::Property("Reload Assembly On Play", mProject->mConfig.ReloadAssemblyOnPlay))
			{
				serializeProject = true;
			}
			UI::EndPropertyGrid();
		}
		ImGui::PopID();

		ImGui::PushID("Physics");
		if (UI::PropertyGridHeader("Physics"))
		{
			UI::BeginPropertyGrid();

			{
				PhysicsSettings& settings = PhysicsManager::GetSettings();

				UI::Property("Fixed Timestep (Default: 0.02)", settings.FixedDeltaTime);
				UI::Property("Gravity (Default: -9.81)", settings.Gravity.y);

				static const char* broadphaseTypeStrings[] = { "Sweep And Prune", "Multi Box Pruning", "Automatic Box Pruning" };
				UI::PropertyDropdown("Broadphase Type", broadphaseTypeStrings, 3, (int*)&settings.BroadphaseAlgorithm);

				if (settings.BroadphaseAlgorithm != BroadphaseType::AutomaticBoxPrune)
				{
					UI::Property("World Bounds (Min)", settings.WorldBoundsMin);
					UI::Property("World Bounds (Max)", settings.WorldBoundsMax);
					UI::PropertySlider("Grid Subdivisions", (int&)settings.WorldBoundsSubdivisions, 1, 10000);
				}

				static const char* frictionTypeStrings[] = { "Patch", "One Directional", "Two Directional" };
				UI::PropertyDropdown("Friction Model", frictionTypeStrings, 3, (int*)&settings.FrictionModel);

				UI::PropertySlider("Solver Iterations", (int&)settings.SolverIterations, 1, 512);
				UI::PropertySlider("Solver Velocity Iterations", (int&)settings.SolverVelocityIterations, 1, 512);

#ifdef NR_DEBUG
				UI::Property("Debug On Play", settings.DebugOnPlay);

				static const char* debugTypeStrings[] = { "Debug To File", "Live Debug" };
				UI::PropertyDropdown("Debug Type", debugTypeStrings, 2, (int*)&settings.DebugType);
#endif
			}

			// Layer List
			{
				if (ImGui::Button("New Layer"))
				{
					ImGui::OpenPopup("NewLayerNamePopup");
				}

				if (ImGui::BeginPopup("NewLayerNamePopup"))
				{
					ImGui::InputText("##LayerNameID", mNewLayerNameBuffer, 255);

					if (ImGui::Button("Add"))
					{
						PhysicsLayerManager::AddLayer(std::string(mNewLayerNameBuffer));
						memset(mNewLayerNameBuffer, 0, 50);
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				uint32_t buttonId = 1024;
				for (const auto& layer : PhysicsLayerManager::GetLayers())
				{
					if (ImGui::Button(layer.Name.c_str()))
					{
						mSelectedLayer = layer.ID;
					}

					if (layer.Name != "Default")
					{
						ImGui::SameLine();
						ImGui::PushID(buttonId++);
						if (ImGui::Button("X"))
						{
							PhysicsLayerManager::RemoveLayer(layer.ID);
							serializeProject = true;
						}
						ImGui::PopID();
					}
				}
			}

			ImGui::NextColumn();
			ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f));
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			// Selected Layer
			{
				static std::string s_IDString = "##";

				if (mSelectedLayer != -1)
				{
					const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(mSelectedLayer);

					for (const auto& layer : PhysicsLayerManager::GetLayers())
					{
						if (layer.ID == mSelectedLayer)
						{
							continue;
						}

						const PhysicsLayer& otherLayerInfo = PhysicsLayerManager::GetLayer(layer.ID);
						bool shouldCollide;

						if (layerInfo.CollidesWith == 0 || otherLayerInfo.CollidesWith == 0)
						{
							shouldCollide = false;
						}
						else
						{
							shouldCollide = layerInfo.CollidesWith & otherLayerInfo.BitValue;
						}

						ImGui::TextUnformatted(otherLayerInfo.Name.c_str());
						ImGui::SameLine();
						if (ImGui::Checkbox((s_IDString + otherLayerInfo.Name).c_str(), &shouldCollide))
						{
							PhysicsLayerManager::SetLayerCollision(mSelectedLayer, layer.ID, shouldCollide);
							serializeProject = true;
						}
					}

					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
					{
						mSelectedLayer = -1;
					}
				}
			}

			UI::EndPropertyGrid();			
			
			ImGui::TreePop();
		}
		ImGui::End();

		if (serializeProject)
		{
			ProjectSerializer serializer(mProject);
			serializer.Serialize(mProject->mConfig.ProjectDirectory + "/" + mProject->mConfig.ProjectFileName);
		}
	}
}
