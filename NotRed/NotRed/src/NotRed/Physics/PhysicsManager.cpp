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
	static int sEntityStorageBufferPosition;
	static float sSimulationTime = 0.0f;

	void PhysicsManager::Init()
	{
		PhysicsWrappers::Initialize();
		PhysicsLayerManager::AddLayer("Default");
	}

	void PhysicsManager::Shutdown()
	{
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

		physx::PxRigidActor* actor = PhysicsWrappers::CreateActor(rigidbody, e.Transform());
		sSimulatedEntities.push_back(e);
		Entity* entityStorage = &sEntityStorageBuffer[sEntityStorageBufferPosition++];
		*entityStorage = e;
		actor->userData = (void*)entityStorage;
		rigidbody.RuntimeActor = actor;

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
				transform = FromPhysicsTransform(actor->getGlobalPose()) * glm::scale(glm::mat4(1.0f), scale);
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
		sEntityStorageBufferPosition = 0;
		sSimulatedEntities.clear();
		sScene->release();
		sScene = nullptr;
	}

	void* PhysicsManager::GetPhysicsScene()
	{
		return sScene;
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