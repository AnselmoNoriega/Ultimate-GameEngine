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

    private:
        void DrawNode(Entity& entity);

    private:
        Ref<Scene> mContext;
        Entity mSelectionContext;
    };
}