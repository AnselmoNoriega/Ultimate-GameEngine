#include "nrpch.h"
#include "PhysicsScene.h"

#include <glm/glm.hpp>

#include "NotRed/Asset/AssetManager.h"

#include "PhysicsManager.h"
#include "PhysicsInternal.h"
#include "PhysicsUtils.h"
#include "ContactListener.h"

#include "NotRed/Debug/Profiler.h"

namespace NR
{
	static ContactListener sContactListener;

	PhysicsScene::PhysicsScene(const Ref<Scene>& scene, const PhysicsSettings& settings)
		: mEntityScene(scene), mSubStepSize(settings.FixedDeltaTime)
	{
		physx::PxSceneDesc sceneDesc(PhysicsInternal::GetPhysicsSDK().getTolerancesScale());
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD | physx::PxSceneFlag::eENABLE_PCM;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ENHANCED_DETERMINISM;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;

		sceneDesc.gravity = PhysicsUtils::ToPhysicsVector(settings.Gravity);
		sceneDesc.broadPhaseType = PhysicsUtils::ToPhysicsBroadphaseType(settings.BroadphaseAlgorithm);
		sceneDesc.frictionType = PhysicsUtils::ToPhysicsFrictionType(settings.FrictionModel);
		sceneDesc.filterShader = (physx::PxSimulationFilterShader)PhysicsInternal::FilterShader;
		sceneDesc.cpuDispatcher = PhysicsInternal::GetCPUDispatcher();
		sceneDesc.simulationEventCallback = &sContactListener;

		NR_CORE_ASSERT(sceneDesc.isValid());

		mPhysicsScene = PhysicsInternal::GetPhysicsSDK().createScene(sceneDesc);
		mPhysicsControllerManager = PxCreateControllerManager(*mPhysicsScene);
		NR_CORE_ASSERT(mPhysicsScene);

		CreateRegions();
	}

	PhysicsScene::~PhysicsScene()
	{

	}

	void PhysicsScene::Simulate(float dt, bool callFixedUpdate)
	{
		if (callFixedUpdate)
		{
			for (auto& actor : mActors)
			{
				actor->FixedUpdate(mSubStepSize);
			}
		}

		bool advanced = Advance(dt);

		if (advanced)
		{
			uint32_t nbActiveActors;
			physx::PxActor** activeActors = mPhysicsScene->getActiveActors(nbActiveActors);
			for (uint32_t i = 0; i < nbActiveActors; ++i)
			{
				Ref<PhysicsActor> actor = (PhysicsActor*)activeActors[i]->userData;
				if (actor && !actor->IsSleeping())
				{
					actor->SynchronizeTransform();
				}
			}
		}
	}

	Ref<PhysicsActor> PhysicsScene::GetActor(Entity entity)
	{
		for (auto& actor : mActors)
		{
			if (actor->GetEntity() == entity)
				return actor;
		}

		return nullptr;
	}

	const Ref<PhysicsActor>& PhysicsScene::GetActor(Entity entity) const
	{
		for (const auto& actor : mActors)
		{
			if (actor->GetEntity() == entity)
				return actor;
		}

		return nullptr;
	}

	Ref<PhysicsActor> PhysicsScene::CreateActor(Entity entity)
	{
		NR_PROFILE_FUNC();

		Ref<PhysicsActor> actor = Ref<PhysicsActor>::Create(entity);

		mActors.push_back(actor);
		mPhysicsScene->addActor(*actor->mRigidActor);

		return actor;
	}

	void PhysicsScene::RemoveActor(Ref<PhysicsActor> actor)
	{
		NR_PROFILE_FUNC();

		if (!actor)
		{
			return;
		}

		for (auto& collider : actor->mColliders)
		{
			collider->DetachFromActor(actor->mRigidActor);
			collider->Release();
		}

		mPhysicsScene->removeActor(*actor->mRigidActor);
		actor->mRigidActor->release();
		actor->mRigidActor = nullptr;

		for (auto it = mActors.begin(); it != mActors.end(); it++)
		{
			if ((*it)->GetEntity() == actor->GetEntity())
			{
				mActors.erase(it);
				break;
			}
		}
	}

	Ref<PhysicsController> PhysicsScene::GetController(Entity entity)
	{
		for (auto& controller : mControllers)
		{
			if (controller->GetEntity() == entity)
				return controller;
		}

		return nullptr;
	}

