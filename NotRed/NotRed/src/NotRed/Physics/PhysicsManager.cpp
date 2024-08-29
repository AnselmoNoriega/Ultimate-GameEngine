#include "nrpch.h"
#include "PhysicsManager.h"

#include "PhysicsWrappers.h"
#include "NotRed/Scene/Scene.h"

namespace NR
{
	static physx::PxScene* sScene;
	static std::vector<Entity> sSimulatedEntities;
	static Entity* sEntityStorageBuffer;
	static int sEntityStorageBufferPosition;

	static std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4& transform)
	{
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat orientation;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		return { translation, orientation, scale };
	}

	void PhysicsManager::Init()
	{
		PhysicsWrappers::Initialize();
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

		// Create Actor Body
		physx::PxRigidActor* actor = PhysicsWrappers::CreateActor(rigidbody, e.Transform());
		sSimulatedEntities.push_back(e);
		Entity* entityStorage = &sEntityStorageBuffer[sEntityStorageBufferPosition++];
		*entityStorage = e;
		actor->userData = (void*)entityStorage;
		rigidbody.RuntimeActor = actor;

		// Physics Material
		physx::PxMaterial* material = PhysicsWrappers::CreateMaterial(e.GetComponent<PhysicsMaterialComponent>());

		auto [translation, rotationQuat, scale] = DecomposeTransform(e.Transform());

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
		if (rigidbody.BodyType == RigidBodyComponent::Type::Static)
		{
			PhysicsWrappers::SetCollisionFilters(*actor, (uint32_t)FilterGroup::Static, (uint32_t)FilterGroup::All);
		}
		else if (rigidbody.BodyType == RigidBodyComponent::Type::Dynamic)
		{
			PhysicsWrappers::SetCollisionFilters(*actor, (uint32_t)FilterGroup::Dynamic, (uint32_t)FilterGroup::All);
		}

		sScene->addActor(*actor);
	}

	void PhysicsManager::Simulate()
	{
		constexpr float stepSize = 0.016666660f;
		sScene->simulate(stepSize);
		sScene->fetchResults(true);

		for (Entity& e : sSimulatedEntities)
		{
			auto& transform = e.Transform();
			// TODO: Come up with a better solution for scale
			auto [p, r, scale] = DecomposeTransform(transform);
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