#include "nrpch.h"
#include "Entity.h"

namespace NR
{
	Entity::Entity(const std::string& name)
		: mName(name), mTransform(1.0f)
	{

	}

	Entity::~Entity()
	{

	}

}