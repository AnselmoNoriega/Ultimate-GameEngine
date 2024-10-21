#pragma once

#include "NotRed/Renderer/Mesh.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"

#include "NotRed/Editor/EditorCamera.h"

#include "AssetEditorPanel.h"

namespace NR
{
	class MeshViewerPanel : public AssetEditor
	{
	public:
		MeshViewerPanel();
		~MeshViewerPanel() = default;

		void Update(float dt) override;
		void OnEvent(Event& e) override;
		void RenderViewport();
		void ImGuiRender() override;

		void SetAsset(const Ref<Asset>& asset) override;

		void Close() override {}
		void Render() override {}

		void ResetCamera(EditorCamera& camera);

	private:
		struct MeshScene
		{
			Ref<Scene> mScene;
			Ref<SceneRenderer> mSceneRenderer;

			EditorCamera mCamera;

			std::string mName;

			Ref<MeshAsset> mMeshAsset;
			Ref<Mesh> mMesh;

			Entity mMeshEntity;
			Entity mDirectionaLight;

			bool mViewportPanelFocused = false;
			bool mViewportPanelMouseOver = false;
			bool mResetDockspace = true;
			bool mIsViewportVisible = false;
		};

	private:
		void RenderMeshTab(ImGuiID dockspaceID, const std::shared_ptr<MeshScene>& sceneData);
		void DrawMeshNode(const Ref<MeshAsset>& meshAsset, const Ref<Mesh>& mesh);
		void MeshNodeHierarchy(const Ref<MeshAsset>& meshAsset, Ref<Mesh> mesh, aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

	private:
		bool mResetDockspace = true;

		std::unordered_map<std::string, std::shared_ptr<MeshScene>> mOpenMeshes;
		glm::vec2 mViewportBounds[2];

		bool mWindowOpen = false;

		std::string mTabToFocus;
	};

}