#include "nrpch.h"
#include "PhysicsActor.h"

#include <glm/gtx/compatibility.hpp>

#include "PhysicsManager.h"
#include "PhysicsInternal.h"
#include "PhysicsLayer.h"

#include "NotRed/Script/ScriptEngine.h"

namespace NR
{
	PhysicsActor::PhysicsActor(Entity entity)
		: mEntity(entity), mRigidBodyData(entity.GetComponent<RigidBodyComponent>()), mRigidActor(nullptr)
	{
		CreateRigidActor();
	}

	PhysicsActor::~PhysicsActor()
	{
		mColliders.clear();
	}

	void PhysicsActor::Rotate(const glm::vec3& rotation)
	{
		physx::PxTransform transform = mRigidActor->getGlobalPose();
		transform.q *= (physx::PxQuat(glm::radians(rotation.x), { 1.0f, 0.0f, 0.0f })
			* physx::PxQuat(glm::radians(rotation.y), { 0.0f, 1.0f, 0.0f })
			* physx::PxQuat(glm::radians(rotation.z), { 0.0f, 0.0f, 1.0f }));
		mRigidActor->setGlobalPose(transform);
	}

	float PhysicsActor::GetMass() const
	{
		return !IsDynamic() ? mRigidBodyData.Mass : mRigidActor->is<physx::PxRigidDynamic>()->getMass();
	}

