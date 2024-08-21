#pragma once

#include "NotRed/Scene/Scene.h"

namespace NR
{
	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);

		void ImGuiRender();
		
	private:
		void DrawMeshNode(const Ref<Mesh>& mesh);
		void MeshNodeHierarchy(const Ref<Mesh>& mesh, aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);
		
	private:
		Ref<Scene> mContext;
		Ref<Mesh> mSelectionContext;
	};
}