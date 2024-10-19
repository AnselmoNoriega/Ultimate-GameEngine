#pragma once

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"
#include "NotRed/Renderer/Mesh.h"

namespace NR
{
	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);
		void SetSelected(Entity entity);

		void SetSelectionChangedCallback(const std::function<void(Entity)>& func) { mSelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(Entity)>& func) { mEntityDeletedCallback = func; }
		void SetMeshAssetConvertCallback(const std::function<void(Entity, Ref<MeshAsset>)>& func) { mMeshAssetConvertCallback = func; }
		void SetInvalidMetadataCallback(const std::function<void(Entity, AssetHandle)>& func) { mInvalidMetadataCallback = func; }

		void ImGuiRender();

	private:
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);

	private:
		Ref<Scene> mContext;
		Entity mSelectionContext;

		std::function<void(Entity)> mSelectionChangedCallback, mEntityDeletedCallback;
		std::function<void(Entity, Ref<MeshAsset>)> mMeshAssetConvertCallback;
		std::function<void(Entity, AssetHandle)> mInvalidMetadataCallback;
	};

}