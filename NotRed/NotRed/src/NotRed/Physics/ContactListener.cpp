#include "nrpch.h"
#include "ContactListener.h"

#include "PhysicsActor.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"
#include "NotRed/Script/ScriptEngine.h"

namespace NR
{
	void ContactListener::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
	{
		PX_UNUSED(constraints);
		PX_UNUSED(count);
	}

	void ContactListener::onWake(physx::PxActor** actors, physx::PxU32 count)
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			physx::PxActor& physxActor = *actors[i];
			Ref<PhysicsActor> actor = (PhysicsActor*)physxActor.userData;
			NR_CORE_INFO("PhysX Actor waking up: ID: {0}, Name: {1}", actor->GetEntity().GetID(), actor->GetEntity().GetComponent<TagComponent>().Tag);
		}
	}

	void ContactListener::onSleep(physx::PxActor** actors, physx::PxU32 count)
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			physx::PxActor& physxActor = *actors[i];
			Ref<PhysicsActor> actor = (PhysicsActor*)physxActor.userData;
			NR_CORE_INFO("PhysX Actor going to sleep: ID: {0}, Name: {1}", actor->GetEntity().GetID(), actor->GetEntity().GetComponent<TagComponent>().Tag);
		}
	}

	void ContactListener::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
	{
		Ref<PhysicsActor> actorA = (PhysicsActor*)(pairHeader.actors[0]->userData);
		Ref<PhysicsActor> actorB = (PhysicsActor*)(pairHeader.actors[1]->userData);

		if (!Scene::GetScene(actorA->GetEntity().GetSceneID())->IsPlaying())
		{
			return;
		}

		if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
		{
			if (ScriptEngine::IsEntityModuleValid(actorA->GetEntity()))		ScriptEngine::CollisionBegin(actorA->GetEntity());
			if (ScriptEngine::IsEntityModuleValid(actorB->GetEntity()))		ScriptEngine::CollisionBegin(actorB->GetEntity());
		}
		else if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
		{
			if (ScriptEngine::IsEntityModuleValid(actorA->GetEntity()))		ScriptEngine::CollisionEnd(actorA->GetEntity());
			if (ScriptEngine::IsEntityModuleValid(actorB->GetEntity()))		ScriptEngine::CollisionEnd(actorB->GetEntity());
		}
	}

	void ContactListener::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
	{
		Ref<PhysicsActor> actorA = (PhysicsActor*)pairs->triggerActor->userData;
		Ref<PhysicsActor> actorB = (PhysicsActor*)pairs->otherActor->userData;

		if (!Scene::GetScene(actorA->GetEntity().GetSceneID())->IsPlaying())
		{
			return;
		}

		if (pairs->status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			if (ScriptEngine::IsEntityModuleValid(actorA->GetEntity()))		 ScriptEngine::TriggerBegin(actorA->GetEntity());
			if (ScriptEngine::IsEntityModuleValid(actorB->GetEntity()))		 ScriptEngine::TriggerBegin(actorB->GetEntity());
		}
		else if (pairs->status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
		{
			if (ScriptEngine::IsEntityModuleValid(actorA->GetEntity()))		 ScriptEngine::TriggerEnd(actorA->GetEntity());
			if (ScriptEngine::IsEntityModuleValid(actorB->GetEntity()))		 ScriptEngine::TriggerEnd(actorB->GetEntity());
		}
	}

	void ContactListener::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
	{
		PX_UNUSED(bodyBuffer);
		PX_UNUSED(poseBuffer);
		PX_UNUSED(count);
	}
}