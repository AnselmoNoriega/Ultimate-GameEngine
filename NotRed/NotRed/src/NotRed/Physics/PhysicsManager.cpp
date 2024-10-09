#include "nrpch.h"
#include "PhysicsManager.h"

#include "PhysicsInternal.h"
#include "PhysicsLayer.h"
#include "Debug/PhysicsDebugger.h"

#include "NotRed/Scene/Scene.h"

namespace NR
{
	void PhysicsManager::Init()
	{
		PhysicsInternal::Initialize();
		PhysicsLayerManager::AddLayer("Default");
	}

	void PhysicsManager::Shutdown()
	{
		PhysicsInternal::Shutdown();
	}

	void PhysicsManager::CreateScene()
	{
		sScene = Ref<PhysicsScene>::Create(sSettings);	
		
		if (sSettings.DebugOnPlay)
		{
			PhysicsDebugger::StartDebugging("PhysicsDebugInfo");
		}
	}

	void PhysicsManager::DestroyScene()
	{
		if (sSettings.DebugOnPlay)
		{
			PhysicsDebugger::StopDebugging();
		}
		sScene->Destroy();
	}

	void PhysicsManager::CreateActors(Ref<Scene> scene)
	{
		{
			auto view = scene->GetAllEntitiesWith<RigidBodyComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, scene.Raw() };
				CreateActor(e);
			}
		}
		{
			auto view = scene->GetAllEntitiesWith<TransformComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, scene.Raw() };
				if (e.HasComponent<RigidBodyComponent>())
				{
					continue;
				}

				if (!e.HasAny<BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent>())
				{
					continue;
				}

				bool noRigidBodyInChain = true;
				Entity current = e;
				while (Entity parent = scene->FindEntityByID(current.GetParentID()))
				{
					if (parent.HasComponent<RigidBodyComponent>())
					{
						noRigidBodyInChain = false;
						break;
					}
					current = parent;
				}
				if (!noRigidBodyInChain)
				{
					continue;
				}

				e.AddComponent<RigidBodyComponent>();
				CreateActor(e);
			}
		}
	}

	Ref<PhysicsActor> PhysicsManager::CreateActor(Entity entity)
	{
		auto existingActor = sScene->GetActor(entity);
		if (existingActor)
		{
			return existingActor;
		}

		Ref<PhysicsActor> actor = sScene->CreateActor(entity);
		Ref<Scene> scene = Scene::GetScene(entity.GetSceneID());
		for (auto childId : entity.Children())
		{
			Entity child = scene->FindEntityByID(childId);
			if (child.HasComponent<RigidBodyComponent>())
			{
				CreateActor(child);
				continue;
			}
			if (child.HasComponent<BoxColliderComponent>())
			{
				actor->AddCollider(child.GetComponent<BoxColliderComponent>(), child, child.Transform().Translation);
			}
			if (child.HasComponent<SphereColliderComponent>())
			{
				actor->AddCollider(child.GetComponent<SphereColliderComponent>(), child, child.Transform().Translation);
			}
			if (child.HasComponent<CapsuleColliderComponent>())
			{
				actor->AddCollider(child.GetComponent<CapsuleColliderComponent>(), child, child.Transform().Translation);
			}
			if (child.HasComponent<MeshColliderComponent>())
			{
				actor->AddCollider(child.GetComponent<MeshColliderComponent>(), child, child.Transform().Translation);
			}
		}
		actor->SetSimulationData(entity.GetComponent<RigidBodyComponent>().Layer);
		return actor;
	}

	PhysicsSettings PhysicsManager::sSettings;
	Ref<PhysicsScene> PhysicsManager::sScene;
}