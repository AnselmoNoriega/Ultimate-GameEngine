#include "nrpch.h"
#include "PhysicsScene.h"

#include <glm/glm.hpp>

#include "NotRed/Asset/AssetManager.h"

#include "PhysicsManager.h"
#include "PhysicsInternal.h"
#include "PhysicsUtils.h"
#include "ContactListener.h"

#include "NotRed/Debug/Profiler.h"

#include "NotRed/ImGui/ImGui.h"

namespace NR
{
	static ContactListener sContactListener;

	PhysicsScene::PhysicsScene(const PhysicsSettings& settings)
		: mSubStepSize(settings.FixedDeltaTime)
	{
		physx::PxSceneDesc sceneDesc(PhysicsInternal::GetPhysicsSDK().getTolerancesScale());
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD | physx::PxSceneFlag::eENABLE_PCM;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ENHANCED_DETERMINISM;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;

		sceneDesc.gravity = PhysicsUtils::ToPhysicsVector(settings.Gravity);
		sceneDesc.broadPhaseType = PhysicsUtils::ToPhysicsBroadphaseType(settings.BroadphaseAlgorithm);
		sceneDesc.frictionType = PhysicsUtils::ToPhysicsFrictionType(settings.FrictionModel);
		sceneDesc.filterShader = (physx::PxSimulationFilterShader)PhysicsInternal::FilterShader;
		sceneDesc.cpuDispatcher = PhysicsInternal::GetCPUDispatcher();
		sceneDesc.simulationEventCallback = &sContactListener;

		NR_CORE_ASSERT(sceneDesc.isValid());

		mPhysicsScene = PhysicsInternal::GetPhysicsSDK().createScene(sceneDesc);
		mPhysicsControllerManager = PxCreateControllerManager(*mPhysicsScene);
		NR_CORE_ASSERT(mPhysicsScene);

		CreateRegions();
	}

	PhysicsScene::~PhysicsScene()
	{
		Clear();

		mPhysicsControllerManager->release();
		mPhysicsScene->release();
		mPhysicsScene = nullptr;
	}

	void PhysicsScene::Simulate(float dt)
	{
		if (mEntityScene->IsPlaying())
		{
			for (auto& actor : mActors)
			{
				actor->FixedUpdate(mSubStepSize);
			}
		}

		for (auto& controller : mControllers)
		{
			controller->Update(dt);
		}

		bool advanced = Advance(dt);

		if (advanced)
		{
			uint32_t nbActiveActors;
			physx::PxActor** activeActors = mPhysicsScene->getActiveActors(nbActiveActors);
			for (uint32_t i = 0; i < nbActiveActors; ++i)
			{
				Ref<PhysicsActor> actor = (PhysicsActor*)activeActors[i]->userData;
				if (actor && !actor->IsSleeping())
				{
					actor->SynchronizeTransform();
				}
			}

			for (auto& controller : mControllers)
			{
				controller->SynchronizeTransform();
			}			
			
			for (auto& joint : mJoints)
			{
				joint->PostSimulation();
			}
		}
	}

	bool PhysicsScene::Advance(float dt)
	{
		SubstepStrategy(dt);

		for (uint32_t i = 0; i < mNumSubSteps; ++i)
		{
			{
				NR_PROFILE_FUNC("PxScene::simulate");
				mPhysicsScene->simulate(mSubStepSize, nullptr);
			}
			{
				NR_PROFILE_FUNC("PxScene::fetchResults");
				mPhysicsScene->fetchResults(true);
			}
		}

		return mNumSubSteps != 0;
	}

	void PhysicsScene::SubstepStrategy(float dt)
	{
		if (mAccumulator > mSubStepSize)
		{
			mAccumulator = 0.0f;
		}

		mAccumulator += dt;
		if (mAccumulator < mSubStepSize)
		{
			mNumSubSteps = 0;
			return;
		}

		mNumSubSteps = glm::min(static_cast<uint32_t>(mAccumulator / mSubStepSize), MAX_SUB_STEPS);
		mAccumulator -= (float)mNumSubSteps * mSubStepSize;
	}

