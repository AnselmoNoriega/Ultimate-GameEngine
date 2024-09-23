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
		// TODO: Call FixedUpdate

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