#pragma once

#include "NotRed/Core/Core.h"
#include "NotRed/Scene/Scene.h"

#include "NotRed/Scene/Entity.h"

namespace NR
{
    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel() = default;
        SceneHierarchyPanel(const Ref<Scene>& context);

        void SetContext(const Ref<Scene>& context);

        void ImGuiRender();

        Entity GetSelectedEntity() { return mSelectionContext; }

        void SetSelecetedEntity(Entity entity);

    private:
        void DrawNode(Entity& entity);
        void DrawComponents(Entity& entity);

    private:
        Ref<Scene> mContext;
        Entity mSelectionContext;
    };
}