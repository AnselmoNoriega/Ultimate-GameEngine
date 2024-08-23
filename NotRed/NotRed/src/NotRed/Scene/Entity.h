#pragma once

#include <glm/glm.hpp>

#include "Scene.h"
#include "Components.h"
#include "NotRed/Renderer/Mesh.h"

namespace NR
{
	class Entity
	{
	public:
		Entity() = delete;
		Entity(entt::entity handle, Scene* scene)
			: mEntityHandle(handle), mScene(scene) {}

		~Entity() = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			return mScene->mRegistry.emplace<T>(mEntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			return mScene->mRegistry.get<T>(mEntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return mScene->mRegistry.has<T>(mEntityHandle);
		}

		glm::mat4& Transform() { return mScene->mRegistry.get<TransformComponent>(mEntityHandle); }
		const glm::mat4& Transform() const { return mScene->mRegistry.get<TransformComponent>(mEntityHandle); }

		operator uint32_t () const { return (uint32_t)mEntityHandle; }
		operator bool() const { return (uint32_t)mEntityHandle && mScene; }

		bool operator==(const Entity& other) const
		{
			return mEntityHandle == other.mEntityHandle && mScene == other.mScene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		Entity(const std::string& name);

	private:
		entt::entity mEntityHandle;
		Scene* mScene = nullptr;

		friend class Scene;
		friend class ScriptEngine;
	};

}