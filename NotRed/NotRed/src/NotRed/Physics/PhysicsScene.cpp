#include "nrpch.h"
#include "PhysicsScene.h"

#include <glm/glm.hpp>

#include "PhysicsManager.h"
#include "PhysicsInternal.h"
#include "PhysicsUtils.h"
#include "ContactListener.h"

namespace NR
{
	static ContactListener sContactListener;

	PhysicsScene::PhysicsScene(const PhysicsSettings& settings)
		: mSubStepSize(settings.FixedDeltaTime)
	{
		physx::PxSceneDesc sceneDesc(PhysicsInternal::GetPhysicsSDK().getTolerancesScale());
		sceneDesc.gravity = PhysicsUtils::ToPhysicsVector(settings.Gravity);
		sceneDesc.broadPhaseType = PhysicsInternal::ToPhysicsBroadphaseType(settings.BroadphaseAlgorithm);
		sceneDesc.cpuDispatcher = PhysicsInternal::GetCPUDispatcher();
		sceneDesc.filterShader = (physx::PxSimulationFilterShader)PhysicsInternal::FilterShader;
		sceneDesc.simulationEventCallback = &sContactListener;
		sceneDesc.frictionType = PhysicsInternal::ToPhysicsFrictionType(settings.FrictionModel);

		NR_CORE_ASSERT(sceneDesc.isValid());

		mPhysicsScene = PhysicsInternal::GetPhysicsSDK().createScene(sceneDesc);
		NR_CORE_ASSERT(mPhysicsScene);
	}

	PhysicsScene::~PhysicsScene()
	{

	}

	void PhysicsScene::Simulate(float dt)
	{
		for (auto& actor : mActors)
		{
			actor->FixedUpdate(mSubStepSize);
		}

		bool advanced = Advance(dt);

		if (advanced)
		{
			for (auto& actor : mActors)
				actor->SynchronizeTransform();
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
		Ref<PhysicsActor> actor = Ref<PhysicsActor>::Create(entity);

		mActors.push_back(actor);
		mPhysicsScene->addActor(*actor->mRigidActor);

		return actor;
	}

	void PhysicsScene::RemoveActor(Ref<PhysicsActor> actor)
	{
		if (!actor)
			return;

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

	bool PhysicsScene::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit* outHit)
	{
		physx::PxRaycastBuffer hitInfo;
		bool result = mPhysicsScene->raycast(PhysicsUtils::ToPhysicsVector(origin), PhysicsUtils::ToPhysicsVector(glm::normalize(direction)), maxDistance, hitInfo);
		if (result)
		{
			Entity& entity = *(Entity*)hitInfo.block.actor->userData;
			outHit->HitEntity = entity.GetID();
			outHit->Position = PhysicsUtils::FromPhysicsVector(hitInfo.block.position);
			outHit->Normal = PhysicsUtils::FromPhysicsVector(hitInfo.block.normal);
			outHit->Distance = hitInfo.block.distance;
		}
		return result;
	}

	bool PhysicsScene::OverlapBox(const glm::vec3& origin, const glm::vec3& halfSize, std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		return OverlapGeometry(origin, physx::PxBoxGeometry(halfSize.x, halfSize.y, halfSize.z), buffer, count);
	}

	bool PhysicsScene::OverlapCapsule(const glm::vec3& origin, float radius, float halfHeight, std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		return OverlapGeometry(origin, physx::PxCapsuleGeometry(radius, halfHeight), buffer, count);
	}

	bool PhysicsScene::OverlapSphere(const glm::vec3& origin, float radius, std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		return OverlapGeometry(origin, physx::PxSphereGeometry(radius), buffer, count);
	}

	bool PhysicsScene::OverlapGeometry(const glm::vec3& origin, const physx::PxGeometry& geometry, std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		physx::PxOverlapBuffer buf(buffer.data(), OVERLAP_MAX_COLLIDERS);
		physx::PxTransform pose = PhysicsUtils::ToPhysicsTransform(glm::translate(glm::mat4(1.0f), origin));
		bool result = mPhysicsScene->overlap(geometry, pose, buf);
		if (result)
		{
			memcpy(buffer.data(), buf.touches, buf.nbTouches * sizeof(physx::PxOverlapHit));
			count = buf.nbTouches;
		}
		return result;
	}

	void PhysicsScene::CreateRegions()
	{
		const PhysicsSettings& settings = PhysicsManager::GetSettings();

		if (settings.BroadphaseAlgorithm != BroadphaseType::AutomaticBoxPrune)
		{
			physx::PxBounds3* regionBounds = new physx::PxBounds3[(size_t)settings.WorldBoundsSubdivisions * settings.WorldBoundsSubdivisions];
			physx::PxBounds3 globalBounds(PhysicsUtils::ToPhysicsVector(settings.WorldBoundsMin), PhysicsUtils::ToPhysicsVector(settings.WorldBoundsMax));
			uint32_t regionCount = physx::PxBroadPhaseExt::createRegionsFromWorldBounds(regionBounds, globalBounds, settings.WorldBoundsSubdivisions);

			for (uint32_t i = 0; i < regionCount; ++i)
			{
				physx::PxBroadPhaseRegion region;
				region.mBounds = regionBounds[i];
				mPhysicsScene->addBroadPhaseRegion(region);
			}
		}
	}

	bool PhysicsScene::Advance(float dt)
	{
		SubstepStrategy(dt, mNumSubSteps, mSubStepSize);

		if (mNumSubSteps == 0)
		{
			return false;
		}

		for (uint32_t i = 0; i < mNumSubSteps; ++i)
		{
			mPhysicsScene->simulate(mSubStepSize);
			mPhysicsScene->fetchResults(true);
		}

		return true;
	}

	void PhysicsScene::SubstepStrategy(float dt, uint32_t& substepCount, float& substepSize)
	{
		if (mAccumulator > mSubStepSize)
		{
			mAccumulator = 0.0f;
		}

		mAccumulator += dt;
		if (mAccumulator < mSubStepSize)
		{
			substepCount = 0;
			return;
		}

		substepSize = mSubStepSize;
		substepCount = glm::min(static_cast<uint32_t>(mAccumulator / mSubStepSize), MAX_SUB_STEPS);

		mAccumulator -= (float)substepCount * substepSize;
	}

	void PhysicsScene::Destroy()
	{
		NR_CORE_ASSERT(mPhysicsScene);

		for (auto& actor : mActors)
		{
			RemoveActor(actor);
		}

		mActors.clear();
		mPhysicsScene->release();
		mPhysicsScene = nullptr;
	}
}