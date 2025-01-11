#pragma once

#include <vector>
#include <string>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/ImGui/Colors.h"
#include "NotRed/Util/StringUtils.h"

namespace NR::UI
{
    //=========================================================================================
    /// Utilities
    ImTextureID GetTextureID(Ref<Texture2D> texture);

    class ScopedStyle
    {
    public:
        ScopedStyle(const ScopedStyle&) = delete;
        ScopedStyle operator=(const ScopedStyle&) = delete;
        template<typename T>
        ScopedStyle(ImGuiStyleVar styleVar, T value) { ImGui::PushStyleVar(styleVar, value); }
        ~ScopedStyle() { ImGui::PopStyleVar(); }
    };

    class ScopedColor
    {
    public:
        ScopedColor(const ScopedColor&) = delete;
        ScopedColor operator=(const ScopedColor&) = delete;
        template<typename T>
        ScopedColor(ImGuiCol colorId, T color) { ImGui::PushStyleColor(colorId, color); }
        ~ScopedColor() { ImGui::PopStyleColor(); }
    };

    class ScopedFont
    {
    public:
        ScopedFont(const ScopedFont&) = delete;
        ScopedFont operator=(const ScopedFont&) = delete;
        ScopedFont(ImFont* font) { ImGui::PushFont(font); }
        ~ScopedFont() { ImGui::PopFont(); }
    };

    class ScopedID
    {
    public:
        ScopedID(const ScopedID&) = delete;
        ScopedID operator=(const ScopedID&) = delete;
        template<typename T>
        ScopedID(T id) { ImGui::PushID(id); }
        ~ScopedID() { ImGui::PopID(); }
    };

    class ScopedColorStack
    {
    public:
        ScopedColorStack(const ScopedColorStack&) = delete;
        ScopedColorStack operator=(const ScopedColorStack&) = delete;

        template <typename ColorType, typename... OtherColors>
        ScopedColorStack(ImGuiCol firstColorID, ColorType firstColor, OtherColors&& ... otherColorPairs)
            : mCount((sizeof... (otherColorPairs) / 2) + 1)
        {
            static_assert ((sizeof... (otherColorPairs) & 1u) == 0,
                "ScopedColorStack constructor expects a list of pairs of color IDs and colors as its arguments");

            PushColor(firstColorID, firstColor, std::forward<OtherColors>(otherColorPairs)...);
        }

        ~ScopedColorStack() { ImGui::PopStyleColor(mCount); }

    private:
        template <typename ColorType, typename... OtherColors>
        void PushColor(ImGuiCol colorID, ColorType color, OtherColors&& ... otherColorPairs)
        {
            if constexpr (sizeof... (otherColorPairs) == 0)
            {
                ImGui::PushStyleColor(colorID, color);
            }
            else
            {
                ImGui::PushStyleColor(colorID, color);
                PushColor(std::forward<OtherColors>(otherColorPairs)...);
            }
        }

    private:
        int mCount;
    };

    // The delay won't work on texts, because the timer isn't tracked for them.
    static bool IsItemHovered(float delayInSeconds = 0.1f, ImGuiHoveredFlags flags = 0)
    {
        return ImGui::IsItemHovered() && GImGui->HoveredIdTimer > delayInSeconds; /*HoveredIdNotActiveTimer*/
    }

    static void SetTooltip(std::string_view text, float delayInSeconds = 0.1f, bool allowWhenDisabled = true)
    {
        if (IsItemHovered(delayInSeconds, allowWhenDisabled ? ImGuiHoveredFlags_AllowWhenDisabled : 0))
        {
            UI::ScopedColor textCol(ImGuiCol_Text, Colors::Theme::textBrighter);
            ImGui::SetTooltip(text.data());
        }
    }

