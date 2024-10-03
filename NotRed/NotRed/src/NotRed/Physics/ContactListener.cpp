#include "nrpch.h"
#include "ContactListener.h"

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
			physx::PxActor& actor = *actors[i]; Entity& entity = *(Entity*)actor.userData;
			NR_CORE_INFO("Physics Actor waking up: ID: {0}, Name: {1}", (uint64_t)entity.GetID(), entity.GetComponent<TagComponent>().Tag);
		}
	}

	void ContactListener::onSleep(physx::PxActor** actors, physx::PxU32 count)
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			physx::PxActor& actor = *actors[i]; 
			Entity& entity = *(Entity*)actor.userData; 
			NR_CORE_INFO("Physics Actor going to sleep: ID: {0}, Name: {1}", (uint64_t)entity.GetID(), entity.GetComponent<TagComponent>().Tag);
		}
	}

	void ContactListener::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
	{
		Entity& a = *(Entity*)pairHeader.actors[0]->userData;
		Entity& b = *(Entity*)pairHeader.actors[1]->userData;

		if (!Scene::GetScene(a.GetSceneID())->IsPlaying())
		{
			return;
		}

		if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
		{
			if (ScriptEngine::IsEntityModuleValid(a)) ScriptEngine::CollisionBegin(a);
			if (ScriptEngine::IsEntityModuleValid(b)) ScriptEngine::CollisionBegin(b);
		}
		else if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
		{
			if (ScriptEngine::IsEntityModuleValid(a)) ScriptEngine::CollisionEnd(a);
			if (ScriptEngine::IsEntityModuleValid(b)) ScriptEngine::CollisionEnd(b);
		}
	}

	void ContactListener::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
	{
		Entity& a = *(Entity*)pairs->triggerActor->userData;
		Entity& b = *(Entity*)pairs->otherActor->userData;

		if (!Scene::GetScene(a.GetSceneID())->IsPlaying())
		{
			return;
		}

		if (pairs->status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			if (ScriptEngine::IsEntityModuleValid(a)) ScriptEngine::TriggerBegin(a);
			if (ScriptEngine::IsEntityModuleValid(b)) ScriptEngine::TriggerBegin(b);
		}
		else if (pairs->status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
		{
			if (ScriptEngine::IsEntityModuleValid(a)) ScriptEngine::TriggerEnd(a);
			if (ScriptEngine::IsEntityModuleValid(b)) ScriptEngine::TriggerEnd(b);
		}
	}

	void ContactListener::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
	{
		PX_UNUSED(bodyBuffer);
		PX_UNUSED(poseBuffer);
		PX_UNUSED(count);
	}
}