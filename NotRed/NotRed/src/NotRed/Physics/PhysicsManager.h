#pragma once

#include "PhysicsScene.h"

namespace NR
{
	class PhysicsManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void Simulate(float dt);

		static void RuntimePlay();
		static void RuntimeStop();

		static PhysicsSettings& GetSettings() { return sSettings; }
		static Ref<PhysicsScene> GetScene() { return sScene; }

	private:
		static PhysicsSettings sSettings;
		static Ref<PhysicsScene> sScene;
	};
}