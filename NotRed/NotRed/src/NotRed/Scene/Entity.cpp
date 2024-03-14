#include "nrpch.h"
#include "Entity.h"

namespace NR
{
    Entity::Entity(entt::entity entity, Scene* scene)
        :mEntity(entity), mScene(scene)
    {
    }
}
