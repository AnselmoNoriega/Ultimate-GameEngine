#pragma once

#include "PhysicsScene.h"

namespace NR
{
	class PhysicsManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void SetCurrentScene(const Ref<PhysicsScene>& scene) { sScene = scene; }

		static void CreateScene();
		static void DestroyScene();

		static PhysicsSettings& GetSettings() { return sSettings; }
		// Gives the current scene
		static Ref<PhysicsScene> GetScene() { return sScene; }

		static void ImGuiRender();

		static void CreateActors(Ref<Scene> scene);
		static Ref<PhysicsActor> CreateActor(Entity entity);
		static Ref<PhysicsController> CreateController(Entity entity);

	private:
		static PhysicsSettings sSettings;
		static Ref<PhysicsScene> sScene;
	};
}