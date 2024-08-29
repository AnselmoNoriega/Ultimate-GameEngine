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

	struct SceneParams
	{
		glm::vec3 Gravity = { 0.0f, -9.81f, 0.0f };
	};

	class PhysicsManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void CreateScene(const SceneParams& params);
		static void CreateActor(Entity e, int entityCount);

		static void Simulate();

		static void DestroyScene();

		static void ConnectVisualDebugger();
		static void DisconnectVisualDebugger();
	};
}