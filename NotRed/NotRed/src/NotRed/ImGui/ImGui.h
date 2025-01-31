#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "NotRed/Asset/AssetMetadata.h"

#include "imgui/imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include "NotRed/Renderer/Texture.h"
#include "NotRed/ImGui/Colors.h"
#include "NotRed/ImGui/ImGuiUtil.h"

#include "choc/text/choc_StringUtilities.h"

//TODO: This shouldn't be here
#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Scene/Entity.h"
#include "NotRed/Util/StringUtils.h"

namespace NR::UI
{
    static int sUIContextID = 0;
    static uint32_t sCounter = 0;
    static char sIDBuffer[16];

    static const char* GenerateID()
    {
        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
        return &sIDBuffer[0];
    }

    static bool IsMouseEnabled()
    {
        return ImGui::GetIO().ConfigFlags & ~ImGuiConfigFlags_NoMouse;
    }

    static void SetMouseEnabled(const bool enable)
    {
        if (enable)
        {
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        }
        else
        {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
        }
    }

    static void PushID()
    {
        ImGui::PushID(sUIContextID++);
        sCounter = 0;
    }

    static void PopID()
    {
        ImGui::PopID();
        sUIContextID--;
    }

    static void BeginPropertyGrid()
    {
        PushID();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        ImGui::Columns(2);
    }

    static bool PropertyGridHeader(const std::string& name, bool openByDefault = true)
    {
        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowItemOverlap
            | ImGuiTreeNodeFlags_FramePadding;

        if (openByDefault)
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        bool open = false;
        const float framePaddingX = 6.0f;
        const float framePaddingY = 6.0f;

        UI::ScopedStyle headerRounding(ImGuiStyleVar_FrameRounding, 0.0f);
        UI::ScopedStyle headerPaddingAndHeight(ImGuiStyleVar_FramePadding, ImVec2{ framePaddingX, framePaddingY });

        ImGui::PushID(name.c_str());
        open = ImGui::TreeNodeEx("##dummy_id", treeNodeFlags, Utils::ToUpper(name).c_str());
        ImGui::PopID();

        return open;
    }

    static void Separator()
    {
        ImGui::Separator();
    }

