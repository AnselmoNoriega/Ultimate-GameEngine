#include "nrpch.h"
#include "PhysicsManager.h"

#include <extensions/PxBroadPhaseExt.h>

#include "NotRed/Script/ScriptEngine.h"

#include "PhysicsWrappers.h"
#include "PhysicsLayer.h"
#include "PhysicsActor.h"

namespace NR
{
	static physx::PxScene* sScene;
	static std::vector<Ref<PhysicsActor>> sSimulatedActors;
	static std::vector<Ref<PhysicsActor>> sStaticActors;
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

				Ref<PhysicsActor>& actor = GetActorForEntity(e);
				actor->SetUserData(&temp[rb.EntityBufferIndex]);
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

		if (sSettings.BroadphaseAlgorithm != BroadphaseType::AutomaticBoxPrune)
		{
			physx::PxBounds3* regionBounds;
			physx::PxBounds3 globalBounds(ToPhysicsVector(sSettings.WorldBoundsMin), ToPhysicsVector(sSettings.WorldBoundsMax));
			uint32_t regionCount = physx::PxBroadPhaseExt::createRegionsFromWorldBounds(regionBounds, globalBounds, sSettings.WorldBoundsSubdivisions);

			for (uint32_t i = 0; i < regionCount; ++i)
			{
				physx::PxBroadPhaseRegion region;
				region.mBounds = regionBounds[i];
				sScene->addBroadPhaseRegion(region);
			}
		}
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

		Ref<PhysicsActor> actor = Ref<PhysicsActor>::Create(e);

		if (actor->IsDynamic())
		{
			sSimulatedActors.push_back(actor);
		}
		else
		{
			sStaticActors.push_back(actor);
		}

		Entity* entityStorage = &sEntityStorageBuffer[sEntityStorageBufferPosition];
		*entityStorage = e;
		actor->SetUserData((void*)entityStorage);
		actor->mRigidBody.EntityBufferIndex = sEntityStorageBufferPosition;
		++sEntityStorageBufferPosition;
	}

	Ref<PhysicsActor> PhysicsManager::GetActorForEntity(Entity entity)
	{
		for (auto& actor : sStaticActors)
		{
			if (actor->GetEntity() == entity)
			{
				return actor;
			}
		}

		for (auto& actor : sSimulatedActors)
		{
			if (actor->GetEntity() == entity)
			{
				return actor;
			}
		}

		return nullptr;
	}

	PhysicsSettings& PhysicsManager::GetSettings()
	{
		return sSettings;
	}

	void PhysicsManager::Simulate(float dt)
	{
		sSimulationTime += dt;

		if (sSimulationTime < sSettings.FixedDeltaTime)
		{
			return;
		}

		sSimulationTime -= sSettings.FixedDeltaTime;

		for (auto& actor : sSimulatedActors)
		{
			actor->Update(sSettings.FixedDeltaTime);
		}

		sScene->simulate(sSettings.FixedDeltaTime);
		sScene->fetchResults(true);

		for (auto& actor : sSimulatedActors)
		{
			actor->SynchronizeTransform();
		}
	}

	void PhysicsManager::DestroyScene()
	{
		NR_CORE_ASSERT(sScene);

		delete[] sEntityStorageBuffer;
		sEntityStorageBuffer = nullptr;
		sEntityStorageBufferPosition = 0;
		sStaticActors.clear();
		sSimulatedActors.clear();
		sScene->release();
		sScene = nullptr;
	}

	void* PhysicsManager::GetPhysicsScene()
	{
		return sScene;
	}
}