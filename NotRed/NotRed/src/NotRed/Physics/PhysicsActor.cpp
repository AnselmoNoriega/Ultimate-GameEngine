#include "nrpch.h"
#include "PhysicsActor.h"

#include <PxPhysicsAPI.h>

#include <glm/gtx/compatibility.hpp>

#include "NotRed/Script/ScriptEngine.h"

#include "PhysicsUtil.h"
#include "PhysicsWrappers.h"
#include "PhysicsLayer.h"


namespace NR
{
	PhysicsActor::PhysicsActor(Entity entity)
		: mEntity(entity), mRigidBody(entity.GetComponent<RigidBodyComponent>())
	{
		if (!mEntity.HasComponent<PhysicsMaterialComponent>())
		{
			mMaterial.StaticFriction = 1.0f;
			mMaterial.DynamicFriction = 1.0f;
			mMaterial.Bounciness = 0.0f;
		}
		else
		{
			mMaterial = entity.GetComponent<PhysicsMaterialComponent>();
		}

		Initialize();
	}

	PhysicsActor::~PhysicsActor()
	{
		if (mActorInternal && mActorInternal->isReleasable())
		{
			mActorInternal->release();
			mActorInternal = nullptr;
		}
	}

	glm::vec3 PhysicsActor::GetPosition()
	{
		return FromPhysicsVector(mActorInternal->getGlobalPose().p);
	}

	glm::quat PhysicsActor::GetRotation()
	{
		return FromPhysicsQuat(mActorInternal->getGlobalPose().q);
	}

	void PhysicsActor::Rotate(const glm::vec3& rotation)
	{
		physx::PxTransform transform = mActorInternal->getGlobalPose();
		transform.q *= (physx::PxQuat(glm::radians(rotation.x), { 1.0f, 0.0f, 0.0f })
			* physx::PxQuat(glm::radians(rotation.y), { 0.0f, 1.0f, 0.0f })
			* physx::PxQuat(glm::radians(rotation.z), { 0.0f, 0.0f, 1.0f }));
		mActorInternal->setGlobalPose(transform);
	}

