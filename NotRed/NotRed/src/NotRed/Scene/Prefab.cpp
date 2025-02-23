#include "nrpch.h"
#include "Prefab.h"

#include "Scene.h"
#include "NotRed/Audio/AudioComponent.h"

#include "NotRed/Asset/AssetImporter.h"

namespace NR
{
	template<typename T>
	static void CopyComponentIfExists(entt::entity dst, entt::registry& dstRegistry, entt::entity src, entt::registry& srcRegistry)
	{
		if (srcRegistry.try_get<T>(src))
		{
			auto& srcComponent = srcRegistry.get<T>(src);
			dstRegistry.emplace_or_replace<T>(dst, srcComponent);
		}
	}

	Entity Prefab::CreatePrefabFromEntity(Entity entity)
	{
		NR_CORE_ASSERT(Handle);

		Entity newEntity = mScene->CreateEntity();

		// Add PrefabComponent
		PrefabComponent prefabComponent;
		prefabComponent.EntityID = Handle;
		prefabComponent.PrefabID = newEntity.GetComponent<IDComponent>().ID;
		newEntity.AddComponent<PrefabComponent>(prefabComponent);

		CopyComponentIfExists<TagComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<TransformComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<MeshComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<StaticMeshComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<DirectionalLightComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<PointLightComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<SkyLightComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<ScriptComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<CameraComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<TextComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<RigidBody2DComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<RigidBodyComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<CharacterControllerComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<BoxColliderComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<SphereColliderComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<CapsuleColliderComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<MeshColliderComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<AudioComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);
		CopyComponentIfExists<AudioListenerComponent>(newEntity, mScene->mRegistry, entity, entity.mScene->mRegistry);

		for (auto childId : entity.Children())
		{
			Entity childDuplicate = CreatePrefabFromEntity(entity.mScene->FindEntityByID(childId));

			childDuplicate.SetParentID(newEntity.GetID());
			newEntity.Children().push_back(childDuplicate.GetID());
		}

		return newEntity;
	}

	Prefab::Prefab()
	{
		mScene = Scene::CreateEmpty();

		mScene->mRegistry.on_construct<ScriptComponent>().connect<&Scene::ScriptComponentConstruct>(mScene);
		mScene->mRegistry.on_destroy<ScriptComponent>().connect<&Scene::ScriptComponentDestroy>(mScene);
	}

	Prefab::~Prefab()
	{
		mScene->mRegistry.on_construct<ScriptComponent>().disconnect(this);
		mScene->mRegistry.on_destroy<ScriptComponent>().disconnect(this);
	}

	void Prefab::Create(Entity entity, bool serialize)
	{
		mScene = Scene::CreateEmpty();
		mEntity = CreatePrefabFromEntity(entity);
		if (serialize)
		{
			AssetImporter::Serialize(this);
		}
	}
}