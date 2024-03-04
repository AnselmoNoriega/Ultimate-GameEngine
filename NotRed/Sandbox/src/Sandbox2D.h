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
};

