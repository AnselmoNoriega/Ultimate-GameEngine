#pragma once

#include "NotRed/Asset/Asset.h"

#include "Entity.h"
#include "Entt/include/entt.hpp"

namespace NR
{
	class Prefab : public Asset
	{
	public:
		Prefab();

		~Prefab();

		// Replaces existing entity if present
		void Create(Entity entity, bool serialize = true);

		static AssetType GetStaticType() { return AssetType::Prefab; }
		AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		Entity CreatePrefabFromEntity(Entity entity);

	private:
		Ref<Scene> mScene;
		Entity mEntity;

	private:
		friend class Scene;
		friend class PrefabEditor;
		friend class PrefabSerializer;
	};
}