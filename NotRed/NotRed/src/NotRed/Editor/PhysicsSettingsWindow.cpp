#include "nrpch.h"
#include "PhysicsSettingsWindow.h"

#include "NotRed/Physics/PhysicsManager.h"
#include "NotRed/Physics/PhysicsLayer.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace NR
{
	static int32_t sSelectedLayer = -1;
	static char sNewLayerNameBuffer[50];

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

		Property("Fixed Timestep (Default: 0.02)", settings.FixedDeltaTime);
		Property("Gravity (Default: -9.81)", settings.Gravity.y);

		const char* broadphaseTypeStrings[] = { "Sweep And Prune", "Multi Box Pruning", "Automatic Box Pruning" };
		const char* currentType = broadphaseTypeStrings[(int)settings.BroadphaseAlgorithm];
		ImGui::TextUnformatted("Broadphase Type");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##BroadphaseTypeSelection", currentType))
		{
			for (int type = 0; type < 3; ++type)
			{
				bool is_selected = (currentType == broadphaseTypeStrings[type]);
				if (ImGui::Selectable(broadphaseTypeStrings[type], is_selected))
				{
					currentType = broadphaseTypeStrings[type];
					settings.BroadphaseAlgorithm = (BroadphaseType)type;
				}
				if (is_selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	void PhysicsSettingsWindow::RenderLayerList()
	{
		if (ImGui::Button("New Layer"))
		{
			ImGui::OpenPopup("NewLayerNamePopup");
		}

		if (ImGui::BeginPopup("NewLayerNamePopup"))
		{
			ImGui::InputText("##LayerNameID", sNewLayerNameBuffer, 50);

			if (ImGui::Button("Add"))
			{
				PhysicsLayerManager::AddLayer(std::string(sNewLayerNameBuffer));
				memset(sNewLayerNameBuffer, 0, 50);
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		uint32_t buttonID = 0;

		for (const auto& layer : PhysicsLayerManager::GetLayers())
		{
			if (ImGui::Button(layer.Name.c_str()))
			{
				sSelectedLayer = layer.ID;
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
		if (sSelectedLayer == -1)
		{
			return;
		}

		const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(sSelectedLayer);

		for (const auto& layer : PhysicsLayerManager::GetLayers())
		{
			if (layer.ID == sSelectedLayer)
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
				PhysicsLayerManager::SetLayerCollision(sSelectedLayer, layer.ID, shouldCollide);
			}
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
		{
			sSelectedLayer = -1;
		}
	}

	bool PhysicsSettingsWindow::Property(const char* label, float& value, float min, float max)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + std::string(label);
		bool changed = ImGui::SliderFloat(id.c_str(), &value, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return changed;
	}

}