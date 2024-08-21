#pragma once

#include <glm/glm.hpp>

#include "NotRed/Renderer/Mesh.h"

namespace NR
{
	class Entity
	{
	public:
		Entity() = delete;
		~Entity();

		void SetMesh(const Ref<Mesh>& mesh) { mMesh = mesh; }
		Ref<Mesh> GetMesh() { return mMesh; }

		void SetMaterial(const Ref<MaterialInstance>& material) { mMaterial = material; }
		Ref<MaterialInstance> GetMaterial() { return mMaterial; }

		const std::string& GetName() const { return mName; }
		const glm::mat4& GetTransform() const { return mTransform; }
		glm::mat4& Transform() { return mTransform; }

	private:
		Entity(const std::string& name);

	private:
		std::string mName;
		glm::mat4 mTransform;

		Ref<Mesh> mMesh;
		Ref<MaterialInstance> mMaterial;

		friend class Scene;
	};

}