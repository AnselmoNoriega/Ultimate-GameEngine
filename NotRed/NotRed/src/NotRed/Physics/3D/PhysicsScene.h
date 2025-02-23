#pragma once

#define OVERLAP_MAX_COLLIDERS 10

#include <PhysX/include/PxPhysicsAPI.h>

#include "PhysicsSettings.h"
#include "PhysicsActor.h"

namespace NR
{
	struct RaycastHit
	{
		uint64_t HitEntity;
		glm::vec3 Position;
		glm::vec3 Normal;
		float Distance;
	};

	struct OverlapHit
	{
		Ref<PhysicsActor> Actor;
		Ref<ColliderShape> Shape;
	};

	class PhysicsScene : public RefCounted
	{
	public:
		PhysicsScene(const PhysicsSettings& settings);
		~PhysicsScene();

		void Simulate(float dt, bool callFixedUpdate = true);

		Ref<PhysicsActor> GetActor(Entity entity);
		const Ref<PhysicsActor>& GetActor(Entity entity) const;

		Ref<PhysicsActor> CreateActor(Entity entity);
		void RemoveActor(Ref<PhysicsActor> actor);

		const std::vector<Ref<PhysicsActor>>& GetActors() const { return mActors; }

		glm::vec3 GetGravity() const { return PhysicsUtils::FromPhysicsVector(mPhysicsScene->getGravity()); }
		void SetGravity(const glm::vec3& gravity) { mPhysicsScene->setGravity(PhysicsUtils::ToPhysicsVector(gravity)); }

		bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit* outHit);

		bool OverlapBox(const glm::vec3& origin, const glm::vec3& halfSize, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count);
		bool OverlapCapsule(const glm::vec3& origin, float radius, float halfHeight, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count);
		bool OverlapSphere(const glm::vec3& origin, float radius, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count);
		void AddRadialImpulse(const glm::vec3& origin, float radius, float strength, EFalloffMode falloff = EFalloffMode::Constant, bool velocityChange = false);

		bool IsValid() const { return mPhysicsScene != nullptr; }

	private:
		void CreateRegions();

		bool Advance(float dt);
		void SubstepStrategy(float dt);

		void Destroy();

		bool OverlapGeometry(const glm::vec3& origin, const physx::PxGeometry& geometry, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count);

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