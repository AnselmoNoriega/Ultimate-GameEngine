#pragma once

#include "NotRed/Renderer/VertexBuffer.h"

namespace NR
{
    class GLVertexBuffer : public VertexBuffer
    {
    public:
        GLVertexBuffer(float* vertices, uint32_t size);
        ~GLVertexBuffer() override;

        void Bind() const override;
        void Unbind() const override;

    private:
        uint32_t mID;
    };
}