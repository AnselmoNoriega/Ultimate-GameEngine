#include "nrpch.h"
#include "ObjectsPanel.h"

#include "NotRed/ImGui/ImGui.h"

namespace NR
{
    ObjectsPanel::ObjectsPanel()
    {
		mCubeImage = Texture2D::Create("Resources/Editor/asset.png");
    }

    void ObjectsPanel::DrawObject(const char* label, AssetHandle handle)
    {
        UI::Image(mCubeImage, ImVec2(30, 30));
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::Selectable(label);

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            UI::Image(mCubeImage, ImVec2(20, 20));
            ImGui::SameLine();

            ImGui::Text(label);

            ImGui::SetDragDropPayload("asset_payload", &handle, sizeof(AssetHandle));
            ImGui::EndDragDropSource();
        }

    }

	void ObjectsPanel::ImGuiRender()
	{
#if 0
		static const AssetHandle CubeHandle = AssetManager::GetAssetHandleFromFilePath("Assets/Meshes/Default/Cube.fbx");
		static const AssetHandle CapsuleHandle = AssetManager::GetAssetHandleFromFilePath("Assets/Meshes/Default/Capsule.fbx");
		static const AssetHandle SphereHandle = AssetManager::GetAssetHandleFromFilePath("Assets/Meshes/Default/Sphere.fbx");
		static const AssetHandle CylinderHandle = AssetManager::GetAssetHandleFromFilePath("Assets/Meshes/Default/Cylinder.fbx");
		static const AssetHandle TorusHandle = AssetManager::GetAssetHandleFromFilePath("Assets/Meshes/Default/Torus.fbx");
		static const AssetHandle PlaneHandle = AssetManager::GetAssetHandleFromFilePath("Assets/Meshes/Default/Plane.fbx");
		static const AssetHandle ConeHandle = AssetManager::GetAssetHandleFromFilePath("Assets/Meshes/Default/Cone.fbx");

		ImGui::Begin("Objects");
		{
			ImGui::BeginChild("##objects_window");
			DrawObject("Cube", CubeHandle);
			DrawObject("Sphere", SphereHandle);
			DrawObject("Capsule", CapsuleHandle);
			DrawObject("Cylinder", CylinderHandle);
			DrawObject("Torus", TorusHandle);
			DrawObject("Plane", PlaneHandle);
			DrawObject("Cone", ConeHandle);
			ImGui::EndChild();
		}

		ImGui::End();
#endif
		ImGui::Begin("Objects");
		ImGui::End();
	}
}