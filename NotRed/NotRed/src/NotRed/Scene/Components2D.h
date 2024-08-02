#pragma once

#include <glm/glm.hpp>

#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Text/Font.h"

namespace NR
{
    struct SpriteRendererComponent
    {
        glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        Ref<Texture2D> Texture;

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const glm::vec4& color)
            : Color(color) {};
    };

    struct CircleRendererComponent
    {
        glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
        float Thickness = 1.0f;

        CircleRendererComponent() = default;
        CircleRendererComponent(const CircleRendererComponent&) = default;
    };

    struct TextComponent
    {
        Ref<Font> TextFont = Font::GetDefault();
        std::string TextString;
        glm::vec4 Color{ 1.0f };
        float LineSpacing = 0.0f;
    };

    struct Rigidbody2DComponent
    {
        enum class BodyType { Static, Kinematic, Dynamic };

        BodyType Type = BodyType::Static;
        bool FixedRotation = false;

        void* RuntimeBody = nullptr;

        float Density = 1.0f;
        float Friction = 0.5f;
        float Restitution = 0.1f;
        float RestitutionThreshold = 0.5f;

        Rigidbody2DComponent() = default;
        Rigidbody2DComponent(const Rigidbody2DComponent&) = default;
    };

    struct BoxCollider2DComponent
    {
        glm::vec2 Offset = { 0.0f, 0.0f };
        glm::vec2 Size = { 0.5f, 0.5f };

        void* RuntimeFixture = nullptr;

        BoxCollider2DComponent() = default;
        BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
    };

    struct CircleCollider2DComponent
    {
        glm::vec2 Offset = { 0.0f, 0.0f };
        float Radius = 0.5f;

        void* RuntimeFixture = nullptr;

        CircleCollider2DComponent() = default;
        CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
    };
}