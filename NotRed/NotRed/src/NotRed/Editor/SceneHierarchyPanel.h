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

		void ImGuiRender();

		void SetSelectionChangedCallback(const std::function<void(Entity)>& func) { mSelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(Entity)>& func) { mEntityDeletedCallback = func; }
		
	private:
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
		void DrawMeshNode(const Ref<Mesh>& mesh, uint32_t& imguiMeshID);
		void MeshNodeHierarchy(const Ref<Mesh>& mesh, aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

	private:
		Ref<Scene> mContext;
		Entity mSelectionContext;

		std::function<void(Entity)> mSelectionChangedCallback, mEntityDeletedCallback;
	};
}