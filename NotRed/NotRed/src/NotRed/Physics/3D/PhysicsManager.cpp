#include "nrpch.h"
#include "PhysicsManager.h"

#include "PhysicsInternal.h"
#include "PhysicsLayer.h"
#include "Debug/PhysicsDebugger.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Project/Project.h"

#include "NotRed/ImGui/ImGui.h"

#include "NotRed/Debug/Profiler.h"

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
		NR_PROFILE_FUNC();

		sScene = Ref<PhysicsScene>::Create(sSettings);	

#ifdef NR_DEBUG
		if (sSettings.DebugOnPlay && !PhysicsDebugger::IsDebugging())
		{
			PhysicsDebugger::StartDebugging(
				(Project::GetActive()->GetProjectDirectory() / "PhysicsDebugInfo").string(), 
				PhysicsManager::GetSettings().DebugType == DebugType::LiveDebug
			);
		}
#endif
	}

	void PhysicsManager::DestroyScene()
	{
#ifdef NR_DEBUG
		if (sSettings.DebugOnPlay)
		{
			PhysicsDebugger::StopDebugging();
		}
#endif

		sScene->Destroy();
	}

	void PhysicsManager::CreateActors(Ref<Scene> scene)
	{
		NR_PROFILE_FUNC();

		{
			auto view = scene->GetAllEntitiesWith<RigidBodyComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, scene.Raw() };
				CreateActor(e);
			}
		}
		{
			auto view = scene->GetAllEntitiesWith<CharacterControllerComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, scene.Raw() };
				CreateController(e);
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

				if (e.HasComponent<CharacterControllerComponent>())
				{
					continue;
				}

				if (!e.HasAny<BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent>())
				{
					continue;
				}

				bool parentWithRigidBody = false;
				Entity current = e;
				while (Entity parent = scene->FindEntityByID(current.GetParentID()))
				{
					if (parent.HasComponent<RigidBodyComponent>())
					{
						parentWithRigidBody = true;
						break;
					}
					current = parent;
				}

				if (parentWithRigidBody)
				{
					continue;
				}

				auto& rigidbody = e.AddComponent<RigidBodyComponent>();

				CreateActor(e);
			}
		}
	}

	Ref<PhysicsActor> PhysicsManager::CreateActor(Entity entity)
	{
		NR_PROFILE_FUNC();

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

	Ref<PhysicsController> PhysicsManager::CreateController(Entity entity)
	{
		auto existingController = sScene->GetController(entity);
		if (existingController)
		{
			return existingController;
		}

		Ref<PhysicsController> controller = sScene->CreateController(entity);
		controller->SetSimulationData(entity.GetComponent<CharacterControllerComponent>().Layer);
		return controller;
	}

	void PhysicsManager::ImGuiRender()
	{
		ImGui::Begin("Physics Stats");
		if (sScene && sScene->IsValid())
		{
			auto gravity = sScene->GetGravity();
			std::string gravityString = fmt::format("X: {0}, Y: {1}, Z: {2}", gravity.x, gravity.y, gravity.z);
			ImGui::Text("Gravity: %s", gravityString.c_str());
			
			const auto& actors = sScene->GetActors();
			ImGui::Text("Actors: %d", actors.size());

			for (const auto& actor : actors)
			{
				UUID id = actor->GetEntity().GetID();
				std::string label = fmt::format("{0}##{1}", actor->GetEntity().GetComponent<TagComponent>().Tag, id);
				bool open = UI::PropertyGridHeader(label, false);
				if (open)
				{
					UI::BeginPropertyGrid();
					UI::PushItemDisabled();

					glm::vec3 translation = actor->GetPosition();
					glm::vec3 rotation = actor->GetRotation();
					
					UI::Property("Translation", translation);
					UI::Property("Rotation", rotation);
					
					bool isDynamic = actor->IsDynamic();
					bool isKinematic = actor->IsKinematic();
					
					UI::Property("Is Dynamic", isDynamic);
					UI::Property("Is Kinematic", isKinematic);

					if (actor->IsDynamic())
					{
						float mass = actor->GetMass();
						UI::Property("Mass", mass);

						float inverseMass = actor->GetInverseMass();
						UI::Property("Inverse Mass", inverseMass);

						bool hasGravity = !actor->IsGravityDisabled();
						UI::Property("Has Gravity", hasGravity);

						bool isSleeping = actor->IsSleeping();
						UI::Property("Is Sleeping", isSleeping);

						glm::vec3 linearVelocity = actor->GetVelocity();
						float maxLinearVelocity = actor->GetMaxVelocity();
						glm::vec3 angularVelocity = actor->GetAngularVelocity();
						float maxAngularVelocity = actor->GetMaxAngularVelocity();

						UI::Property("Linear Velocity", linearVelocity);
						UI::Property("Max Linear Velocity", maxLinearVelocity);
						UI::Property("Angular Velocity", angularVelocity);
						UI::Property("Max Angular Velocity", maxAngularVelocity);

						float linearDrag = actor->GetLinearDrag();
						float angularDrag = actor->GetAngularDrag();

						UI::Property("Linear Drag", linearDrag);
						UI::Property("Angular Drag", angularDrag);
					}

					UI::PopItemDisabled();
					UI::EndPropertyGrid();

					const auto& collisionShapes = actor->GetCollisionShapes();
					ImGui::Text("Shapes: %d", collisionShapes.size());

					for (const auto& shape : collisionShapes)
					{
						std::string shapeLabel = fmt::format("{0}##{1}", shape->GetShapeName(), id);
						bool shapeOpen = UI::PropertyGridHeader(shapeLabel, false);
						if (shapeOpen)
						{
							UI::BeginPropertyGrid();
							UI::PushItemDisabled();

							glm::vec3 offset = shape->GetOffset();
							bool isTrigger = shape->IsTrigger();

							UI::Property("Offset", offset);
							UI::Property("Is Trigger", isTrigger);

							const auto& material = shape->GetMaterial();
							float staticFriction = material.getStaticFriction();
							float dynamicFriction = material.getDynamicFriction();
							float restitution = material.getRestitution();

							UI::Property("Static Friction", staticFriction);
							UI::Property("Dynamic Friction", staticFriction);
							UI::Property("Restitution", restitution);

							UI::PopItemDisabled();
							UI::EndPropertyGrid();
							ImGui::TreePop();
						}
					}
					ImGui::TreePop();
				}
			}
		}
		ImGui::End();
	}

	PhysicsSettings PhysicsManager::sSettings;
	Ref<PhysicsScene> PhysicsManager::sScene;
}