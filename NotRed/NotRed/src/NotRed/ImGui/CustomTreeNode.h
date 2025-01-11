#pragma once

#include <nrpch.h>

#include "NotRed/ImGui/ImGui.h"

#include "imgui.h"
#include "imgui_internal.h"


namespace ImGui
{
    bool TreeNodeWithIcon(NR::Ref<NR::Texture2D> icon, ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end, ImColor iconTint = IM_COL32_WHITE);
    bool TreeNodeWithIcon(NR::Ref<NR::Texture2D> icon, const void* ptr_id, ImGuiTreeNodeFlags flags, ImColor iconTint, const char* fmt, ...);
    bool TreeNodeWithIcon(NR::Ref<NR::Texture2D> icon, const char* label, ImGuiTreeNodeFlags flags, ImColor iconTint = IM_COL32_WHITE);
}