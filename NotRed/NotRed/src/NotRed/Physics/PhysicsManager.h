#pragma once

#include "NotRed/Scene/Entity.h"

namespace NR
{
	class PhysicsActor;

	enum class ForceMode : uint16_t
	{
		Force,
		Impulse,
		VelocityChange,
		Acceleration
	};

	enum class FilterGroup : uint32_t
	{
		Static = 1 << 0,
		Dynamic = 1 << 1,
		Kinematic = 1 << 2,
		All = Static | Dynamic | Kinematic
	};

	enum class BroadphaseType
	{
		SweepAndPrune,
		MultiBoxPrune,
		AutomaticBoxPrune
	};

	enum class FrictionType
	{
		Patch,
		OneDirectional,
		TwoDirectional
	};

	struct PhysicsSettings
	{
		float FixedDeltaTime = 0.02f;
		glm::vec3 Gravity = { 0.0f, -9.81f, 0.0f };
		BroadphaseType BroadphaseAlgorithm = BroadphaseType::AutomaticBoxPrune;
		glm::vec3 WorldBoundsMin = glm::vec3(0.0f);
		glm::vec3 WorldBoundsMax = glm::vec3(1.0f);
		uint32_t WorldBoundsSubdivisions = 2;
		FrictionType FrictionModel = FrictionType::Patch;
		uint32_t SolverIterations = 6;
		uint32_t SolverVelocityIterations = 1;
	};

	struct RaycastHit
	{
		uint64_t EntityID;
		glm::vec3 Position;
		glm::vec3 Normal;
		float Distance;
	};

	class PhysicsManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void ExpandEntityBuffer(uint32_t entityCount);

		static void CreateScene();
		static void CreateActor(Entity e);

		static void Simulate(float dt);

		static void* GetPhysicsScene();
		static Ref<PhysicsActor> GetActorForEntity(Entity entity);

		static void DestroyScene();

		static PhysicsSettings& GetSettings();
	};
}