	Ref<PhysicsActor> PhysicsScene::GetActor(Entity entity)
	{
		for (auto& actor : mActors)
		{
			if (actor->GetEntity() == entity)
			{
				return actor;
			}
		}

		return nullptr;
	}

	const Ref<PhysicsActor>& PhysicsScene::GetActor(Entity entity) const
	{
		for (const auto& actor : mActors)
		{
			if (actor->GetEntity() == entity)
			{
				return actor;
			}
		}

		return nullptr;
	}

	Ref<PhysicsActor> PhysicsScene::CreateActor(Entity entity)
	{
		NR_PROFILE_FUNC();

		auto existingActor = GetActor(entity);
		if (existingActor)
		{
			return existingActor;
		}

		Ref<PhysicsActor> actor = Ref<PhysicsActor>::Create(entity);

		for (auto childId : entity.Children())
		{
			Entity child = mEntityScene->FindEntityByUUID(childId);

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

		mActors.push_back(actor);
		mPhysicsScene->addActor(*actor->mRigidActor);

		return actor;
	}

	void PhysicsScene::RemoveActor(Ref<PhysicsActor> actor)
	{
		NR_PROFILE_FUNC();

		if (!actor)
		{
			return;
		}

		for (auto& collider : actor->mColliders)
		{
			collider->DetachFromActor(actor->mRigidActor);
			collider->Release();
		}

		mPhysicsScene->removeActor(*actor->mRigidActor);
		actor->mRigidActor->release();
		actor->mRigidActor = nullptr;

		for (auto it = mActors.begin(); it != mActors.end(); it++)
		{
			if ((*it)->GetEntity() == actor->GetEntity())
			{
				mActors.erase(it);
				break;
			}
		}
	}

	Ref<PhysicsController> PhysicsScene::GetController(Entity entity)
	{
		for (auto& controller : mControllers)
		{
			if (controller->GetEntity() == entity)
				return controller;
		}

		return nullptr;
	}

	Ref<PhysicsController> PhysicsScene::CreateController(Entity entity)
	{
		auto existingController = GetController(entity);
		if (existingController)
		{
			return existingController;
		}

		Ref<PhysicsController> controller = Ref<PhysicsController>::Create(entity);

		const auto& transformComponent = entity.GetComponent<TransformComponent>();
		const auto& characterControllerComponent = entity.GetComponent<CharacterControllerComponent>();

		if (entity.HasComponent<CapsuleColliderComponent>())
		{
			const auto& capsuleColliderComponent = entity.GetComponent<CapsuleColliderComponent>();

			Ref<PhysicsMaterial> mat = AssetManager::GetAsset<PhysicsMaterial>(capsuleColliderComponent.Material);
			if (!mat)
			{
				mat = Ref<PhysicsMaterial>::Create(0.6f, 0.6f, 0.0f);
			}

			controller->mMaterial = PhysicsInternal::GetPhysicsSDK().createMaterial(mat->StaticFriction, mat->DynamicFriction, mat->Bounciness);

			float radiusScale = glm::max(transformComponent.Scale.x, transformComponent.Scale.z);

			physx::PxCapsuleControllerDesc desc;
			desc.position = PhysicsUtils::ToPhysicsExtendedVector(entity.Transform().Translation + capsuleColliderComponent.Offset); // not convinced this is correct.  (e.g. it needs to be world space, not local)
			desc.height = capsuleColliderComponent.Height * transformComponent.Scale.y;
			desc.radius = capsuleColliderComponent.Radius * radiusScale;
			desc.nonWalkableMode = physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING;  // TODO: get from component
			desc.climbingMode = physx::PxCapsuleClimbingMode::eCONSTRAINED;
			desc.slopeLimit = std::max(0.0f, cos(glm::radians(characterControllerComponent.SlopeLimitDeg)));
			desc.stepOffset = characterControllerComponent.StepOffset;
			desc.contactOffset = 0.01;                                                     // TODO: get from component
			desc.material = controller->mMaterial;
			desc.upDirection = { 0.0f, 1.0f, 0.0f };

			NR_CORE_VERIFY(controller->mController = mPhysicsControllerManager->createController(desc));
		}
		else if (entity.HasComponent<BoxColliderComponent>())
		{
			const auto& boxColliderComponent = entity.GetComponent<BoxColliderComponent>();

			Ref<PhysicsMaterial> mat = AssetManager::GetAsset<PhysicsMaterial>(boxColliderComponent.Material);
			if (!mat)
			{
				mat = Ref<PhysicsMaterial>::Create(0.6f, 0.6f, 0.0f);
			}

			controller->mMaterial = PhysicsInternal::GetPhysicsSDK().createMaterial(mat->StaticFriction, mat->DynamicFriction, mat->Bounciness);

			physx::PxBoxControllerDesc desc;
			desc.position = PhysicsUtils::ToPhysicsExtendedVector(entity.Transform().Translation + boxColliderComponent.Offset); // not convinced this is correct.  (e.g. it needs to be world space, not local)
			desc.halfHeight = (boxColliderComponent.Size.y * transformComponent.Scale.y) / 2.0f;
			desc.halfSideExtent = (boxColliderComponent.Size.x * transformComponent.Scale.x) / 2.0f;
			desc.halfForwardExtent = (boxColliderComponent.Size.z * transformComponent.Scale.z) / 2.0f;
			desc.nonWalkableMode = physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING;  // TODO: get from component
			desc.slopeLimit = std::max(0.0f, cos(glm::radians(characterControllerComponent.SlopeLimitDeg)));
			desc.stepOffset = characterControllerComponent.StepOffset;
			desc.contactOffset = 0.01;                                                     // TODO: get from component
			desc.material = controller->mMaterial;
			desc.upDirection = { 0.0f, 1.0f, 0.0f };

			NR_CORE_VERIFY(controller->mController = mPhysicsControllerManager->createController(desc));
		}

		controller->mHasGravity = !characterControllerComponent.DisableGravity;
		controller->mController->getActor()->userData = controller.Raw();

		controller->SetSimulationData(entity.GetComponent<CharacterControllerComponent>().Layer);

		mControllers.push_back(controller);
		return controller;
	}

	void PhysicsScene::RemoveController(Ref<PhysicsController> controller)
	{
		if (!controller || !controller->mController)
		{
			return;
		}

		controller->mController->release();
		controller->mController = nullptr;

		for (auto it = mControllers.begin(); it != mControllers.end(); it++)
		{
			if ((*it)->GetEntity() == controller->GetEntity())
			{
				mControllers.erase(it);
				break;
			}
		}
	}

	Ref<JointBase> PhysicsScene::GetJoint(Entity entity)
	{
		for (auto& joint : mJoints)
		{
			if (joint->GetEntity() == entity)
			{
				return joint;
			}
		}

		return nullptr;
	}

	Ref<JointBase> PhysicsScene::CreateJoint(Entity entity)
	{
		auto existingJoint = GetJoint(entity);
		if (existingJoint)
		{
			return existingJoint;
		}

		Ref<JointBase> joint = nullptr;

		const auto& fixedJointComponent = entity.GetComponent<FixedJointComponent>();
		Entity connectedEntity = mEntityScene->FindEntityByID(fixedJointComponent.ConnectedEntity);
		NR_CORE_VERIFY(connectedEntity);

		if (entity.HasComponent<FixedJointComponent>())
		{
			joint = Ref<FixedJoint>::Create(entity, connectedEntity);
		}

		if (!joint || !joint->IsValid())
		{
			return nullptr;
		}

		mJoints.push_back(joint);
		return joint;
	}

	void PhysicsScene::RemoveJoint(Ref<JointBase> joint)
	{
		if (!joint || !joint->IsValid())
		{
			return;
		}

		joint->Release();

		for (auto it = mJoints.begin(); it != mJoints.end(); it++)
		{
			if ((*it)->GetEntity() == joint->GetEntity())
			{
				mJoints.erase(it);
				break;
			}
		}
	}

	bool PhysicsScene::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit* outHit)
	{
		NR_PROFILE_FUNC();

		physx::PxRaycastBuffer hitInfo;
		bool result = mPhysicsScene->raycast(PhysicsUtils::ToPhysicsVector(origin), PhysicsUtils::ToPhysicsVector(glm::normalize(direction)), maxDistance, hitInfo);
		if (result)
		{
			Ref<PhysicsActorBase> object = (PhysicsActorBase*)hitInfo.block.actor->userData;
			outHit->HitEntity = object->GetEntity().GetID();
			outHit->Position = PhysicsUtils::FromPhysicsVector(hitInfo.block.position);
			outHit->Normal = PhysicsUtils::FromPhysicsVector(hitInfo.block.normal);
			outHit->Distance = hitInfo.block.distance;
		}
		return result;
	}