	void PhysicsActor::SetMass(float mass)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Cannot set mass of non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		physx::PxRigidBodyExt::setMassAndUpdateInertia(*actor, mass);
		mRigidBodyData.Mass = mass;
	}

	void PhysicsActor::AddForce(const glm::vec3& force, ForceMode forceMode)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to add force to non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		actor->addForce(PhysicsUtils::ToPhysicsVector(force), (physx::PxForceMode::Enum)forceMode);
	}

	void PhysicsActor::AddTorque(const glm::vec3& torque, ForceMode forceMode)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to add torque to non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		actor->addTorque(PhysicsUtils::ToPhysicsVector(torque), (physx::PxForceMode::Enum)forceMode);
	}

	const glm::vec3& PhysicsActor::GetVelocity() const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to get velocity of non-dynamic PhysicsActor.");
			return glm::vec3(0.0f);
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		return PhysicsUtils::FromPhysicsVector(actor->getLinearVelocity());
	}

	void PhysicsActor::SetVelocity(const glm::vec3& velocity)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to set velocity of non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		actor->setLinearVelocity(PhysicsUtils::ToPhysicsVector(velocity));
	}

	const glm::vec3& PhysicsActor::GetAngularVelocity() const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to get angular velocity of non-dynamic PhysicsActor.");
			return glm::vec3(0.0f);
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		return PhysicsUtils::FromPhysicsVector(actor->getAngularVelocity());
	}

	void PhysicsActor::SetAngularVelocity(const glm::vec3& velocity)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to set angular velocity of non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		actor->setAngularVelocity(PhysicsUtils::ToPhysicsVector(velocity));
	}

	float PhysicsActor::GetMaxVelocity() const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to get max linear velocity of non-dynamic PhysicsActor.");
			return 0.0f;
		}
		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		return actor->getMaxLinearVelocity();
	}

	void PhysicsActor::SetMaxVelocity(float maxVelocity)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to set max linear velocity of non-dynamic PhysicsActor.");
			return;
		}
		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		actor->setMaxLinearVelocity(maxVelocity);
	}

	float PhysicsActor::GetMaxAngularVelocity() const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to get max angular velocity of non-dynamic PhysicsActor.");
			return 0.0f;
		}
		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		return actor->getMaxAngularVelocity();
	}

	void PhysicsActor::SetMaxAngularVelocity(float maxVelocity)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to set max angular velocity of non-dynamic PhysicsActor.");
			return;
		}
		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		actor->setMaxAngularVelocity(maxVelocity);
	}

	void PhysicsActor::SetLinearDrag(float drag) const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to set linear drag of non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		actor->setLinearDamping(drag);
	}

	void PhysicsActor::SetAngularDrag(float drag) const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to set angular drag of non-dynamic PhysicsActor.");
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		actor->setAngularDamping(drag);
	}

	void PhysicsActor::SetLayer(uint32_t layerId)
	{
		const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(layerId);

		if (layerInfo.CollidesWith == 0)
		{
			return;
		}

		physx::PxFilterData filterData;
		filterData.word0 = layerInfo.BitValue;
		filterData.word1 = layerInfo.CollidesWith;

		for (auto& collider : mColliders)
		{
			collider->SetFilterData(filterData);
		}
	}

	void PhysicsActor::SetKinematic(bool isKinematic)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Static PhysicsActor can't be kinematic.");
			return;
		}

		mRigidBodyData.IsKinematic = isKinematic;
		mRigidActor->is<physx::PxRigidDynamic>()->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, isKinematic);
	}

	void PhysicsActor::SetGravityDisabled(bool disable)
	{
		mRigidActor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, disable);
	}

	void PhysicsActor::ModifyLockFlag(ActorLockFlag flag, bool addFlag)
	{
		if (addFlag)
		{
			mLockFlags |= (uint32_t)flag;
		}
		else
		{
			mLockFlags &= ~(uint32_t)flag;
		}

		if (!IsDynamic())
		{
			return;
		}

		mRigidActor->is<physx::PxRigidDynamic>()->setRigidDynamicLockFlag(PhysicsUtils::ToPhysicsActorLockFlag(flag), addFlag);
	}

	void PhysicsActor::FixedUpdate(float fixedDeltaTime)
	{
		if (!ScriptEngine::IsEntityModuleValid(mEntity))
		{
			return;
		}
		ScriptEngine::UpdatePhysicsEntity(mEntity, fixedDeltaTime);
	}

	void PhysicsActor::AddCollider(BoxColliderComponent& collider)
	{
		mColliders.push_back(Ref<BoxColliderShape>::Create(collider, *this));
	}

	void PhysicsActor::AddCollider(SphereColliderComponent& collider)
	{
		mColliders.push_back(Ref<SphereColliderShape>::Create(collider, *this));
	}

	void PhysicsActor::AddCollider(CapsuleColliderComponent& collider)
	{
		mColliders.push_back(Ref<CapsuleColliderShape>::Create(collider, *this));
	}

	void PhysicsActor::AddCollider(MeshColliderComponent& collider)
	{
		if (collider.IsConvex)
		{
			mColliders.push_back(Ref<ConvexMeshShape>::Create(collider, *this));
		}
		else
		{
			mColliders.push_back(Ref<TriangleMeshShape>::Create(collider, *this));
		}
	}

	void PhysicsActor::CreateRigidActor()
	{
		auto& sdk = PhysicsInternal::GetPhysicsSDK();

		Ref<Scene> scene = Scene::GetScene(mEntity.GetSceneID());
		glm::mat4 transform = scene->GetTransformRelativeToParent(mEntity);

		if (mRigidBodyData.BodyType == RigidBodyComponent::Type::Static)
		{
			mRigidActor = sdk.createRigidStatic(PhysicsUtils::ToPhysicsTransform(transform));
		}
		else
		{
			const PhysicsSettings& settings = PhysicsManager::GetSettings();

			mRigidActor = sdk.createRigidDynamic(PhysicsUtils::ToPhysicsTransform(transform));

			SetLinearDrag(mRigidBodyData.LinearDrag);
			SetAngularDrag(mRigidBodyData.AngularDrag);

			SetKinematic(mRigidBodyData.IsKinematic);

			ModifyLockFlag(ActorLockFlag::PositionX, mRigidBodyData.LockPositionX);
			ModifyLockFlag(ActorLockFlag::PositionY, mRigidBodyData.LockPositionY);
			ModifyLockFlag(ActorLockFlag::PositionZ, mRigidBodyData.LockPositionZ);
			ModifyLockFlag(ActorLockFlag::RotationX, mRigidBodyData.LockRotationX);
			ModifyLockFlag(ActorLockFlag::RotationY, mRigidBodyData.LockRotationY);
			ModifyLockFlag(ActorLockFlag::RotationZ, mRigidBodyData.LockRotationZ);

			SetGravityDisabled(mRigidBodyData.DisableGravity);

			mRigidActor->is<physx::PxRigidDynamic>()->setSolverIterationCounts(settings.SolverIterations, settings.SolverVelocityIterations);

			SetMass(mRigidBodyData.Mass);
		}

		if (!PhysicsLayerManager::IsLayerValid(mRigidBodyData.Layer))
		{
			mRigidBodyData.Layer = 0;
		}

		if (mEntity.HasComponent<BoxColliderComponent>())
		{
			AddCollider(mEntity.GetComponent<BoxColliderComponent>());
		}
		if (mEntity.HasComponent<SphereColliderComponent>()) 
		{
			AddCollider(mEntity.GetComponent<SphereColliderComponent>());
		}
		if (mEntity.HasComponent<CapsuleColliderComponent>()) 
		{
			AddCollider(mEntity.GetComponent<CapsuleColliderComponent>());
		}
		if (mEntity.HasComponent<MeshColliderComponent>()) 
		{
			AddCollider(mEntity.GetComponent<MeshColliderComponent>());
		}

		SetLayer(mRigidBodyData.Layer);
		mRigidActor->userData = &mEntity;

#ifdef NR_DEBUG
		auto& name = mEntity.GetComponent<TagComponent>().Tag;
		mRigidActor->setName(name.c_str());
#endif
	}

	void PhysicsActor::SynchronizeTransform()
	{
		if (IsDynamic())
		{
			TransformComponent& transform = mEntity.Transform();
			physx::PxTransform actorPose = mRigidActor->getGlobalPose();
			transform.Translation = PhysicsUtils::FromPhysicsVector(actorPose.p);
			transform.Rotation = glm::eulerAngles(PhysicsUtils::FromPhysicsQuat(actorPose.q));
		}
		else
		{
			Ref<Scene> scene = Scene::GetScene(mEntity.GetSceneID());
			mRigidActor->setGlobalPose(PhysicsUtils::ToPhysicsTransform(scene->GetTransformRelativeToParent(mEntity)));
		}
	}
}