	Ref<PhysicsController> PhysicsScene::CreateController(Entity entity)
	{
		Ref<PhysicsController> controller = Ref<PhysicsController>::Create(entity);

		const auto& capsuleColliderComponent = entity.GetComponent<CapsuleColliderComponent>();
		const auto& characterControllerComponent = entity.GetComponent<CharacterControllerComponent>();

		Ref<PhysicsMaterial> mat = AssetManager::GetAsset<PhysicsMaterial>(capsuleColliderComponent.Material);
		if (!mat)
		{
			mat = Ref<PhysicsMaterial>::Create(0.6f, 0.6f, 0.0f);
		}

		controller->mMaterial = PhysicsInternal::GetPhysicsSDK().createMaterial(mat->StaticFriction, mat->DynamicFriction, mat->Bounciness);

		physx::PxCapsuleControllerDesc desc;
		desc.position = PhysicsUtils::ToPhysicsExtendedVector(entity.Transform().Translation + capsuleColliderComponent.Offset); // not convinced this is correct.  (e.g. it needs to be world space, not local)
		desc.height = capsuleColliderComponent.Height;
		desc.radius = capsuleColliderComponent.Radius;
		desc.nonWalkableMode = physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING;
		desc.climbingMode = physx::PxCapsuleClimbingMode::eCONSTRAINED;
		desc.slopeLimit = std::max(0.0f, cos(glm::radians(characterControllerComponent.SlopeLimitDeg)));
		desc.stepOffset = characterControllerComponent.StepOffset;
		desc.contactOffset = 0.01;
		desc.material = controller->mMaterial;

		NR_CORE_VERIFY(controller->mController = mPhysicsControllerManager->createController(desc));

		mControllers.push_back(controller);
		return controller;
	}

	void PhysicsScene::RemoveController(Ref<PhysicsController> controller)
	{
		if (!controller || !controller->mController)
		{
			return;
		}

		controller->mController->release();
		controller->mController = nullptr;

		for (auto it = mControllers.begin(); it != mControllers.end(); it++)
		{
			if ((*it)->GetEntity() == controller->GetEntity())
			{
				mControllers.erase(it);
				break;
			}
		}
	}

	Ref<JointBase> PhysicsScene::GetJoint(Entity entity)
	{
		for (auto& joint : mJoints)
		{
			if (joint->GetEntity() == entity)
			{
				return joint;
			}
		}

		return nullptr;
	}

	Ref<JointBase> PhysicsScene::CreateJoint(Entity entity)
	{
		Ref<JointBase> joint = nullptr;

		if (entity.HasComponent<FixedJointComponent>())
		{
			joint = Ref<FixedJoint>::Create(entity);
		}

		if (!joint || !joint->IsValid())
		{
			return nullptr;
		}

		mJoints.push_back(joint);
		return joint;
	}

	void PhysicsScene::RemoveJoint(Ref<JointBase> joint)
	{
		if (!joint || !joint->IsValid())
		{
			return;
		}

		joint->Release();

		for (auto it = mJoints.begin(); it != mJoints.end(); it++)
		{
			if ((*it)->GetEntity() == joint->GetEntity())
			{
				mJoints.erase(it);
				break;
			}
		}
	}

