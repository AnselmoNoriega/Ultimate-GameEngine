#include "nrpch.h"
#include "PhysicsActor.h"

#include <glm/gtx/compatibility.hpp>

#include "PhysicsManager.h"
#include "PhysicsInternal.h"
#include "PhysicsLayer.h"

#include "NotRed/Debug/Profiler.h"

#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Asset/AssetManager.h"

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

	void PhysicsActor::SetPosition(const glm::vec3& translation, bool autowake)
	{
		physx::PxTransform transform = mRigidActor->getGlobalPose();
		transform.p = PhysicsUtils::ToPhysicsVector(translation);
		mRigidActor->setGlobalPose(transform, autowake);

		if (mRigidBodyData.BodyType == RigidBodyComponent::Type::Static)
		{
			SynchronizeTransform();
		}
	}

	void PhysicsActor::SetRotation(const glm::vec3& rotation, bool autowake)
	{
		physx::PxTransform transform = mRigidActor->getGlobalPose();
		transform.q = PhysicsUtils::ToPhysicsQuat(glm::quat(rotation));
		mRigidActor->setGlobalPose(transform, autowake);

		if (mRigidBodyData.BodyType == RigidBodyComponent::Type::Static)
		{
			SynchronizeTransform();
		}
	}

	void PhysicsActor::Rotate(const glm::vec3& rotation, bool autowake)
	{
		physx::PxTransform transform = mRigidActor->getGlobalPose();
		transform.q *= (physx::PxQuat(glm::radians(rotation.x), { 1.0f, 0.0f, 0.0f })
			* physx::PxQuat(glm::radians(rotation.y), { 0.0f, 1.0f, 0.0f })
			* physx::PxQuat(glm::radians(rotation.z), { 0.0f, 0.0f, 1.0f }));
		mRigidActor->setGlobalPose(transform, autowake);

		if (mRigidBodyData.BodyType == RigidBodyComponent::Type::Static)
		{
			SynchronizeTransform();
		}
	}

	void PhysicsActor::WakeUp()
	{
		if (IsDynamic())
		{
			mRigidActor->is<physx::PxRigidDynamic>()->wakeUp();
		}
	}

	void PhysicsActor::Sleep()
	{
		if (IsDynamic())
		{
			mRigidActor->is<physx::PxRigidDynamic>()->putToSleep();
		}
	}

	bool PhysicsActor::IsSleeping() const
	{
		return IsDynamic() ? mRigidActor->is<physx::PxRigidDynamic>()->isSleeping() : false;
	}

	float PhysicsActor::GetMass() const
	{
		return !IsDynamic() ? mRigidBodyData.Mass : mRigidActor->is<physx::PxRigidDynamic>()->getMass();
	}

	void PhysicsActor::SetMass(float mass)
	{
		if (IsKinematic())
		{
			return;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);

		physx::PxRigidBodyExt::setMassAndUpdateInertia(*actor, mass);
		mRigidBodyData.Mass = mass;
	}

	float PhysicsActor::GetInverseMass() const
	{
		return !IsDynamic() ? -mRigidBodyData.Mass : mRigidActor->is<physx::PxRigidDynamic>()->getInvMass();
	}

	glm::mat4 PhysicsActor::GetCenterOfMass() const
	{
		if (!IsDynamic())
		{
			return glm::mat4(1.0f);
		}

		const auto actor = mRigidActor->is<physx::PxRigidDynamic>();
		return PhysicsUtils::FromPhysicsTransform(actor->getGlobalPose().transform(actor->getCMassLocalPose()));
	}

	glm::mat4 PhysicsActor::GetLocalCenterOfMass() const { return !IsDynamic() ? glm::mat4(1.0f) : PhysicsUtils::FromPhysicsTransform(mRigidActor->is<physx::PxRigidDynamic>()->getCMassLocalPose()); }

	glm::vec3 PhysicsActor::GetKinematicTargetPosition() const
	{
		if (!IsKinematic())
		{
			NR_CORE_WARN("Trying to set kinematic target for a non-kinematic actor.");
			return glm::vec3(0.0f, 0.0f, 0.0f);
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);

		physx::PxTransform target;
		actor->getKinematicTarget(target);
		return PhysicsUtils::FromPhysicsVector(target.p);
	}

	glm::vec3 PhysicsActor::GetKinematicTargetRotation() const
	{
		if (!IsKinematic())
		{
			NR_CORE_WARN("Trying to get kinematic target for a non-kinematic actor.");
			return glm::vec3(0.0f, 0.0f, 0.0f);
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);

		physx::PxTransform target;
		actor->getKinematicTarget(target);
		return glm::eulerAngles(PhysicsUtils::FromPhysicsQuat(target.q));
	}

	void PhysicsActor::SetKinematicTarget(const glm::vec3& targetPosition, const glm::vec3& targetRotation) const
	{
		if (!IsKinematic())
		{
			NR_CORE_WARN("Trying to set kinematic target for a non-kinematic actor.");
			return;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		actor->setKinematicTarget(PhysicsUtils::ToPhysicsTransform(targetPosition, targetRotation));
	}

	void PhysicsActor::AddForce(const glm::vec3& force, ForceMode forceMode)
	{
		NR_PROFILE_FUNC();

		if (!IsDynamic() || IsKinematic())
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
		if (!IsDynamic() || IsKinematic())
		{
			NR_CORE_WARN("Trying to add torque to non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		actor->addTorque(PhysicsUtils::ToPhysicsVector(torque), (physx::PxForceMode::Enum)forceMode);
	}

	void PhysicsActor::AddRadialImpulse(const glm::vec3& origin, float radius, float strength, EFalloffMode falloff, bool velocityChange)
	{
		if (!IsDynamic() || IsKinematic())
		{
			return;
		}

		float mass = GetMass();
		glm::mat4 centerOfMassTransform = GetCenterOfMass();
		glm::vec3 centerOfMass = centerOfMassTransform[3];
		glm::vec3 delta = centerOfMass - origin;
		float magnitude = glm::length(delta);
		
		if (magnitude > radius)
		{
			return;
		}

		delta = glm::normalize(delta);
		float impulseMagnitude = strength;
		if (falloff == EFalloffMode::Linear)
		{
			impulseMagnitude *= (1.0f - (magnitude / radius));
		}

		glm::vec3 impulse = delta * impulseMagnitude;
		ForceMode mode = velocityChange ? ForceMode::VelocityChange : ForceMode::Impulse;
		AddForce(impulse, mode);
	}

	glm::vec3 PhysicsActor::GetVelocity() const
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

		mRigidActor->is<physx::PxRigidDynamic>()->setLinearVelocity(PhysicsUtils::ToPhysicsVector(velocity));
	}

	glm::vec3 PhysicsActor::GetAngularVelocity() const
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

	float PhysicsActor::GetLinearDrag() const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to get linear drag of non-dynamic PhysicsActor.");
			return 0.0f;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		return actor->getLinearDamping();
	}

	float PhysicsActor::GetAngularDrag() const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to get angular drag of non-dynamic PhysicsActor.");
			return 0.0f;
		}

		physx::PxRigidDynamic* actor = mRigidActor->is<physx::PxRigidDynamic>();
		NR_CORE_ASSERT(actor);
		return actor->getAngularDamping();
	}

	void PhysicsActor::SetSimulationData(uint32_t layerId)
	{
		const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(layerId);

		if (layerInfo.CollidesWith == 0)
		{
			return;
		}

		physx::PxFilterData filterData;
		filterData.word0 = layerInfo.BitValue;
		filterData.word1 = layerInfo.CollidesWith;
		filterData.word2 = (uint32_t)mRigidBodyData.CollisionDetection;

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
		if (!IsDynamic())
		{
			return;
		}

		if (addFlag)
		{
			mLockFlags |= (uint32_t)flag;
		}
		else
		{
			mLockFlags &= ~(uint32_t)flag;
		}

		mRigidActor->is<physx::PxRigidDynamic>()->setRigidDynamicLockFlags((physx::PxRigidDynamicLockFlags)mLockFlags);
	}

	void PhysicsActor::FixedUpdate(float fixedDeltaTime)
	{
		if (!ScriptEngine::IsEntityModuleValid(mEntity))
		{
			return;
		}

		ScriptEngine::PhysicsUpdateEntity(mEntity, fixedDeltaTime);
	}

	void PhysicsActor::AddCollider(BoxColliderComponent& collider, Entity entity, const glm::vec3& offset)
	{
		mColliders.push_back(Ref<BoxColliderShape>::Create(collider, *this, entity, offset));
	}

	void PhysicsActor::AddCollider(SphereColliderComponent& collider, Entity entity, const glm::vec3& offset)
	{
		mColliders.push_back(Ref<SphereColliderShape>::Create(collider, *this, entity, offset));
	}

	void PhysicsActor::AddCollider(CapsuleColliderComponent& collider, Entity entity, const glm::vec3& offset)
	{
		mColliders.push_back(Ref<CapsuleColliderShape>::Create(collider, *this, entity, offset));
	}

	void PhysicsActor::AddCollider(MeshColliderComponent& collider, Entity entity, const glm::vec3& offset)
	{
		NR_PROFILE_FUNC();
		const AssetMetadata& metadata = AssetManager::GetMetadata(collider.CollisionMesh);
		if (metadata.IsValid() && metadata.Type == AssetType::Mesh)
		{
			mColliders.push_back(Ref<ConvexMeshShape>::Create(collider, *this, entity, offset));
		}
		else
		{
			if (IsDynamic() && !IsKinematic())
			{
				NR_CORE_ERROR("Can't have a non-convex MeshColliderComponent for a non-kinematic dynamic RigidBodyComponent!");
				return;
			}

			mColliders.push_back(Ref<TriangleMeshShape>::Create(collider, *this, entity, offset));
		}
	}

	void PhysicsActor::CreateRigidActor()
	{
		NR_PROFILE_FUNC();

		auto& sdk = PhysicsInternal::GetPhysicsSDK();

		Ref<Scene> scene = Scene::GetScene(mEntity.GetSceneID());
		glm::mat4 transform = scene->GetWorldSpaceTransformMatrix(mEntity);

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

			ModifyLockFlag(ActorLockFlag::TranslationX, mRigidBodyData.LockPositionX);
			ModifyLockFlag(ActorLockFlag::TranslationY, mRigidBodyData.LockPositionY);
			ModifyLockFlag(ActorLockFlag::TranslationZ, mRigidBodyData.LockPositionZ);
			ModifyLockFlag(ActorLockFlag::RotationX, mRigidBodyData.LockRotationX);
			ModifyLockFlag(ActorLockFlag::RotationY, mRigidBodyData.LockRotationY);
			ModifyLockFlag(ActorLockFlag::RotationZ, mRigidBodyData.LockRotationZ);

			SetGravityDisabled(mRigidBodyData.DisableGravity);

			mRigidActor->is<physx::PxRigidDynamic>()->setSolverIterationCounts(settings.SolverIterations, settings.SolverVelocityIterations);
			mRigidActor->is<physx::PxRigidDynamic>()->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, mRigidBodyData.CollisionDetection == RigidBodyComponent::CollisionDetectionType::Continuous);
			mRigidActor->is<physx::PxRigidDynamic>()->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, mRigidBodyData.CollisionDetection == RigidBodyComponent::CollisionDetectionType::ContinuousSpeculative);
		}

		if (!PhysicsLayerManager::IsLayerValid(mRigidBodyData.Layer))
		{
			mRigidBodyData.Layer = 0;
		}

		if (mEntity.HasComponent<BoxColliderComponent>())
		{
			AddCollider(mEntity.GetComponent<BoxColliderComponent>(), mEntity);
		}
		if (mEntity.HasComponent<SphereColliderComponent>()) 
		{
			AddCollider(mEntity.GetComponent<SphereColliderComponent>(), mEntity);
		}
		if (mEntity.HasComponent<CapsuleColliderComponent>()) 
		{
			AddCollider(mEntity.GetComponent<CapsuleColliderComponent>(), mEntity);
		}
		if (mEntity.HasComponent<MeshColliderComponent>()) 
		{
			AddCollider(mEntity.GetComponent<MeshColliderComponent>(), mEntity);
		}

		SetMass(mRigidBodyData.Mass);

		mRigidActor->userData = this;

#ifdef NR_DEBUG
		auto& name = mEntity.GetComponent<TagComponent>().Tag;
		mRigidActor->setName(name.c_str());
#endif
	}

	void PhysicsActor::SynchronizeTransform()
	{
		TransformComponent& transform = mEntity.Transform();
		physx::PxTransform actorPose = mRigidActor->getGlobalPose();
		transform.Translation = PhysicsUtils::FromPhysicsVector(actorPose.p);
		transform.Rotation = glm::eulerAngles(PhysicsUtils::FromPhysicsQuat(actorPose.q));
	}
}