#pragma once

#include <imgui/imgui.h>

namespace NR::UI
{
    struct StyleColor
    {
        StyleColor() = default;

        StyleColor(ImGuiCol idx, ImVec4 color, bool predicate = true)
            : mSet(predicate)
        {
            if (predicate)
            {
                ImGui::PushStyleColor(idx, color);
            }
        }

        StyleColor(ImGuiCol idx, ImU32 color, bool predicate = true)
            : mSet(predicate)
        {
            if (predicate)
            {
                ImGui::PushStyleColor(idx, color);
            }
        }

        ~StyleColor()
        {
            if (mSet)
            {
                ImGui::PopStyleColor();
            }
        }
    private:
        bool mSet = false;
    };


}