	bool PhysicsScene::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit* outHit)
	{
		NR_PROFILE_FUNC();

		physx::PxRaycastBuffer hitInfo;
		bool result = mPhysicsScene->raycast(PhysicsUtils::ToPhysicsVector(origin), PhysicsUtils::ToPhysicsVector(glm::normalize(direction)), maxDistance, hitInfo);
		if (result)
		{
			Ref<PhysicsActor> actor = (PhysicsActor*)hitInfo.block.actor->userData;
			outHit->HitEntity = actor->GetEntity().GetID();
			outHit->Position = PhysicsUtils::FromPhysicsVector(hitInfo.block.position);
			outHit->Normal = PhysicsUtils::FromPhysicsVector(hitInfo.block.normal);
			outHit->Distance = hitInfo.block.distance;
		}
		return result;
	}

	bool PhysicsScene::OverlapBox(const glm::vec3& origin, const glm::vec3& halfSize, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		return OverlapGeometry(origin, physx::PxBoxGeometry(halfSize.x, halfSize.y, halfSize.z), buffer, count);
	}

	bool PhysicsScene::OverlapCapsule(const glm::vec3& origin, float radius, float halfHeight, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		return OverlapGeometry(origin, physx::PxCapsuleGeometry(radius, halfHeight), buffer, count);
	}

	bool PhysicsScene::OverlapSphere(const glm::vec3& origin, float radius, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		return OverlapGeometry(origin, physx::PxSphereGeometry(radius), buffer, count);
	}

	void PhysicsScene::AddRadialImpulse(const glm::vec3& origin, float radius, float strength, EFalloffMode falloff, bool velocityChange)
	{
		NR_PROFILE_FUNC();
		std::array<OverlapHit, OVERLAP_MAX_COLLIDERS> overlappedColliders;
		memset(overlappedColliders.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(OverlapHit));
		uint32_t count = 0;

		if (!OverlapSphere(origin, radius, overlappedColliders, count))
		{
			return;
		}

		for (uint32_t i = 0; i < count; ++i)
		{
			auto actor = overlappedColliders[i].Actor;
			if (actor->IsDynamic() && !actor->IsKinematic())
			{
				actor->AddRadialImpulse(origin, radius, strength, falloff, velocityChange);
			}
		}
	}

	static std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS> sOverlapBuffer;
	bool PhysicsScene::OverlapGeometry(const glm::vec3& origin, const physx::PxGeometry& geometry, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		NR_PROFILE_FUNC();

		physx::PxOverlapBuffer buf(sOverlapBuffer.data(), OVERLAP_MAX_COLLIDERS);
		physx::PxTransform pose = PhysicsUtils::ToPhysicsTransform(glm::translate(glm::mat4(1.0f), origin));

		bool result = mPhysicsScene->overlap(geometry, pose, buf);
		if (result)
		{
			count = buf.nbTouches > OVERLAP_MAX_COLLIDERS ? OVERLAP_MAX_COLLIDERS : buf.nbTouches;
			for (uint32_t i = 0; i < count; ++i)
			{
				buffer[i].Actor = (PhysicsActor*)sOverlapBuffer[i].actor->userData;
				buffer[i].Shape = (ColliderShape*)sOverlapBuffer[i].shape->userData;
			}
		}
		return result;
	}

	void PhysicsScene::CreateRegions()
	{
		NR_PROFILE_FUNC();

		const PhysicsSettings& settings = PhysicsManager::GetSettings();

		if (settings.BroadphaseAlgorithm == BroadphaseType::AutomaticBoxPrune)
		{
			return;
		}

		physx::PxBounds3* regionBounds = new physx::PxBounds3[settings.WorldBoundsSubdivisions * settings.WorldBoundsSubdivisions];
		physx::PxBounds3 globalBounds(PhysicsUtils::ToPhysicsVector(settings.WorldBoundsMin), PhysicsUtils::ToPhysicsVector(settings.WorldBoundsMax));
		uint32_t regionCount = physx::PxBroadPhaseExt::createRegionsFromWorldBounds(regionBounds, globalBounds, settings.WorldBoundsSubdivisions);
		
		for (uint32_t i = 0; i < regionCount; ++i)
		{
			physx::PxBroadPhaseRegion region;
			region.mBounds = regionBounds[i];
			mPhysicsScene->addBroadPhaseRegion(region);
		}
	}

	bool PhysicsScene::Advance(float dt)
	{
		NR_PROFILE_FUNC();

		SubstepStrategy(dt);

		for (uint32_t i = 0; i < mNumSubSteps; ++i)
		{
			mPhysicsScene->simulate(mSubStepSize);
			mPhysicsScene->fetchResults(true);
		}

		return mNumSubSteps != 0;
	}

	void PhysicsScene::SubstepStrategy(float dt)
	{
		if (mAccumulator > mSubStepSize)
		{
			mAccumulator = 0.0f;
		}

		mAccumulator += dt;
		if (mAccumulator < mSubStepSize)
		{
			mNumSubSteps = 0;
			return;
		}

		mNumSubSteps = glm::min(static_cast<uint32_t>(mAccumulator / mSubStepSize), MAX_SUB_STEPS);
		mAccumulator -= (float)mNumSubSteps * mSubStepSize;
	}

	void PhysicsScene::Destroy()
	{
		NR_CORE_ASSERT(mPhysicsScene);

		for (auto& joint : mJoints)
		{
			RemoveJoint(joint);
		}

		for (auto& controller : mControllers)
		{
			RemoveController(controller);
		}

		for (auto& actor : mActors)
		{
			RemoveActor(actor);
		}

		mControllers.clear();
		mActors.clear();
		mPhysicsControllerManager->release();
		mPhysicsScene->release();
		mPhysicsScene = nullptr;

		mEntityScene = nullptr;
	}
}