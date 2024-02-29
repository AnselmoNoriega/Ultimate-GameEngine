#pragma once

#include "NotRed/Renderer/RendererAPI.h"

namespace NR
{
    class GLRendererAPI : public RendererAPI
    {
    public:
        void SetClearColor(const glm::vec4& color) override;
        void Clear() override;

        void DrawIndexed(const Ref<VertexArray>& vertexArray) override;
    };
}