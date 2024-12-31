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
		auto removedActorA = pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0;
		auto removedActorB = pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1;

		Ref<PhysicsActor> actorA, actorB;
		if (!removedActorA)
		{
			actorA = (PhysicsActor*)pairHeader.actors[0]->userData;
		}
		if (!removedActorB)
		{
			actorB = (PhysicsActor*)pairHeader.actors[1]->userData;
		}
			
		Ref<PhysicsActor> validActor = actorA ? actorA : actorB;
		if (!validActor || !Scene::GetScene(validActor->GetEntity().GetSceneID())->IsPlaying())
		{
			return;
		}

		if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
		{
			if (!removedActorA && !removedActorB && ScriptEngine::IsEntityModuleValid(actorA->GetEntity())) ScriptEngine::CollisionBegin(actorA->GetEntity(), actorB->GetEntity());
			if (!removedActorB && !removedActorA && ScriptEngine::IsEntityModuleValid(actorB->GetEntity())) ScriptEngine::CollisionBegin(actorB->GetEntity(), actorA->GetEntity());
		}
		else if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
		{	
			if (!removedActorA && !removedActorB && ScriptEngine::IsEntityModuleValid(actorA->GetEntity())) ScriptEngine::CollisionEnd(actorA->GetEntity(), actorB->GetEntity());
			if (!removedActorB && !removedActorA && ScriptEngine::IsEntityModuleValid(actorB->GetEntity())) ScriptEngine::CollisionEnd(actorB->GetEntity(), actorA->GetEntity());
		}
	}

	void ContactListener::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
	{
		auto removedTrigger = pairs->flags & physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER;
		auto removedOther = pairs->flags & physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER;

		Ref<PhysicsActor> triggerActor, otherActor;
		if (!removedTrigger)
		{
			triggerActor = (PhysicsActor*)pairs->triggerActor->userData;
		}
		if (!removedOther)
		{
			otherActor = (PhysicsActor*)pairs->otherActor->userData;
		}

		Ref<PhysicsActor> validActor = triggerActor ? triggerActor : otherActor;
		if (!validActor || !Scene::GetScene(validActor->GetEntity().GetSceneID())->IsPlaying())
		{
			return;
		}

		if (pairs->status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			if (triggerActor && otherActor && ScriptEngine::IsEntityModuleValid(triggerActor->GetEntity()) && ScriptEngine::IsEntityModuleValid(otherActor->GetEntity())) ScriptEngine::TriggerBegin(triggerActor->GetEntity(), otherActor->GetEntity());
			if (otherActor && triggerActor && ScriptEngine::IsEntityModuleValid(triggerActor->GetEntity()) && ScriptEngine::IsEntityModuleValid(otherActor->GetEntity())) ScriptEngine::TriggerBegin(otherActor->GetEntity(), triggerActor->GetEntity());
		}
		else if (pairs->status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
		{
			if (triggerActor && otherActor && ScriptEngine::IsEntityModuleValid(triggerActor->GetEntity()) && ScriptEngine::IsEntityModuleValid(otherActor->GetEntity())) ScriptEngine::TriggerEnd(triggerActor->GetEntity(), otherActor->GetEntity());
			if (otherActor && triggerActor && ScriptEngine::IsEntityModuleValid(otherActor->GetEntity()) && ScriptEngine::IsEntityModuleValid(triggerActor->GetEntity())) ScriptEngine::TriggerEnd(otherActor->GetEntity(), triggerActor->GetEntity());
		}
	}

	void ContactListener::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
	{
		PX_UNUSED(bodyBuffer);
		PX_UNUSED(poseBuffer);
		PX_UNUSED(count);
	}
}