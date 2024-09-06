#include "nrpch.h"
#include "ObjectsPanel.h"

#include "NotRed/Util/DragDropData.h"
#include "NotRed/ImGui/ImGui.h"

namespace NR
{
	ObjectsPanel::ObjectsPanel()
	{
		mCubeImage = Texture2D::Create("Assets/Editor/asset.png");
	}

	void ObjectsPanel::ImGuiRender()
	{
		ImGui::Begin("Objects", NULL, ImGuiWindowFlags_None);
		{
			char buff[100] = { 0 };
			const char* inputText;
			const char* inputHint;
			inputText = "";
			inputHint = "Type To Search";
			ImGui::PushItemWidth(ImGui::GetWindowWidth() - 20);
			ImGui::InputTextWithHint(inputText, inputHint, buff, 100);

			ImGui::BeginChild("##objects_window");
			{
				ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(30, 30));
				ImGui::SameLine();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
				ImGui::Selectable("Cube");

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(20, 20));
					ImGui::SameLine();

					ImGui::Text("Cube");

					DragDropData data("Mesh", "Assets/Meshes/Default/Cube.fbx", "Cube");
					ImGui::SetDragDropPayload("scene_entity_objectP", &data, sizeof(data));
					ImGui::EndDragDropSource();
				}

				ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(30, 30));
				ImGui::SameLine();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
				ImGui::Selectable("Capsule");

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(20, 20));
					ImGui::SameLine();

					ImGui::Text("Capsule");

					DragDropData data("Mesh", "Assets/Meshes/Default/Capsule.fbx", "Capsule");
					ImGui::SetDragDropPayload("scene_entity_objectP", &data, sizeof(data));
					ImGui::EndDragDropSource();
				}

				ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(30, 30));
				ImGui::SameLine();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
				ImGui::Selectable("Sphere");

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(20, 20));
					ImGui::SameLine();

					ImGui::Text("Sphere");
					DragDropData data("Mesh", "Assets/Meshes/Default/Sphere.fbx", "Sphere");
					ImGui::SetDragDropPayload("scene_entity_objectP", &data, sizeof(data));
					ImGui::EndDragDropSource();
				}

				ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(30, 30));
				ImGui::SameLine();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
				ImGui::Selectable("Cylinder");

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(20, 20));
					ImGui::SameLine();

					ImGui::Text("Cylinder");
					DragDropData data("Mesh", "Assets/Meshes/Default/Cylinder.fbx", "Cylinder");
					ImGui::SetDragDropPayload("scene_entity_objectP", &data, sizeof(data));
					ImGui::EndDragDropSource();
				}

				ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(30, 30));
				ImGui::SameLine();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
				ImGui::Selectable("Torus");

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(20, 20));
					ImGui::SameLine();

					ImGui::Text("Torus");
					DragDropData data("Mesh", "Assets/Meshes/Default/Torus.fbx", "Torus");
					ImGui::SetDragDropPayload("scene_entity_objectP", &data, sizeof(data));
					ImGui::EndDragDropSource();
				}


				ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(30, 30));
				ImGui::SameLine();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
				ImGui::Selectable("Plane");

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(20, 20));
					ImGui::SameLine();

					ImGui::Text("Plane");
					DragDropData data("Mesh", "Assets/Meshes/Default/Plane.fbx", "Plane");
					ImGui::SetDragDropPayload("scene_entity_objectP", &data, sizeof(data));
					ImGui::EndDragDropSource();
				}

				ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(30, 30));
				ImGui::SameLine();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
				ImGui::Selectable("Cone");

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(20, 20));
					ImGui::SameLine();

					ImGui::Text("Cone");
					DragDropData data("Mesh", "Assets/Meshes/Default/Cone.fbx", "Cone");
					ImGui::SetDragDropPayload("scene_entity_objectP", &data, sizeof(data));
					ImGui::EndDragDropSource();
				}

				ImGui::EndChild();
			}
		}

		ImGui::End();
	}

}