    //=========================================================================================
    /// Colors
    static ImU32 ColorWithValue(const ImColor& color, float value)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, sat, std::min(value, 1.0f));
    }

    static ImU32 ColorWithSaturation(const ImColor& color, float saturation)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, std::min(saturation, 1.0f), val);
    }

    static ImU32 ColorWithHue(const ImColor& color, float hue)
    {
        const ImVec4& colRow = color.Value;
        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, h, s, v);
        return ImColor::HSV(std::min(hue, 1.0f), s, v);
    }

    static ImU32 ColorWithMultipliedValue(const ImColor& color, float multiplier)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
    }

    static ImU32 ColorWithMultipliedSaturation(const ImColor& color, float multiplier)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, std::min(sat * multiplier, 1.0f), val);
    }

    static ImU32 ColorWithMultipliedHue(const ImColor& color, float multiplier)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(std::min(hue * multiplier, 1.0f), sat, val);
    }

    namespace Draw 
    {
        //=========================================================================================
        /// Lines
        static void Underline(bool fullWidth = false, float offsetX = 0.0f, float offsetY = -1.0f)
        {
            if (fullWidth)
            {
                if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
                {
                    ImGui::PushColumnsBackground();
                }
                else if (ImGui::GetCurrentTable() != nullptr)
                {
                    ImGui::TablePushBackgroundChannel();
                }
            }

            const float width = fullWidth ? ImGui::GetWindowWidth() : ImGui::GetContentRegionAvail().x;
            const ImVec2 cursor = ImGui::GetCursorScreenPos();

            ImGui::GetWindowDrawList()->AddLine(ImVec2(cursor.x + offsetX, cursor.y + offsetY),
                ImVec2(cursor.x + width, cursor.y + offsetY),
                Colors::Theme::backgroundDark, 1.0f
            );

            if (fullWidth)
            {
                if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
                {
                    ImGui::PopColumnsBackground();
                }
                else if (ImGui::GetCurrentTable() != nullptr)
                {
                    ImGui::TablePopBackgroundChannel();
                }
            }
        }
    }

    class ScopedStyleStack
    {
    public:
        ScopedStyleStack(const ScopedStyleStack&) = delete;
        ScopedStyleStack operator=(const ScopedStyleStack&) = delete;

        template <typename ValueType, typename... OtherStylePairs>
        ScopedStyleStack(ImGuiStyleVar firstStyleVar, ValueType firstValue, OtherStylePairs&& ... otherStylePairs)
            : mCount((sizeof... (otherStylePairs) / 2) + 1)
        {
            static_assert ((sizeof... (otherStylePairs) & 1u) == 0,
                "ScopedStyleStack constructor expects a list of pairs of color IDs and colors as its arguments");

            PushStyle(firstStyleVar, firstValue, std::forward<OtherStylePairs>(otherStylePairs)...);
        }

        ~ScopedStyleStack() { ImGui::PopStyleVar(mCount); }

    private:
        template <typename ValueType, typename... OtherStylePairs>
        void PushStyle(ImGuiStyleVar styleVar, ValueType value, OtherStylePairs&& ... otherStylePairs)
        {
            if constexpr (sizeof... (otherStylePairs) == 0)
            {
                ImGui::PushStyleVar(styleVar, value);
            }
            else
            {
                ImGui::PushStyleVar(styleVar, value);
                PushStyle(std::forward<OtherStylePairs>(otherStylePairs)...);
            }
        }

    private:
        int mCount;
    };


    //=========================================================================================
    /// Rectangle

    static inline ImRect GetItemRect()
    {
        return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    }

    static inline ImRect RectExpanded(const ImRect& rect, float x, float y)
    {
        ImRect result = rect;
        result.Min.x -= x;
        result.Min.y -= y;
        result.Max.x += x;
        result.Max.y += y;
        return result;
    }

    static inline ImRect RectOffset(const ImRect& rect, float x, float y)
    {
        ImRect result = rect;
        result.Min.x += x;
        result.Min.y += y;
        result.Max.x += x;
        result.Max.y += y;
        return result;
    }

    static inline ImRect RectOffset(const ImRect& rect, ImVec2 xy)
    {
        return RectOffset(rect, xy.x, xy.y);
    }

    //=========================================================================================
    /// Window
    static bool IsMouseCoveredByOtherWindow(const char* currentWindowName)
    {
        auto isWindowFocused = [&currentWindowName]
            {
                // Note: child name usually starts with "parent window name"/"child name"
                auto* scenHierarchyWindow = ImGui::FindWindowByName(currentWindowName);
                if (GImGui->NavWindow)
                {
                    return GImGui->NavWindow == scenHierarchyWindow || 
                        Utils::StartsWith(GImGui->NavWindow->Name, currentWindowName);
                }
                else
                {
                    return false;
                }
            };
        if (!isWindowFocused())
        {
            if (GImGui->NavWindow)
            {
                const ImRect otherWindowRect = UI::RectExpanded(GImGui->NavWindow->Rect(), 0.0f, 0.0f);
                if (ImGui::IsMouseHoveringRect(otherWindowRect.Min, otherWindowRect.Max))
                {
                    return true;
                }
            }
        }
        return false;
    };

    //=========================================================================================
    /// Shadows

    static void DrawShadow(
        const Ref<Texture2D>& shadowImage, 
        int radius, 
        ImVec2 rectMin, ImVec2 rectMax, 
        float alphMultiplier = 1.0f, float lengthStretch = 10.0f,
        bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true
    )
    {
        const float widthOffset = lengthStretch;
        const float alphaTop = std::min(0.25f * alphMultiplier, 1.0f);
        const float alphaSides = std::min(0.30f * alphMultiplier, 1.0f);
        const float alphaBottom = std::min(0.60f * alphMultiplier, 1.0f);
        const auto p1 = rectMin;
        const auto p2 = rectMax;

        ImTextureID textureID = GetTextureID(shadowImage);

        auto* drawList = ImGui::GetWindowDrawList();
        if (drawLeft)
        {
            drawList->AddImage(textureID, { p1.x - widthOffset,  p1.y - radius }, { p2.x + widthOffset, p1.y }, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImColor(0.0f, 0.0f, 0.0f, alphaTop));
        }
        if (drawRight)
        {
            drawList->AddImage(textureID, { p1.x - widthOffset,  p2.y }, { p2.x + widthOffset, p2.y + radius }, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), ImColor(0.0f, 0.0f, 0.0f, alphaBottom));
        }

        if (drawTop)
        {
            drawList->AddImageQuad(
                textureID, 
                { p1.x - radius, p1.y - widthOffset }, 
                { p1.x, p1.y - widthOffset }, 
                { p1.x, p2.y + widthOffset }, 
                { p1.x - radius, p2.y + widthOffset },
                { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, 
                ImColor(0.0f, 0.0f, 0.0f, alphaSides)
            );
        }
        if (drawBottom)
        {
            drawList->AddImageQuad(
                textureID, 
                { p2.x, p1.y - widthOffset }, 
                { p2.x + radius, p1.y - widthOffset }, 
                { p2.x + radius, p2.y + widthOffset }, 
                { p2.x, p2.y + widthOffset },
                { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, 
                ImColor(0.0f, 0.0f, 0.0f, alphaSides)
            );
        }
    };

    static void DrawShadow(
        const Ref<Texture2D>& shadowImage,
        int radius, ImRect rectangle, 
        float alphMultiplier = 1.0f, float lengthStretch = 10.0f,
        bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true
    )
    {
        DrawShadow(shadowImage, radius, rectangle.Min, rectangle.Max, alphMultiplier, lengthStretch, drawLeft, drawRight, drawTop, drawBottom);
    };


    static void DrawShadow(
        const Ref<Texture2D>& shadowImage, 
        int radius, 
        float alphMultiplier = 1.0f, float lengthStretch = 10.0f,
        bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true
    )
    {
        DrawShadow(shadowImage, radius, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), alphMultiplier, lengthStretch, drawLeft, drawRight, drawTop, drawBottom);
    };

    static void DrawShadowInner(
        const Ref<Texture2D>& shadowImage, 
        int radius, 
        ImVec2 rectMin, ImVec2 rectMax, 
        float alpha = 1.0f, float lengthStretch = 10.0f,
        bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true
    )
    {
        const float widthOffset = lengthStretch;
        const float alphaTop = alpha; //std::min(0.25f * alphMultiplier, 1.0f);
        const float alphaSides = alpha; //std::min(0.30f * alphMultiplier, 1.0f);
        const float alphaBottom = alpha; //std::min(0.60f * alphMultiplier, 1.0f);
        const auto p1 = ImVec2(rectMin.x + radius, rectMin.y + radius);
        const auto p2 = ImVec2(rectMax.x - radius, rectMax.y - radius);
        auto* drawList = ImGui::GetWindowDrawList();

        ImTextureID textureID = GetTextureID(shadowImage);

        if (drawTop)
        {
            drawList->AddImage(
                textureID, 
                { p1.x - widthOffset,  p1.y - radius }, 
                { p2.x + widthOffset, p1.y }, 
                ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), 
                ImColor(0.0f, 0.0f, 0.0f, alphaTop)
            );
        }
        if (drawBottom)
        {
            drawList->AddImage(
                textureID, 
                { p1.x - widthOffset,  p2.y }, 
                { p2.x + widthOffset, p2.y + radius }, 
                ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 
                ImColor(0.0f, 0.0f, 0.0f, alphaBottom)
            );
        }
        if (drawLeft)
        {
            drawList->AddImageQuad(
                textureID, 
                { p1.x - radius, p1.y - widthOffset }, 
                { p1.x, p1.y - widthOffset }, 
                { p1.x, p2.y + widthOffset }, 
                { p1.x - radius, p2.y + widthOffset },
                { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, 
                ImColor(0.0f, 0.0f, 0.0f, alphaSides)
            );
        }
        if (drawRight)
        {
            drawList->AddImageQuad(
                textureID, 
                { p2.x, p1.y - widthOffset }, 
                { p2.x + radius, p1.y - widthOffset }, 
                { p2.x + radius, p2.y + widthOffset }, 
                { p2.x, p2.y + widthOffset },
                { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f }, 
                ImColor(0.0f, 0.0f, 0.0f, alphaSides)
            );
        }
    };

    static void DrawShadowInner(
        const Ref<Texture2D>& shadowImage, 
        int radius, ImRect rectangle, 
        float alpha = 1.0f, float lengthStretch = 10.0f,
        bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true
    )
    {
        DrawShadowInner(shadowImage, radius, rectangle.Min, rectangle.Max, alpha, lengthStretch, drawLeft, drawRight, drawTop, drawBottom);
    };


    static void DrawShadowInner(
        const Ref<Texture2D>& shadowImage, 
        int radius, 
        float alpha = 1.0f, float lengthStretch = 10.0f,
        bool drawLeft = true, bool drawRight = true, bool drawTop = true, bool drawBottom = true
    )
    {
        DrawShadowInner(shadowImage, radius, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), alpha, lengthStretch, drawLeft, drawRight, drawTop, drawBottom);
    };


    //=========================================================================================
    /// Property Fields

    // GetColFunction function takes 'int' index of the option to display and returns ImColor
    template<typename GetColFunction>
    static bool PropertyDropdown(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected, GetColFunction getColorFunction)
    {
        const char* current = options[*selected].c_str();
        ImGui::Text(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        auto drawItemActivityOutline = [](float rounding = 0.0f, bool drawWhenInactive = false)
            {
                auto* drawList = ImGui::GetWindowDrawList();
                if (ImGui::IsItemHovered() && !ImGui::IsItemActive())
                {
                    drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                        ImColor(60, 60, 60), rounding, 0, 1.5f);
                }
                if (ImGui::IsItemActive())
                {
                    drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                        ImColor(80, 80, 80), rounding, 0, 1.0f);
                }
                else if (!ImGui::IsItemHovered() && drawWhenInactive)
                {
                    drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                        ImColor(50, 50, 50), rounding, 0, 1.0f);
                }
            };

        bool changed = false;

        const std::string id = "##" + std::string(label);

        ImGui::PushStyleColor(ImGuiCol_Text, getColorFunction(*selected).Value);

        if (ImGui::BeginCombo(id.c_str(), current))
        {
            for (int i = 0; i < optionCount; ++i)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, getColorFunction(i).Value);

                const bool is_selected = (current == options[i]);
                if (ImGui::Selectable(options[i].c_str(), is_selected))
                {
                    current = options[i].c_str();
                    *selected = i;
                    changed = true;
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();

                ImGui::PopStyleColor();
            }
            ImGui::EndCombo();

        }
        ImGui::PopStyleColor();

        drawItemActivityOutline(2.5f);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return changed;
    };

    static void PopupMenuHeader(const std::string& text, bool indentAfter = true, bool unindendBefore = false)
    {
        if (unindendBefore) ImGui::Unindent();
        ImGui::TextColored(ImColor(170, 170, 170).Value, text.c_str());
        ImGui::Separator();
        if (indentAfter) ImGui::Indent();
    };

    //=========================================================================================
    /// Button Image

    static void DrawButtonImage(
        const Ref<Texture2D>& imageNormal, const Ref<Texture2D>& imageHovered, const Ref<Texture2D>& imagePressed,
        ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
        ImVec2 rectMin, ImVec2 rectMax)
    {
        auto* drawList = ImGui::GetWindowDrawList();
        if (ImGui::IsItemActive())
        {
            drawList->AddImage(GetTextureID(imagePressed), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintPressed);
        }
        else if (ImGui::IsItemHovered())
        {
            drawList->AddImage(GetTextureID(imageHovered), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintHovered);
        }
        else
        {
            drawList->AddImage(GetTextureID(imageNormal), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintNormal);
        }
    };

    static void DrawButtonImage(
        const Ref<Texture2D>& imageNormal, const Ref<Texture2D>& imageHovered, const Ref<Texture2D>& imagePressed,
        ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
        ImRect rectangle
    )
    {
        DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
    };

    static void DrawButtonImage(
        const Ref<Texture2D>& image,
        ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
        ImVec2 rectMin, ImVec2 rectMax
    )
    {
        DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectMin, rectMax);
    };

    static void DrawButtonImage(
        const Ref<Texture2D>& image,
        ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
        ImRect rectangle
    )
    {
        DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
    };


    static void DrawButtonImage(
        const Ref<Texture2D>& imageNormal, const Ref<Texture2D>& imageHovered, const Ref<Texture2D>& imagePressed,
        ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed
    )
    {
        DrawButtonImage(imageNormal, imageHovered, imagePressed, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    };

    static void DrawButtonImage(
        const Ref<Texture2D>& image,
        ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed
    )
    {
        DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    };

    //=========================================================================================
    /// Border

    static void DrawBorder(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColor, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        auto min = rectMin;
        min.x -= thickness;
        min.y -= thickness;
        min.x += offsetX;
        min.y += offsetY;
        auto max = rectMax;
        max.x += thickness;
        max.y += thickness;
        max.x += offsetX;
        max.y += offsetY;

        auto* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(borderColor), 0.0f, 0, thickness);
    };

    static void DrawBorder(const ImVec4& borderColor, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        DrawBorder(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColor, thickness, offsetX, offsetY);
    };

    static void DrawBorder(float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        DrawBorder(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
    };

    static void DrawBorder(ImVec2 rectMin, ImVec2 rectMax, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        DrawBorder(rectMin, rectMax, ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
    };

    static void DrawBorderHorizontal(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColor, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        auto min = rectMin;
        min.y -= thickness;
        min.x += offsetX;
        min.y += offsetY;
        auto max = rectMax;
        max.y += thickness;
        max.x += offsetX;
        max.y += offsetY;

        auto* drawList = ImGui::GetWindowDrawList();
        const auto color = ImGui::ColorConvertFloat4ToU32(borderColor);
        drawList->AddLine(min, ImVec2(max.x, min.y), color, thickness);
        drawList->AddLine(ImVec2(min.x, max.y), max, color, thickness);
    };

    static void DrawBorderHorizontal(ImVec2 rectMin, ImVec2 rectMax, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        DrawBorderHorizontal(rectMin, rectMax, ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
    };

    static void DrawBorderHorizontal(const ImVec4& borderColor, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        DrawBorderHorizontal(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColor, thickness, offsetX, offsetY);
    };

    static void DrawBorderHorizontal(float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        DrawBorderHorizontal(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
    };

    static void DrawBorderVertical(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColor, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        auto min = rectMin;
        min.x -= thickness;
        min.x += offsetX;
        min.y += offsetY;
        auto max = rectMax;
        max.x += thickness;
        max.x += offsetX;
        max.y += offsetY;

        auto* drawList = ImGui::GetWindowDrawList();
        const auto color = ImGui::ColorConvertFloat4ToU32(borderColor);
        drawList->AddLine(min, ImVec2(min.x, max.y), color, thickness);
        drawList->AddLine(ImVec2(max.x, min.y), max, color, thickness);
    };

    static void DrawBorderVertical(const ImVec4& borderColor, float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        DrawBorderVertical(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColor, thickness, offsetX, offsetY);
    };

    static void DrawBorderVertical(float thickness = 1.0f, float offsetX = 0.0f, float offsetY = 0.0f)
    {
        DrawBorderVertical(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
    };

    static void DrawItemActivityOutline(float rounding = 0.0f, bool drawWhenInactive = false, ImColor colorWhenActive = ImColor(80, 80, 80))
    {
        auto* drawList = ImGui::GetWindowDrawList();
        const ImRect rect = RectExpanded(GetItemRect(), 1.0f, 1.0f);
        if (ImGui::IsItemHovered() && !ImGui::IsItemActive())
        {
            drawList->AddRect(rect.Min, rect.Max,
                ImColor(60, 60, 60), rounding, 0, 1.5f);
        }
        if (ImGui::IsItemActive())
        {
            drawList->AddRect(rect.Min, rect.Max,
                colorWhenActive, rounding, 0, 1.0f);
        }
        else if (!ImGui::IsItemHovered() && drawWhenInactive)
        {
            drawList->AddRect(rect.Min, rect.Max,
                ImColor(50, 50, 50), rounding, 0, 1.0f);
        }
    };

    //=========================================================================================
    /// Cursor

    static void ShiftCursorX(float distance)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + distance);
    }

    static void ShiftCursorY(float distance)
    {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + distance);
    }

    static void ShiftCursor(float x, float y)
    {
        const ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursor.x + x, cursor.y + y));
    }
}