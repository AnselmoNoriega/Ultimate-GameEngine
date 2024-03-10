#pragma once

#include "NotRed.h"

class Sandbox2D : public NR::Layer
{
public:
    Sandbox2D();
    ~Sandbox2D() override = default;

    void Attach();
    void Detach();

    void Update(float deltaTime) override;
    void OnEvent(NR::Event& myEvent) override;
    void ImGuiRender() override;

private:
    NR::OrthographicCameraController mCameraController;

    NR::Ref<NR::Texture2D> mTexture;

    glm::vec4 mSquareColor = { 0.8f, 0.3f, 0.2f, 1.0f };

    glm::vec2 objPositions[2] = {
        { 1.0f, 0.0f },
        {-1.0f, 0.0f }
    };
};

