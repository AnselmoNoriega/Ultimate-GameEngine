#include "nrpch.h"
#include "PhysicsManager.h"

#include "PhysicsWrappers.h"
#include "NotRed/Scene/Scene.h"

namespace NR
{
	static std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4& transform)
	{
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat orientation;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		return { translation, orientation, scale };
	}

	static std::vector<std::string> sLayerNames;
	static uint32_t sLastLayerID = 0;

	uint32_t PhysicsLayerManager::AddLayer(const std::string& name)
	{
		PhysicsLayer layer = { sLastLayerID, name, (1 << sLastLayerID) };
		sLayers[sLastLayerID] = layer;
		++sLastLayerID;
		sLayerNames.push_back(name);

		SetLayerCollision(layer.ID, layer.ID, true);

		return layer.ID;
	}

	void PhysicsLayerManager::RemoveLayer(uint32_t layerId)
	{
		if (sLayers.find(layerId) != sLayers.end())
		{
			for (std::vector<std::string>::iterator it = sLayerNames.begin(); it != sLayerNames.end(); ++it)
			{
				if (*it == sLayers[layerId].Name)
				{
					sLayerNames.erase(it--);
				}
			}

			sLayers.erase(layerId);
		}
	}

	void PhysicsLayerManager::SetLayerCollision(uint32_t layerId, uint32_t otherLayer, bool collides)
	{
		if (sLayerCollisions.find(layerId) == sLayerCollisions.end())
		{
			sLayerCollisions[layerId] = std::vector<PhysicsLayer>();
			sLayerCollisions[layerId].reserve(1);
		}

		if (sLayerCollisions.find(otherLayer) == sLayerCollisions.end())
		{
			sLayerCollisions[otherLayer] = std::vector<PhysicsLayer>();
			sLayerCollisions[otherLayer].reserve(1);
		}

		if (collides)
		{
			sLayerCollisions[layerId].push_back(GetLayerInfo(otherLayer));
			sLayerCollisions[otherLayer].push_back(GetLayerInfo(layerId));
		}
		else
		{
			for (std::vector<PhysicsLayer>::iterator it = sLayerCollisions[layerId].begin(); it != sLayerCollisions[layerId].end(); ++it)
			{
				if (it->ID == otherLayer)
				{
					sLayerCollisions[layerId].erase(it--);
				}
			}

			for (std::vector<PhysicsLayer>::iterator it = sLayerCollisions[otherLayer].begin(); it != sLayerCollisions[otherLayer].end(); ++it)
			{
				if (it->ID == layerId)
				{
					sLayerCollisions[otherLayer].erase(it--);
				}
			}
		}
	}

	const std::vector<PhysicsLayer>& PhysicsLayerManager::GetLayerCollisions(uint32_t layerId)
	{
		NR_CORE_ASSERT(sLayerCollisions.find(layerId) != sLayerCollisions.end());
		return sLayerCollisions[layerId];
	}

	const PhysicsLayer& PhysicsLayerManager::GetLayerInfo(uint32_t layerId)
	{
		NR_CORE_ASSERT(sLayers.find(layerId) != sLayers.end());
		return sLayers[layerId];
	}

	const PhysicsLayer& PhysicsLayerManager::GetLayerInfo(const std::string& layerName)
	{
		for (const auto& kv : sLayers)
		{
			if (kv.second.Name == layerName)
			{
				return kv.second;
			}
		}

		return {};
	}

	const std::vector<std::string>& PhysicsLayerManager::GetLayerNames()
	{
		return sLayerNames;
	}

	void PhysicsLayerManager::ClearLayers()
	{
		sLayers.clear();
		sLayerCollisions.clear();
		sLastLayerID = 0;
		sLayerNames.clear();
	}

	void PhysicsLayerManager::Init()
	{
		AddLayer("Default");
	}

	void PhysicsLayerManager::Shutdown()
	{
	}

	std::unordered_map<uint32_t, PhysicsLayer> PhysicsLayerManager::sLayers;
	std::unordered_map<uint32_t, std::vector<PhysicsLayer>> PhysicsLayerManager::sLayerCollisions;

	static physx::PxScene* sScene;
	static std::vector<Entity> sSimulatedEntities;
	static Entity* sEntityStorageBuffer;
	static int sEntityStorageBufferPosition;
	static float sSimulationTime = 0.0f;

	void PhysicsManager::Init()
	{
		PhysicsWrappers::Initialize();
		PhysicsLayerManager::Init();
	}

	void PhysicsManager::Shutdown()
	{
		PhysicsLayerManager::Shutdown();
		PhysicsWrappers::Shutdown();
	}

	void PhysicsManager::CreateScene(const SceneParams& params)
	{
		NR_CORE_ASSERT(sScene == nullptr, "Scene already has a Physics Scene!");
		sScene = PhysicsWrappers::CreateScene(params);
	}

	void PhysicsManager::CreateActor(Entity e, int entityCount)
	{
		if (!e.HasComponent<RigidBodyComponent>())
		{
			NR_CORE_WARN("Trying to create PhysX actor from a non-rigidbody actor!");
			return;
		}

		if (!e.HasComponent<PhysicsMaterialComponent>())
		{
			NR_CORE_WARN("Trying to create PhysX actor without a PhysicsMaterialComponent!");
			return;
		}

		RigidBodyComponent& rigidbody = e.GetComponent<RigidBodyComponent>();

		if (sEntityStorageBuffer == nullptr)
		{
			sEntityStorageBuffer = new Entity[entityCount];
		}

		// Create Actor Body
		physx::PxRigidActor* actor = PhysicsWrappers::CreateActor(rigidbody, e.Transform());
		sSimulatedEntities.push_back(e);
		Entity* entityStorage = &sEntityStorageBuffer[sEntityStorageBufferPosition++];
		*entityStorage = e;
		actor->userData = (void*)entityStorage;
		rigidbody.RuntimeActor = actor;

		// Physics Material
		physx::PxMaterial* material = PhysicsWrappers::CreateMaterial(e.GetComponent<PhysicsMaterialComponent>());


		const auto& transform = e.Transform();
		auto [translation, rotation, scale] = DecomposeTransform(transform);

		// Add all colliders
		if (e.HasComponent<BoxColliderComponent>())
		{
			BoxColliderComponent& collider = e.GetComponent<BoxColliderComponent>();
			PhysicsWrappers::AddBoxCollider(*actor, *material, collider, scale);
		}

		if (e.HasComponent<SphereColliderComponent>())
		{
			SphereColliderComponent& collider = e.GetComponent<SphereColliderComponent>();
			PhysicsWrappers::AddSphereCollider(*actor, *material, collider, scale);
		}

		if (e.HasComponent<CapsuleColliderComponent>())
		{
			CapsuleColliderComponent& collider = e.GetComponent<CapsuleColliderComponent>();
			PhysicsWrappers::AddCapsuleCollider(*actor, *material, collider, scale);
		}

		if (e.HasComponent<MeshColliderComponent>())
		{
			MeshColliderComponent& collider = e.GetComponent<MeshColliderComponent>();
			PhysicsWrappers::AddMeshCollider(*actor, *material, collider, scale);
		}

		// Set collision filters
		PhysicsWrappers::SetCollisionFilters(*actor, rigidbody.Layer);

		sScene->addActor(*actor);
	}

	void PhysicsManager::Simulate(float dt)
	{
		constexpr float stepSize = 1.0f / 50.0f;
		sSimulationTime += dt;

		if (sSimulationTime < stepSize)
		{
			return;
		}

		sSimulationTime -= stepSize;

		sScene->simulate(stepSize);
		sScene->fetchResults(true);

		for (Entity& e : sSimulatedEntities)
		{
			auto& transform = e.Transform();

			auto [translation, rotation, scale] = DecomposeTransform(transform);
			RigidBodyComponent& rb = e.GetComponent<RigidBodyComponent>();
			physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rb.RuntimeActor);

			if (rb.BodyType == RigidBodyComponent::Type::Dynamic)
			{
				transform = FromPhysicsTransform(actor->getGlobalPose()) * glm::scale(glm::mat4(1.0F), scale);
			}
			else if (rb.BodyType == RigidBodyComponent::Type::Static)
			{
				actor->setGlobalPose(ToPhysicsTransform(transform));
			}
		}
	}

	void PhysicsManager::DestroyScene()
	{
		delete[] sEntityStorageBuffer;
		sEntityStorageBuffer = nullptr;
		sSimulatedEntities.clear();
		sScene->release();
		sScene = nullptr;
	}

	void PhysicsManager::ConnectVisualDebugger()
	{
		PhysicsWrappers::ConnectVisualDebugger();
	}

	void PhysicsManager::DisconnectVisualDebugger()
	{
		PhysicsWrappers::DisconnectVisualDebugger();
	}

}