#pragma once

#include <PxPhysicsAPI.h>

#include "PhysicsSettings.h"
#include "PhysicsActor.h"

namespace NR
{
	class PhysicsScene : public RefCounted
	{
	public:
		PhysicsScene(const PhysicsSettings& settings);
		~PhysicsScene();

		void Simulate(float dt);

		Ref<PhysicsActor> GetActor(Entity entity);
		const Ref<PhysicsActor>& GetActor(Entity entity) const;

		Ref<PhysicsActor> CreateActor(Entity entity);
		void RemoveActor(Ref<PhysicsActor> actor);

	private:
		void CreateRegions();

		bool Advance(float dt);
		void SubstepStrategy(float dt, uint32_t& substepCount, float& substepSize);

		void Destroy();

	private:
		physx::PxScene* mPhysicsScene;

		std::vector<Ref<PhysicsActor>> mActors;

		float mSubStepSize;
		float mAccumulator = 0.0f;
		uint32_t mNumSubSteps = 0;
		const uint32_t MAX_SUB_STEPS = 8;

	private:
		friend class PhysicsManager;
	};
}