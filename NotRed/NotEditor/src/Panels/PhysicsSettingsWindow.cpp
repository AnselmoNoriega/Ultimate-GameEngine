#include "nrpch.h"
#include "PhysicsSettingsWindow.h"

#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Physics/PhysicsLayer.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "NotRed/ImGui/ImGui.h"

namespace NR
{
	void PhysicsSettingsWindow::ImGuiRender(bool& show)
	{
		if (!show)
		{
			return;
		}

		ImGui::Begin("Physics", &show);
		ImGui::PushID(0);
		ImGui::Columns(2);
		RenderWorldSettings();
		ImGui::EndColumns();
		ImGui::PopID();

		ImGui::Separator();

		ImGui::PushID(1);
		ImGui::Columns(2);

		RenderLayerList();
		ImGui::NextColumn();
		RenderSelectedLayer();

		ImGui::EndColumns();
		ImGui::PopID();
		ImGui::End();
	}

	void PhysicsSettingsWindow::RenderWorldSettings()
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
			UI::PropertySlider("Grid Subdivisions", (int&)settings.WorldBoundsSubdivisions, 1.0F, 10000.0F);
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

	void PhysicsSettingsWindow::RenderLayerList()
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

		uint32_t buttonID = 0;

		for (const auto& layer : PhysicsLayerManager::GetLayers())
		{
			if (ImGui::Button(layer.Name.c_str()))
			{
				mSelectedLayer = layer.ID;
			}

			if (layer.Name != "Default")
			{
				ImGui::SameLine();
				ImGui::PushID(buttonID++);
				if (ImGui::Button("X"))
				{
					PhysicsLayerManager::RemoveLayer(layer.ID);
				}
				ImGui::PopID();
			}
		}
	}

	static std::string sIDString = "##";

	void PhysicsSettingsWindow::RenderSelectedLayer()
	{
		if (mSelectedLayer == -1)
		{
			return;
		}

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
			if (ImGui::Checkbox((sIDString + otherLayerInfo.Name).c_str(), &shouldCollide))
			{
				PhysicsLayerManager::SetLayerCollision(mSelectedLayer, layer.ID, shouldCollide);
			}
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
		{
			mSelectedLayer = -1;
		}
	}
}