	bool PhysicsScene::OverlapBox(const glm::vec3& origin, const glm::vec3& halfSize, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		return OverlapGeometry(origin, physx::PxBoxGeometry(halfSize.x, halfSize.y, halfSize.z), buffer, count);
	}

	bool PhysicsScene::OverlapCapsule(const glm::vec3& origin, float radius, float halfHeight, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		return OverlapGeometry(origin, physx::PxCapsuleGeometry(radius, halfHeight), buffer, count);
	}

	bool PhysicsScene::OverlapSphere(const glm::vec3& origin, float radius, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		return OverlapGeometry(origin, physx::PxSphereGeometry(radius), buffer, count);
	}

	void PhysicsScene::AddRadialImpulse(const glm::vec3& origin, float radius, float strength, EFalloffMode falloff, bool velocityChange)
	{
		NR_PROFILE_FUNC();
		std::array<OverlapHit, OVERLAP_MAX_COLLIDERS> overlappedColliders;
		memset(overlappedColliders.data(), 0, OVERLAP_MAX_COLLIDERS * sizeof(OverlapHit));
		uint32_t count = 0;

		if (!OverlapSphere(origin, radius, overlappedColliders, count))
		{
			return;
		}

		for (uint32_t i = 0; i < count; ++i)
		{
			auto actorBase = overlappedColliders[i].Actor;
			if (actorBase->GetType() == PhysicsActorBase::Type::Actor)
			{
				auto actor = actorBase.As<PhysicsActor>();
				if (actor->IsDynamic() && !actor->IsKinematic())
				{
					actor->AddRadialImpulse(origin, radius, strength, falloff, velocityChange);
				}
			}
		}
	}

