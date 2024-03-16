#pragma once

#include "Entity.h"

namespace NR
{
    class ScriptableEntity
    {
    public:
        template<typename T>
        T& GetComponent()
        {
            return mEntity.GetComponent<T>();
        }

    private:
        Entity mEntity;
        friend class Scene;
    };
}