#pragma once

#include <glm/glm.hpp>

#include "NotRed/Renderer/Mesh.h"

namespace NR
{
	class Entity
	{
	public:
		Entity();
		~Entity();

		void SetMesh(const Ref<Mesh>& mesh) { mMesh = mesh; }
		Ref<Mesh> GetMesh() { return mMesh; }

		void SetMaterial(const Ref<MaterialInstance>& material) { mMaterial = material; }
		Ref<MaterialInstance> GetMaterial() { return mMaterial; }

		const glm::mat4& GetTransform() const { return mTransform; }
		glm::mat4& Transform() { return mTransform; }

	private:
		glm::mat4 mTransform;

		Ref<Mesh> mMesh;
		Ref<MaterialInstance> mMaterial;
	};

}