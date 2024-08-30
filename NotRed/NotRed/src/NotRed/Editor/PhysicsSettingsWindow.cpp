#include "nrpch.h"
#include "PhysicsSettingsWindow.h"

#include "NotRed/Physics/PhysicsLayer.h"

#include "imgui.h"

namespace NR
{
	static int32_t sSelectedLayer = -1;
	static char sNewLayerNameBuffer[50];

	void PhysicsSettingsWindow::ImGuiRender(bool* show)
	{
		if (!(*show))
		{
			return;
		}

		ImGui::Begin("Physics", show);
		ImGui::PushID(0);
		ImGui::Columns(2);

		RenderLayerList();
		ImGui::NextColumn();
		RenderSelectedLayer();

		ImGui::PopID();
		ImGui::End();
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

		uint32_t buttonId = 1;

		for (const auto& layer : PhysicsLayerManager::GetLayers())
		{
			if (ImGui::Button(layer.Name.c_str()))
			{
				sSelectedLayer = layer.ID;
			}

			if (layer.Name != "Default")
			{
				ImGui::SameLine();
				ImGui::PushID(buttonId++);
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
	}

}