    static void PushItemDisabled()
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    }

    static bool IsItemDisabled()
    {
        return ImGui::GetItemFlags() & ImGuiItemFlags_Disabled;
    }

    static void PopItemDisabled()
    {
        ImGui::PopItemFlag();
    }

    static bool Property(const char* label, std::string& value)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        char buffer[256];
        strcpy_s<256>(buffer, value.c_str());

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::InputText(sIDBuffer, buffer, 256))
        {
            value = buffer;
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static void Property(const char* label, const std::string& value)
    {
        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        itoa(sCounter++, sIDBuffer + 2, 16);

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::InputText(sIDBuffer, (char*)value.c_str(), value.size(), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleVar();

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();
    }

    static void Property(const char* label, const char* value)
    {
        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::InputText(sIDBuffer, (char*)value, 256, ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleVar();

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();
    }

    static bool Property(const char* label, char* value, size_t length)
    {
        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        bool changed = ImGui::InputText(sIDBuffer, value, length);

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return changed;
    }

    static bool Property(const char* label, bool& value)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::Checkbox(sIDBuffer, &value))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool Property(const char* label, int& value, int min, int max)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::DragInt(sIDBuffer, &value, 1.0f, min, max))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool Property(const char* label, int& value)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::DragInt(sIDBuffer, &value))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool PropertySlider(const char* label, int& value, int min, int max)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::SliderInt(sIDBuffer, &value, min, max))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool PropertySlider(const char* label, float& value, float min, float max)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::SliderFloat(sIDBuffer, &value, min, max))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool PropertySlider(const char* label, glm::vec2& value, float min, float max)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::SliderFloat2(sIDBuffer, glm::value_ptr(value), min, max))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool PropertySlider(const char* label, glm::vec3& value, float min, float max)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::SliderFloat3(sIDBuffer, glm::value_ptr(value), min, max))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool PropertySlider(const char* label, glm::vec4& value, float min, float max)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::SliderFloat4(sIDBuffer, glm::value_ptr(value), min, max))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool Property(const char* label, float& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::DragFloat(sIDBuffer, &value, delta, min, max))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool Property(const char* label, glm::vec2& value, float delta = 0.1f)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::DragFloat2(sIDBuffer, glm::value_ptr(value), delta))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool PropertyColor(const char* label, glm::vec3& value)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::ColorEdit3(sIDBuffer, glm::value_ptr(value)))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool Property(const char* label, glm::vec3& value, float delta = 0.1f)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::DragFloat3(sIDBuffer, glm::value_ptr(value), delta))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    static bool Property(const char* label, glm::vec4& value, float delta = 0.1f)
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::DragFloat4(sIDBuffer, glm::value_ptr(value), delta))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return modified;
    }

    template<typename TEnum, typename TUnderlying = int32_t>
    static bool PropertyDropdown(const char* label, const char** options, int32_t optionCount, TEnum& selected)
    {
        TUnderlying selectedIndex = (TUnderlying)selected;
        const char* current = options[selectedIndex];
        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);
        bool changed = false;

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        const std::string id = "##" + std::string(label);
        if (ImGui::BeginCombo(id.c_str(), current))
        {
            for (int i = 0; i < optionCount; ++i)
            {
                const bool is_selected = (current == options[i]);
                if (ImGui::Selectable(options[i], is_selected))
                {
                    current = options[i];
                    selected = (TEnum)i;
                    changed = true;
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();
        return changed;
    }

    static bool PropertyDropdown(const char* label, const char** options, int32_t optionCount, int32_t* selected)
    {
        const char* current = options[*selected];
        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        bool changed = false;

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        const std::string id = "##" + std::string(label);
        if (ImGui::BeginCombo(id.c_str(), current))
        {
            for (int i = 0; i < optionCount; ++i)
            {
                const bool isselected = (current == options[i]);
                if (ImGui::Selectable(options[i], isselected))
                {
                    current = options[i];
                    *selected = i;
                    changed = true;
                }
                if (isselected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return changed;
    }

    static bool PropertyDropdown(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected)
    {
        const char* current = options[*selected].c_str();

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        bool changed = false;

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        const std::string id = "##" + std::string(label);
        if (ImGui::BeginCombo(id.c_str(), current))
        {
            for (int i = 0; i < optionCount; ++i)
            {
                const bool isselected = (current == options[i]);
                if (ImGui::Selectable(options[i].c_str(), isselected))
                {
                    current = options[i].c_str();
                    *selected = i;
                    changed = true;
                }
                if (isselected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return changed;
    }

    enum class PropertyAssetReferenceError
    {
        None = 0, InvalidMetadata
    };

    static AssetHandle sPropertyAssetReferenceAssetHandle;

    struct PropertyAssetReferenceSettings
    {
        bool AdvanceToNextColumn = true;
        bool NoItemSpacing = false; // After label
        float WidthOffset = 0.0f;
    };

    template<typename T>
    static bool PropertyAssetReference(const char* label, Ref<T>& object, PropertyAssetReferenceError* outError = nullptr, const PropertyAssetReferenceSettings& settings = PropertyAssetReferenceSettings())
    {
        bool modified = false;
        if (outError)
        {
            *outError = PropertyAssetReferenceError::None;
        }

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
        {
            ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
            float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
            float itemHeight = 28.0f;

            std::string buttonText = "Null";
            if (object)
            {
                if (!object->IsFlagSet(AssetFlag::Missing))
                {
                    buttonText = AssetManager::GetMetadata(object->Handle).FilePath.stem().string();
                }
                else
                {
                    buttonText = "Missing";
                }
            }
            ImGui::Button(fmt::format("{}##{}", buttonText, sCounter++).c_str(), { width, itemHeight });
        }
        ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

        if (ImGui::BeginDragDropTarget())
        {
            auto data = ImGui::AcceptDragDropPayload("asset_payload");

            if (data)
            {
                AssetHandle assetHandle = *(AssetHandle*)data->Data;
                sPropertyAssetReferenceAssetHandle = assetHandle;
                Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
                if (asset->GetAssetType() == T::GetStaticType())
                {
                    object = asset.As<T>();
                    modified = true;
                }
            }
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        if (settings.AdvanceToNextColumn)
        {
            ImGui::NextColumn();
        }

        return modified;
    }

    template<typename TAssetType, typename TConversionType, typename Fn>
    static bool PropertyAssetReferenceWithConversion(const char* label, Ref<TAssetType>& object, Fn&& conversionFunc, PropertyAssetReferenceError* outError = nullptr, const PropertyAssetReferenceSettings& settings = PropertyAssetReferenceSettings())
    {
        bool succeeded = false;
        if (outError)
        {
            *outError = PropertyAssetReferenceError::None;
        }

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
        ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
        float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
        UI::PushID();

        float itemHeight = 28.0f;

        if (object)
        {
            if (!object->IsFlagSet(AssetFlag::Missing))
            {
                std::string assetFileName = AssetManager::GetMetadata(object->Handle).FilePath.stem().string();
                ImGui::Button((char*)assetFileName.c_str(), { width, itemHeight });
            }
            else
            {
                ImGui::Button("Missing", { width, itemHeight });
            }
        }
        else
        {
            ImGui::Button("Null", { width, itemHeight });
        }

        UI::PopID();
        ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

        if (ImGui::BeginDragDropTarget())
        {
            auto data = ImGui::AcceptDragDropPayload("asset_payload");

            if (data)
            {
                AssetHandle assetHandle = *(AssetHandle*)data->Data;
                sPropertyAssetReferenceAssetHandle = assetHandle;
                Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
                if (asset)
                {
                    // No conversion necessary 
                    if (asset->GetAssetType() == TAssetType::GetStaticType())
                    {
                        object = asset.As<TAssetType>();
                        succeeded = true;
                    }
                    // Convert
                    else if (asset->GetAssetType() == TConversionType::GetStaticType())
                    {
                        conversionFunc(asset.As<TConversionType>());
                        succeeded = false; // Must be handled my conversion function
                    }
                }
                else
                {
                    if (outError)
                    {
                        *outError = PropertyAssetReferenceError::InvalidMetadata;
                    }
                }
            }
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return succeeded;
    }

    template<typename T, typename Fn>
    static bool PropertyAssetReferenceTarget(const char* label, const char* assetName, Ref<T> source, Fn&& targetFunc, const PropertyAssetReferenceSettings& settings = PropertyAssetReferenceSettings())
    {
        bool modified = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);
        if (settings.NoItemSpacing)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });
        }

        ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
        ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
        float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
        UI::PushID();

        float itemHeight = 28.0f;

        if (source)
        {
            if (!source->IsFlagSet(AssetFlag::Missing))
            {
                if (assetName)
                {
                    ImGui::Button((char*)assetName, { width, itemHeight });
                }
                else
                {
                    std::string assetFileName = AssetManager::GetMetadata(source->Handle).FilePath.stem().string();
                    assetName = assetFileName.c_str();
                    ImGui::Button((char*)assetName, { width, itemHeight });
                }
            }
            else
                ImGui::Button("Missing", { width, itemHeight });
        }
        else
        {
            ImGui::Button("Null", { width, itemHeight });
        }

        UI::PopID();
        ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

        if (ImGui::BeginDragDropTarget())
        {
            auto data = ImGui::AcceptDragDropPayload("asset_payload");

            if (data)
            {
                AssetHandle assetHandle = *(AssetHandle*)data->Data;
                sPropertyAssetReferenceAssetHandle = assetHandle;
                Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
                if (asset->GetAssetType() == T::GetStaticType())
                {
                    targetFunc(asset.As<T>());
                    modified = true;
                }
            }
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        if (settings.AdvanceToNextColumn)
        {
            ImGui::NextColumn();
        }
        if (settings.NoItemSpacing)
        {
            ImGui::PopStyleVar();
        }
        return modified;
    }

    static bool DrawComboPreview(const char* preview, float width = 100.0f)
    {
        bool pressed = false;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::BeginHorizontal("horizontal_node_layout", ImVec2(width, 0.0f));
        ImGui::PushItemWidth(90.0f);
        ImGui::InputText("##selected_asset", (char*)preview, 512, ImGuiInputTextFlags_ReadOnly);
        pressed = ImGui::IsItemClicked();
        ImGui::PopItemWidth();

        ImGui::PushItemWidth(10.0f);
        pressed = ImGui::ArrowButton("combo_preview_button", ImGuiDir_Down) || pressed;
        ImGui::PopItemWidth();

        ImGui::EndHorizontal();
        ImGui::PopStyleVar();

        return pressed;
    }

    template<typename TAssetType>
    static bool PropertyAssetDropdown(const char* label, Ref<TAssetType>& object, const ImVec2& size, AssetHandle* selected)
    {
        bool modified = false;
        std::string preview;
        float itemHeight = size.y / 10.0f;

        if (AssetManager::IsAssetHandleValid(*selected))
        {
            object = AssetManager::GetAsset<TAssetType>(*selected);
        }

        if (object)
        {
            if (!object->IsFlagSet(AssetFlag::Missing))
            {
                preview = AssetManager::GetMetadata(object->Handle).FilePath.stem().string();
            }
            else
            {
                preview = "Missing";
            }
        }
        else
        {
            preview = "Null";
        }

        auto& assets = AssetManager::GetLoadedAssets();
        AssetHandle current = *selected;

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        ImGui::SetNextWindowSize(size);
        if (UI::BeginPopup(label, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            ImGui::SetKeyboardFocusHere(0);

            for (auto& [handle, asset] : assets)
            {
                if (asset->GetAssetType() != TAssetType::GetStaticType())
                {
                    continue;
                }

                auto& metadata = AssetManager::GetMetadata(handle);

                bool isselected = (current == handle);
                if (ImGui::Selectable(metadata.FilePath.string().c_str(), isselected))
                {
                    current = handle;
                    *selected = handle;
                    modified = true;
                }
                if (isselected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            UI::EndPopup();
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        return modified;
    }

    static void EndPropertyGrid()
    {
        ImGui::Columns(1);
        UI::Draw::Underline();
        ImGui::PopStyleVar(2); // ItemSpacing, FramePadding
        UI::ShiftCursorY(18.0f);
        PopID();
    }

    static bool BeginTreeNode(const char* name, bool defaultOpen = true)
    {
        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
        if (defaultOpen)
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        return ImGui::TreeNodeEx(name, treeNodeFlags);
    }

    static void EndTreeNode()
    {
        ImGui::TreePop();
    }

    static int sCheckboxCount = 0;

    static void BeginCheckboxGroup(const char* label)
    {
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static bool PropertyCheckboxGroup(const char* label, bool& value)
    {
        bool modified = false;

        if (++sCheckboxCount > 1)
        {
            ImGui::SameLine();
        }

        ImGui::Text(label);
        ImGui::SameLine();

        sIDBuffer[0] = '#';
        sIDBuffer[1] = '#';
        memset(sIDBuffer + 2, 0, 14);
        sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

        if (IsItemDisabled())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if (ImGui::Checkbox(sIDBuffer, &value))
        {
            modified = true;
        }

        if (IsItemDisabled())
        {
            ImGui::PopStyleVar();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        return modified;
    }

    static bool Button(const char* label, const ImVec2& size = ImVec2(0, 0))
    {
        bool result = ImGui::Button(label, size);
        ImGui::NextColumn();
        return result;
    }

    static void EndCheckboxGroup()
    {
        ImGui::PopItemWidth();
        ImGui::NextColumn();
        sCheckboxCount = 0;
    }

    static bool PropertyEntityReference(const char* label, Entity& entity)
    {
        bool receivedValidEntity = false;

        ShiftCursor(10.0f, 9.0f);
        ImGui::Text(label);
        ImGui::NextColumn();
        ShiftCursorY(4.0f);
        ImGui::PushItemWidth(-1);

        ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
        {
            ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
            float width = ImGui::GetContentRegionAvail().x;
            float itemHeight = 28.0f;
            std::string buttonText = "Null";
            if (entity)
            {
                buttonText = entity.GetComponent<TagComponent>().Tag;
            }

            ImGui::Button(fmt::format("{}##{}", buttonText, sCounter++).c_str(), { width, itemHeight });
        }

        ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

        if (ImGui::BeginDragDropTarget())
        {
            auto data = ImGui::AcceptDragDropPayload("scene_entity_hierarchy");
            if (data)
            {
                entity = *(Entity*)data->Data;
                receivedValidEntity = true;
            }

            ImGui::EndDragDropTarget();
        }

        if (!IsItemDisabled())
        {
            DrawItemActivityOutline(2.0f, true, Colors::Theme::accent);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        Draw::Underline();

        return receivedValidEntity;
    }

    void Image(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
    void Image(const Ref<Image2D>& image, uint32_t layer, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
    void Image(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
    bool ImageButton(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
    bool ImageButton(const char* stringID, const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
    bool ImageButton(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
    bool ImageButton(const char* stringID, const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));

    template<uint32_t BuffSize = 256, typename StringType>
    static bool SearchWidget(StringType& searchString, const char* hint = "Search...")
    {
        UI::PushID();
        UI::ShiftCursorY(1.0f);

        const bool layoutSuspended = []
            {
                ImGuiWindow* window = ImGui::GetCurrentWindow();
                if (window->DC.CurrentLayout)
                {
                    ImGui::SuspendLayout();
                    return true;
                }
                return false;
            }();

        bool modified = false;
        bool searching = false;

        const float areaPosX = ImGui::GetCursorPosX();
        const float framePaddingY = ImGui::GetStyle().FramePadding.y;

        UI::ScopedStyle rounding(ImGuiStyleVar_FrameRounding, 3.0f);
        UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(28.0f, framePaddingY));

        if constexpr (std::is_same<StringType, std::string>::value)
        {

            char searchBuffer[BuffSize]{};
            strcpy_s<BuffSize>(searchBuffer, searchString.c_str());
            if (ImGui::InputText(GenerateID(), searchBuffer, BuffSize))
            {
                searchString = searchBuffer;
                modified = true;
            }
            searching = searchBuffer[0] != 0;
        }
        else
        {
            static_assert(std::is_same<decltype(&searchString[0]), char*>::value,
                "searchString paramenter must be std::string& or char*");
            if (ImGui::InputText(GenerateID(), searchString, BuffSize))
            {
                modified = true;
            }
            searching = searchString[0] != 0;
        }
        UI::DrawItemActivityOutline(3.0f, true, Colors::Theme::accent);
        ImGui::SetItemAllowOverlap();

        ImGui::SameLine(areaPosX + 5.0f);
        
        if (layoutSuspended)
        {
            ImGui::ResumeLayout();
        }

        ImGui::BeginHorizontal(GenerateID(), ImGui::GetItemRectSize());

        TextureProperties clampProps;
        clampProps.SamplerWrap = TextureWrap::Clamp;

        static Ref<Texture2D> sSearchIcon = Texture2D::Create("Resources/Editor/icon_search_24px.png", clampProps);
        static Ref<Texture2D> sClearIcon = Texture2D::Create("Resources/Editor/close.png", clampProps);
        const ImVec2 iconSize(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight());

        // Search icon
        {
            const float iconYOffset = framePaddingY - 3.0f;
            UI::ShiftCursorY(iconYOffset);
            UI::Image(sSearchIcon, iconSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
            UI::ShiftCursorY(-iconYOffset);

            if (!searching)
            {
                UI::ShiftCursorY(-framePaddingY + 1.0f);
                UI::ScopedColor text(ImGuiCol_Text, Colors::Theme::textDarker);
                UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(0.0f, framePaddingY));
                ImGui::TextUnformatted(hint);
                UI::ShiftCursorY(-1.0f);
            }
        }

        ImGui::Spring();
        if (searching)
        {
            const float spacingX = 4.0f;			
            const float lineHeight = ImGui::GetItemRectSize().y - framePaddingY / 2.0f;

            if (ImGui::InvisibleButton(GenerateID(), ImVec2{ lineHeight, lineHeight }))
            {
                if constexpr (std::is_same<StringType, std::string>::value)
                {
                    searchString.clear();
                }
                else
                {
                    memset(searchString, 0, BuffSize);
                }

                modified = true;
            }

            if (ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            }

            UI::DrawButtonImage(sClearIcon, IM_COL32(160, 160, 160, 200),
                IM_COL32(170, 170, 170, 255),
                IM_COL32(160, 160, 160, 150),
                UI::RectExpanded(UI::GetItemRect(), -2.0f, -2.0f));

            ImGui::Spring(-1.0f, spacingX * 2.0f);
        }

        ImGui::EndHorizontal();
        UI::ShiftCursorY(-1.0f);
        UI::PopID();

        return modified;
    }

    static bool IsMatchingSearch(const std::string& item, const std::string& searchQuery, bool caseSensitive = false, bool stripWhiteSpaces = true, bool stripUnderscores = true)
    {
        if (searchQuery.empty())
        {
            return true;
        }

        if (item.empty())
        {
            return false;
        }

        std::string nameSanitized = stripUnderscores ? choc::text::replace(item, "_", " ") : item;

        if (stripWhiteSpaces)
        {
            nameSanitized = choc::text::replace(nameSanitized, " ", "");
        }

        std::string searchString = stripWhiteSpaces ? choc::text::replace(searchQuery, " ", "") : searchQuery;
        if (!caseSensitive)
        {
            nameSanitized = Utils::String::ToLower(nameSanitized);
            searchString = Utils::String::ToLower(searchString);
        }

        return choc::text::contains(nameSanitized, searchString);
    }

    namespace Buttons
    {
        static bool Options()
        {
            const bool clicked = ImGui::InvisibleButton("##options", ImVec2{ ImGui::GetFrameHeight(), ImGui::GetFrameHeight() });
            
            static Ref<Texture2D> sGearIcon = Texture2D::Create("Resources/Editor/gear_icon.png");
            const float spaceAvail = std::min(ImGui::GetItemRectSize().x, ImGui::GetItemRectSize().y);
            constexpr float desiredIconSize = 15.0f;
            const float padding = std::max((spaceAvail - desiredIconSize) / 2.0f, 0.0f);
            constexpr auto buttonColor = Colors::Theme::text;
            const uint8_t value = uint8_t(ImColor(buttonColor).Value.x * 255);
            
            UI::DrawButtonImage(
                sGearIcon, IM_COL32(value, value, value, 200),
                IM_COL32(value, value, value, 255),
                IM_COL32(value, value, value, 150),
                UI::RectExpanded(UI::GetItemRect(), -padding, -padding));
            
            return clicked;
        }
    }
}