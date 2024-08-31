#pragma once

#include "NotRed/Scene/Entity.h"

namespace NR
{
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

	struct PhysicsSettings
	{
		float FixedDeltaTime = 0.02f;
		glm::vec3 Gravity = { 0.0f, -9.81f, 0.0f };
		BroadphaseType BroadphaseAlgorithm = BroadphaseType::AutomaticBoxPrune;
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

		static void DestroyScene();

		static PhysicsSettings& GetSettings();
	};
}