	static std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS> sOverlapBuffer;
	bool PhysicsScene::OverlapGeometry(const glm::vec3& origin, const physx::PxGeometry& geometry, std::array<OverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t& count)
	{
		NR_PROFILE_FUNC();

		physx::PxOverlapBuffer buf(sOverlapBuffer.data(), OVERLAP_MAX_COLLIDERS);
		physx::PxTransform pose = PhysicsUtils::ToPhysicsTransform(glm::translate(glm::mat4(1.0f), origin));

		bool result = mPhysicsScene->overlap(geometry, pose, buf);
		if (result)
		{
			count = buf.nbTouches > OVERLAP_MAX_COLLIDERS ? OVERLAP_MAX_COLLIDERS : buf.nbTouches;
			for (uint32_t i = 0; i < count; ++i)
			{
				buffer[i].Actor = (PhysicsActorBase*)sOverlapBuffer[i].actor->userData;
				buffer[i].Shape = (ColliderShape*)sOverlapBuffer[i].shape->userData;
			}
		}
		return result;
	}

	void PhysicsScene::ImGuiRender(bool& show)
	{
		NR_PROFILE_FUNC();

		if (!show)
		{
			return;
		}

		ImGui::Begin("Physics Stats", &show);

		if (IsValid())
		{
			auto gravity = GetGravity();
			std::string gravityString = fmt::format("X: {0}, Y: {1}, Z: {2}", gravity.x, gravity.y, gravity.z);
			ImGui::Text("Gravity: %s", gravityString.c_str());

			ImGui::Text("Actors: %d", mActors.size());



			for (const auto& actor : mActors)
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
						float maxVelocity = actor->GetMaxVelocity();
						glm::vec3 angularVelocity = actor->GetAngularVelocity();
						float maxAngularVelocity = actor->GetMaxAngularVelocity();
						UI::Property("Linear Velocity", linearVelocity);
						UI::Property("Max Linear Velocity", maxVelocity);
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

			ImGui::Text("Joints: %d", mJoints.size());

			for (const auto& joint : mJoints)
			{
				UUID id = joint->GetEntity().GetID();
				std::string label = fmt::format("{0} ({1})##{1}", joint->GetDebugName(), joint->GetEntity().GetComponent<TagComponent>().Tag, id);
				bool open = UI::PropertyGridHeader(label, false);
				if (open)
				{
					UI::BeginPropertyGrid();
					UI::PushItemDisabled();

					bool isBreakable = joint->IsBreakable();
					UI::Property("Is Breakable", isBreakable);

					if (isBreakable)
					{
						bool isBroken = joint->IsBroken();
						UI::Property("Is Broken", isBroken);

						float breakForce, breakTorque;
						joint->GetBreakForceAndTorque(breakForce, breakTorque);
						UI::Property("Break Force", breakForce);
						UI::Property("Break Torque", breakTorque);
					}

					bool isCollisionEnabled = joint->IsCollisionEnabled();
					UI::Property("Is Collision Enabled", isCollisionEnabled);

					bool isPreProcessingEnabled = joint->IsPreProcessingEnabled();
					UI::Property("Is Preprocessing Enabled", isPreProcessingEnabled);

					UI::PopItemDisabled();
					UI::EndPropertyGrid();

					ImGui::TreePop();
				}
			}
		}

		ImGui::End();
	}

