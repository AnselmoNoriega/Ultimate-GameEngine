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

		static void Init();
		static void Shutdown();

		void SetContext(const Ref<Scene>& scene);
		void SetSelected(Entity entity);

		void SetSelectionChangedCallback(const std::function<void(Entity)>& func) { mSelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(Entity)>& func) { mEntityDeletedCallback = func; }
		void SetMeshAssetConvertCallback(const std::function<void(Entity, Ref<MeshSource>)>& func) { mMeshAssetConvertCallback = func; }
		void SetInvalidMetadataCallback(const std::function<void(Entity, AssetHandle)>& func) { mInvalidMetadataCallback = func; }

		void ImGuiRender(bool window = true);

	private:
		void DrawEntityNode(Entity entity, const std::string& searchFilter = {});
		void DrawComponents(Entity entity);

	private:
		Ref<Scene> mContext;
		Entity mSelectionContext;

		static Ref<Texture2D> sPencilIcon;
		static Ref<Texture2D> sPlusIcon;
		static Ref<Texture2D> sGearIcon;

	private:
		std::function<void(Entity)> mSelectionChangedCallback, mEntityDeletedCallback;
		std::function<void(Entity, Ref<MeshSource>)> mMeshAssetConvertCallback;
		std::function<void(Entity, AssetHandle)> mInvalidMetadataCallback;
	};
}