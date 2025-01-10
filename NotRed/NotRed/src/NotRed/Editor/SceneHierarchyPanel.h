#pragma once

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"
#include "NotRed/Renderer/Mesh.h"

namespace NR
{
	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);
		void SetSelected(Entity entity);

		void SetSelectionChangedCallback(const std::function<void(Entity)>& func) { mSelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(Entity)>& func) { mEntityDeletedCallback = func; }
		void SetMeshAssetConvertCallback(const std::function<void(Entity, Ref<MeshAsset>)>& func) { mMeshAssetConvertCallback = func; }
		void SetInvalidMetadataCallback(const std::function<void(Entity, AssetHandle)>& func) { mInvalidMetadataCallback = func; }

		void ImGuiRender(bool window = true);

	private:
		void DrawEntityNode(Entity entity, const std::string& searchFilter = {});
		void DrawComponents(Entity entity);

	private:
		Ref<Scene> mContext;
		Entity mSelectionContext;

		Ref<Texture2D> mPencilIcon;
		Ref<Texture2D> mPlusIcon;

	private:
		std::function<void(Entity)> mSelectionChangedCallback, mEntityDeletedCallback;
		std::function<void(Entity, Ref<MeshAsset>)> mMeshAssetConvertCallback;
		std::function<void(Entity, AssetHandle)> mInvalidMetadataCallback;
	};

}