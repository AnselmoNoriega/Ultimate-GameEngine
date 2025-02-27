#include "nrpch.h"
#include "ContactListener.h"

#include "PhysicsActor.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"
#include "NotRed/Script/ScriptEngine.h"
#include "PhysicsJoints.h"

namespace NR
{
	void ContactListener::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			physx::PxJoint* nativeJoint = (physx::PxJoint*)constraints[i].externalReference;
			Ref<JointBase> joint = (JointBase*)nativeJoint->userData;

			Entity entity = joint->GetEntity();
			Entity connectedEntity = joint->GetConnectedEntity();

			bool actorAScriptModuleValid = ScriptEngine::IsEntityModuleValid(entity);
			bool actorBScriptModuleValid = ScriptEngine::IsEntityModuleValid(connectedEntity);

			glm::vec3 linearForce = joint->GetLastReportedLinearForce();
			glm::vec3 angularForce = joint->GetLastReportedAngularForce();

			if (actorAScriptModuleValid)
			{
				ScriptEngine::JointBreak(entity, linearForce, angularForce);
			}

			if (actorBScriptModuleValid)
			{
				ScriptEngine::JointBreak(connectedEntity, linearForce, angularForce);
			}
		}
	}

	void ContactListener::onWake(physx::PxActor** actors, physx::PxU32 count)
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			physx::PxActor& physxActor = *actors[i];
			Ref<PhysicsActorBase> actor = (PhysicsActorBase*)physxActor.userData;
			NR_CORE_INFO("PhysX Actor waking up: ID: {0}, Name: {1}", actor->GetEntity().GetID(), actor->GetEntity().GetComponent<TagComponent>().Tag);
		}
	}

	void ContactListener::onSleep(physx::PxActor** actors, physx::PxU32 count)
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			physx::PxActor& physxActor = *actors[i];
			Ref<PhysicsActorBase> actor = (PhysicsActorBase*)physxActor.userData;
			NR_CORE_INFO("PhysX Actor going to sleep: ID: {0}, Name: {1}", actor->GetEntity().GetID(), actor->GetEntity().GetComponent<TagComponent>().Tag);
		}
	}

	void ContactListener::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
	{
		if (!ScriptEngine::GetCurrentSceneContext()->IsPlaying())
		{
			return;
		}

		auto removedActorA = pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0;
		auto removedActorB = pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1;

		if (removedActorA || removedActorB)
		{
			return;
		}

		Ref<PhysicsActorBase> actorA = (PhysicsActorBase*)pairHeader.actors[0]->userData;
		Ref<PhysicsActorBase> actorB = (PhysicsActorBase*)pairHeader.actors[1]->userData;

		if (!actorA || !actorB)
		{
			return;
		}

		bool actorAScriptModuleValid = ScriptEngine::IsEntityModuleValid(actorA->GetEntity());
		bool actorBScriptModuleValid = ScriptEngine::IsEntityModuleValid(actorB->GetEntity());
		if (!actorAScriptModuleValid && !actorBScriptModuleValid)
		{
			return;
		}

		if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
		{
			if (actorAScriptModuleValid)
			{
				ScriptEngine::CollisionBegin(actorA->GetEntity(), actorB->GetEntity());
			}
			if (actorBScriptModuleValid)
			{
				ScriptEngine::CollisionBegin(actorB->GetEntity(), actorA->GetEntity());
			}
		}
		else if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
		{
			if (actorAScriptModuleValid)
			{
				ScriptEngine::CollisionEnd(actorA->GetEntity(), actorB->GetEntity());
			}
			if (actorBScriptModuleValid)
			{
				ScriptEngine::CollisionEnd(actorB->GetEntity(), actorA->GetEntity());
			}
		}
	}

	void ContactListener::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
	{
		if (!ScriptEngine::GetCurrentSceneContext()->IsPlaying())
		{
			return;
		}

		for (uint32_t i = 0; i < count; i++)
		{
			if (pairs[i].flags & (physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
			{
				continue;
			}

			Ref<PhysicsActorBase> triggerActor = (PhysicsActorBase*)pairs[i].triggerActor->userData;
			Ref<PhysicsActorBase> otherActor = (PhysicsActorBase*)pairs[i].otherActor->userData;
			
			if (!triggerActor || !otherActor)
			{
				continue;
			}

			bool triggerActorScriptModuleValid = ScriptEngine::IsEntityModuleValid(triggerActor->GetEntity());
			bool otherActorScriptModuleValid = ScriptEngine::IsEntityModuleValid(otherActor->GetEntity());
			if (!triggerActorScriptModuleValid && !otherActorScriptModuleValid)
			{
				continue;
			}

			if (pairs[i].status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
			{
				if (triggerActorScriptModuleValid)
				{
					ScriptEngine::TriggerBegin(triggerActor->GetEntity(), otherActor->GetEntity());
				}
				if (otherActorScriptModuleValid)
				{
					ScriptEngine::TriggerBegin(otherActor->GetEntity(), triggerActor->GetEntity());
				}
			}
			else if (pairs[i].status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
			{
				if (triggerActorScriptModuleValid)
				{
					ScriptEngine::TriggerEnd(triggerActor->GetEntity(), otherActor->GetEntity());
				}
				if (otherActorScriptModuleValid)
				{
					ScriptEngine::TriggerEnd(otherActor->GetEntity(), triggerActor->GetEntity());
				}
			}
		}
	}

	void ContactListener::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
	{
		PX_UNUSED(bodyBuffer);
		PX_UNUSED(poseBuffer);
		PX_UNUSED(count);
	}
}