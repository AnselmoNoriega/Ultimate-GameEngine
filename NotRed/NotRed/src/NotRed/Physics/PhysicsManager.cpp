#include "nrpch.h"
#include "PhysicsManager.h"

#include "PhysicsWrappers.h"
#include "PhysicsLayer.h"

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

	static physx::PxScene* sScene;
	static std::vector<Entity> sSimulatedEntities;
	static Entity* sEntityStorageBuffer;
	static uint32_t sEntityBufferCount;
	static int sEntityStorageBufferPosition;
	static float sSimulationTime = 0.0f;

	static PhysicsSettings sSettings;

	void PhysicsManager::Init()
	{
		PhysicsWrappers::Initialize();
		PhysicsLayerManager::AddLayer("Default");
	}

	void PhysicsManager::Shutdown()
	{
		PhysicsWrappers::Shutdown();
	}

	void PhysicsManager::ExpandEntityBuffer(uint32_t amount)
	{
		NR_CORE_ASSERT(sScene);

		if (sEntityStorageBuffer != nullptr)
		{
			Entity* temp = new Entity[sEntityBufferCount + amount];
			memcpy(temp, sEntityStorageBuffer, sEntityBufferCount * sizeof(Entity));

			for (uint32_t i = 0; i < sEntityBufferCount; ++i)
			{
				Entity& e = sEntityStorageBuffer[i];
				RigidBodyComponent& rb = e.GetComponent<RigidBodyComponent>();

				if (rb.RuntimeActor)
				{
					physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rb.RuntimeActor);
					actor->userData = &temp[rb.EntityBufferIndex];
				}
			}

			delete[] sEntityStorageBuffer;
			sEntityStorageBuffer = temp;
			sEntityBufferCount += amount;
		}
		else
		{
			sEntityStorageBuffer = new Entity[amount];
			sEntityBufferCount = amount;
		}
	}

	void PhysicsManager::CreateScene()
	{
		NR_CORE_ASSERT(sScene == nullptr, "Scene already has a Physics Scene!");
		sScene = PhysicsWrappers::CreateScene();
	}

	void PhysicsManager::CreateActor(Entity e)
	{
		NR_CORE_ASSERT(sScene);

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

		physx::PxRigidActor* actor = PhysicsWrappers::CreateActor(rigidbody, e.Transform());

		if (rigidbody.BodyType == RigidBodyComponent::Type::Dynamic)
		{
			sSimulatedEntities.push_back(e);
		}

		Entity* entityStorage = &sEntityStorageBuffer[sEntityStorageBufferPosition];
		*entityStorage = e;
		actor->userData = (void*)entityStorage;
		rigidbody.RuntimeActor = actor;
		rigidbody.EntityBufferIndex = sEntityStorageBufferPosition;
		++sEntityStorageBufferPosition;

		physx::PxMaterial* material = PhysicsWrappers::CreateMaterial(e.GetComponent<PhysicsMaterialComponent>());

		const auto& transform = e.Transform();
		auto [translation, rotation, scale] = DecomposeTransform(transform);

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

		if (!PhysicsLayerManager::IsLayerValid(rigidbody.Layer))
		{
			rigidbody.Layer = 0;
		}

		PhysicsWrappers::SetCollisionFilters(*actor, rigidbody.Layer);

		sScene->addActor(*actor);
	}

	PhysicsSettings& PhysicsManager::GetSettings()
	{
		return sSettings;
	}

	void PhysicsManager::Simulate(float dt)
	{
		NR_CORE_ASSERT(sScene);

		sSimulationTime += dt;

		if (sSimulationTime < sSettings.FixedDeltaTime)
		{
			return;
		}

		sSimulationTime -= sSettings.FixedDeltaTime;

		sScene->simulate(sSettings.FixedDeltaTime);
		sScene->fetchResults(true);

		for (Entity& e : sSimulatedEntities)
		{
			auto& transform = e.Transform();
			auto [translation, rotation, scale] = DecomposeTransform(transform);
			RigidBodyComponent& rb = e.GetComponent<RigidBodyComponent>();
			physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rb.RuntimeActor);

			transform = FromPhysicsTransform(actor->getGlobalPose()) * glm::scale(glm::mat4(1.0F), scale);
		}
	}

	void PhysicsManager::DestroyScene()
	{
		NR_CORE_ASSERT(sScene);

		delete[] sEntityStorageBuffer;
		sEntityStorageBuffer = nullptr;
		sEntityStorageBufferPosition = 0;
		sSimulatedEntities.clear();
		sScene->release();
		sScene = nullptr;
	}

	void* PhysicsManager::GetPhysicsScene()
	{
		return sScene;
	}
}