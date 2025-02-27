#pragma once

#include "PhysicsScene.h"

namespace NR
{
	class PhysicsManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void ScenePlay();
		static void SceneStop();

		static PhysicsSettings& GetSettings() { return sSettings; }

	private:
		static PhysicsSettings sSettings;
	};
}