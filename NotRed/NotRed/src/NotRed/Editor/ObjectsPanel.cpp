#include "nrpch.h"
#include "ObjectsPanel.h"

#include "NotRed/ImGui/ImGui.h"

namespace NR
{
    ObjectsPanel::ObjectsPanel()
    {
        mCubeImage = Texture2D::Create("Assets/Editor/asset.png");
    }

    void ObjectsPanel::DrawObject(const char* label, AssetHandle handle)
    {
        ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(30, 30));
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::Selectable(label);

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::Image((ImTextureID)mCubeImage->GetRendererID(), ImVec2(20, 20));
            ImGui::SameLine();

            ImGui::Text(label);

            ImGui::SetDragDropPayload("asset_payload", &handle, sizeof(AssetHandle));
            ImGui::EndDragDropSource();
        }

    }

	void ObjectsPanel::ImGuiRender()
	{
		static const AssetHandle CubeHandle = AssetManager::GetAssetIDForFile("Assets/Meshes/Default/Cube.fbx");
		static const AssetHandle CapsuleHandle = AssetManager::GetAssetIDForFile("Assets/Meshes/Default/Capsule.fbx");
		static const AssetHandle SphereHandle = AssetManager::GetAssetIDForFile("Assets/Meshes/Default/Sphere.fbx");
		static const AssetHandle CylinderHandle = AssetManager::GetAssetIDForFile("Assets/Meshes/Default/Cylinder.fbx");
		static const AssetHandle TorusHandle = AssetManager::GetAssetIDForFile("Assets/Meshes/Default/Torus.fbx");
		static const AssetHandle PlaneHandle = AssetManager::GetAssetIDForFile("Assets/Meshes/Default/Plane.fbx");
		static const AssetHandle ConeHandle = AssetManager::GetAssetIDForFile("Assets/Meshes/Default/Cone.fbx");

		ImGui::Begin("Objects");
		{
			ImGui::BeginChild("##objects_window");
			DrawObject("Cube", CubeHandle);
			DrawObject("Capsule", CapsuleHandle);
			DrawObject("Sphere", SphereHandle);
			DrawObject("Cylinder", CylinderHandle);
			DrawObject("Torus", TorusHandle);
			DrawObject("Plane", PlaneHandle);
			DrawObject("Cone", ConeHandle);
			ImGui::EndChild();
		}

		ImGui::End();
	}
}