	void PhysicsScene::InitializeScene(const Ref<Scene>& scene)
	{
		mEntityScene = scene;

		{
			auto view = mEntityScene->GetAllEntitiesWith<RigidBodyComponent>();

			for (auto entity : view)
			{
				Entity e = { entity, mEntityScene.Raw() };
				CreateActor(e);
			}
		}

		{
			auto view = mEntityScene->GetAllEntitiesWith<CharacterControllerComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, mEntityScene.Raw() };
				CreateController(e);
			}
		}

		{
			auto view = mEntityScene->GetAllEntitiesWith<TransformComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, mEntityScene.Raw() };

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
				while (Entity parent = mEntityScene->FindEntityByID(current.GetParentID()))
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
				rigidbody.BodyType = RigidBodyComponent::Type::Static;
				CreateActor(e);
			}
		}

		// Joints
		{
			auto view = mEntityScene->GetAllEntitiesWith<FixedJointComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, mEntityScene.Raw() };
				CreateJoint(e);
			}
		}
	}

	void PhysicsScene::Clear()
	{
		NR_CORE_ASSERT(mPhysicsScene);

		for (auto& joint : mJoints)
		{
			RemoveJoint(joint);
		}

		for (auto& controller : mControllers)
		{
			RemoveController(controller);
		}

		for (auto& actor : mActors)
		{
			RemoveActor(actor);
		}

		mControllers.clear();
		mActors.clear();

		mEntityScene = nullptr;
	}

	void PhysicsScene::CreateRegions()
	{
		const PhysicsSettings& settings = PhysicsManager::GetSettings();

		if (settings.BroadphaseAlgorithm == BroadphaseType::AutomaticBoxPrune)
			return;

		physx::PxBounds3* regionBounds = new physx::PxBounds3[settings.WorldBoundsSubdivisions * settings.WorldBoundsSubdivisions];
		physx::PxBounds3 globalBounds(PhysicsUtils::ToPhysicsVector(settings.WorldBoundsMin), PhysicsUtils::ToPhysicsVector(settings.WorldBoundsMax));
		uint32_t regionCount = physx::PxBroadPhaseExt::createRegionsFromWorldBounds(regionBounds, globalBounds, settings.WorldBoundsSubdivisions);

		for (uint32_t i = 0; i < regionCount; ++i)
		{
			physx::PxBroadPhaseRegion region;
			region.mBounds = regionBounds[i];
			mPhysicsScene->addBroadPhaseRegion(region);
		}
	}
}