	float PhysicsActor::GetMass() const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to set mass of non-dynamic PhysicsActor.");
			return 0.0f;
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)mActorInternal;
		return actor->getMass();
	}

	void PhysicsActor::SetMass(float mass)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to set mass of non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)mActorInternal;
		physx::PxRigidBodyExt::setMassAndUpdateInertia(*actor, mass);
		mRigidBody.Mass = mass;
	}

	void PhysicsActor::AddForce(const glm::vec3& force, ForceMode forceMode)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to add force to non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)mActorInternal;
		actor->addForce(ToPhysicsVector(force), (physx::PxForceMode::Enum)forceMode);
	}

	void PhysicsActor::AddTorque(const glm::vec3& torque, ForceMode forceMode)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to add torque to non-dynamic PhysicsActor.");
			return;
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)mActorInternal;
		actor->addTorque(ToPhysicsVector(torque), (physx::PxForceMode::Enum)forceMode);
	}

	glm::vec3 PhysicsActor::GetVelocity() const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to get velocity of non-dynamic PhysicsActor.");
			return glm::vec3(0.0f);
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)mActorInternal;
		return FromPhysicsVector(actor->getLinearVelocity());
	}

	void PhysicsActor::SetVelocity(const glm::vec3& velocity)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to set velocity of non-dynamic PhysicsActor.");
			return;
		}

		if (!glm::all(glm::isfinite(velocity)))
		{
			return;
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)mActorInternal;
		actor->setLinearVelocity(ToPhysicsVector(velocity));
	}

	glm::vec3 PhysicsActor::GetAngularVelocity() const
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to get angular velocity of non-dynamic PhysicsActor.");
			return glm::vec3(0.0f);
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)mActorInternal;
		return FromPhysicsVector(actor->getAngularVelocity());
	}

	void PhysicsActor::SetAngularVelocity(const glm::vec3& velocity)
	{
		if (!IsDynamic())
		{
			NR_CORE_WARN("Trying to set angular velocity of non-dynamic PhysicsActor.");
			return;
		}

		if (!glm::all(glm::isfinite(velocity)))
			return;

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)mActorInternal;
		actor->setAngularVelocity(ToPhysicsVector(velocity));
	}

	void PhysicsActor::SetDrag(float drag) const
	{
		if (!IsDynamic())
		{
			return;
		}

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)mActorInternal;
		actor->setLinearDamping(drag);
	}

	void PhysicsActor::SetAngularDrag(float drag) const
	{
		if (!IsDynamic())
			return;

		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)mActorInternal;
		actor->setAngularDamping(drag);
	}

	void PhysicsActor::SetLayer(uint32_t layerId)
	{
		physx::PxAllocatorCallback& allocator = PhysicsWrappers::GetAllocator();
		const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(layerId);

		if (layerInfo.CollidesWith == 0)
		{
			return;
		}

		physx::PxFilterData filterData;
		filterData.word0 = layerInfo.BitValue;
		filterData.word1 = layerInfo.CollidesWith;

		const physx::PxU32 numShapes = mActorInternal->getNbShapes();
		physx::PxShape** shapes = (physx::PxShape**)allocator.allocate(sizeof(physx::PxShape*) * numShapes, "", "", 0);
		mActorInternal->getShapes(shapes, numShapes);

		for (physx::PxU32 i = 0; i < numShapes; ++i)
		{
			shapes[i]->setSimulationFilterData(filterData);
		}

		allocator.deallocate(shapes);
	}

	void PhysicsActor::Initialize()
	{
		physx::PxPhysics& physics = PhysicsWrappers::GetPhysics();

		Ref<Scene> scene = Scene::GetScene(mEntity.GetSceneID());
		glm::mat4 transform = scene->GetTransformRelativeToParent(mEntity);

		if (mRigidBody.BodyType == RigidBodyComponent::Type::Static)
		{
			mActorInternal = physics.createRigidStatic(ToPhysicsTransform(transform));
		}
		else
		{
			const PhysicsSettings& settings = PhysicsManager::GetSettings();

			physx::PxRigidDynamic* actor = physics.createRigidDynamic(ToPhysicsTransform(transform));
			actor->setLinearDamping(mRigidBody.LinearDrag);
			actor->setAngularDamping(mRigidBody.AngularDrag);
			actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, mRigidBody.IsKinematic);

			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X, mRigidBody.LockPositionX);
			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, mRigidBody.LockPositionY);
			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, mRigidBody.LockPositionZ);

			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, mRigidBody.LockRotationX);
			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, mRigidBody.LockRotationY);
			actor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, mRigidBody.LockRotationZ);

			actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, mRigidBody.DisableGravity);
			actor->setSolverIterationCounts(settings.SolverIterations, settings.SolverVelocityIterations);

			physx::PxRigidBodyExt::setMassAndUpdateInertia(*actor, mRigidBody.Mass);
			mActorInternal = actor;
		}

		mMaterialInternal = physics.createMaterial(mMaterial.StaticFriction, mMaterial.DynamicFriction, mMaterial.Bounciness);
		if (mEntity.HasComponent<BoxColliderComponent>()) 
		{
			PhysicsWrappers::AddBoxCollider(*this);
		}
		if (mEntity.HasComponent<SphereColliderComponent>()) 
		{
			PhysicsWrappers::AddSphereCollider(*this);
		}
		if (mEntity.HasComponent<CapsuleColliderComponent>()) 
		{
			PhysicsWrappers::AddCapsuleCollider(*this);
		}
		if (mEntity.HasComponent<MeshColliderComponent>()) 
		{
			PhysicsWrappers::AddMeshCollider(*this);
		}

		if (!PhysicsLayerManager::IsLayerValid(mRigidBody.Layer))
		{
			mRigidBody.Layer = 0;
		}

		SetLayer(mRigidBody.Layer);
		mActorInternal->userData = &mEntity;
	}

	void PhysicsActor::Spawn()
	{
		((physx::PxScene*)PhysicsManager::GetPhysicsScene())->addActor(*mActorInternal);
	}

	void PhysicsActor::Update(float fixedTimestep)
	{
		if (!ScriptEngine::IsEntityModuleValid(mEntity))
		{
			return;
		}

		ScriptEngine::UpdateEntityPhysics(mEntity, fixedTimestep);
	}

	void PhysicsActor::SynchronizeTransform()
	{
		if (IsDynamic())
		{
			TransformComponent& transform = mEntity.Transform();
			physx::PxTransform actorPose = mActorInternal->getGlobalPose();
			transform.Translation = FromPhysicsVector(actorPose.p);
			transform.Rotation = glm::eulerAngles(FromPhysicsQuat(actorPose.q));
		}
		else
		{
			Ref<Scene> scene = Scene::GetScene(mEntity.GetSceneID());
			mActorInternal->setGlobalPose(ToPhysicsTransform(scene->GetTransformRelativeToParent(mEntity)));
		}
	}

	void PhysicsActor::AddCollisionShape(physx::PxShape* shape)
	{
		bool status = mActorInternal->attachShape(*shape);
		shape->release();
		if (!status)
		{
			shape = nullptr;
		}
	}
}