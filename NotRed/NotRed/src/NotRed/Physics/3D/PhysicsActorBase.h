#pragma once

#include "NotRed/Scene/Entity.h"

namespace NR
{
	class PhysicsActorBase : public RefCounted
	{
	public:
		enum class Type { None = -1, Actor, Controller };

	public:
		Entity GetEntity() const { return mEntity; }

		virtual void SetSimulationData(uint32_t layerId) = 0;

		Type GetType() const { return mType; }

	private:
		virtual void SynchronizeTransform() = 0;

	protected:
		PhysicsActorBase(Type type, Entity entity)
			: mType(type), mEntity(entity) {}

	protected:
		Entity mEntity;

	private:
		Type mType = Type::None;

		friend class PhysicsScene;
	};
}