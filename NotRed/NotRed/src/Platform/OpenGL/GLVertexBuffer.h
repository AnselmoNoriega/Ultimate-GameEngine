#pragma once

#include "NotRed/Renderer/VertexBuffer.h"

namespace NR
{
    class GLVertexBuffer : public VertexBuffer
    {
    public:
        GLVertexBuffer(uint32_t size);
        GLVertexBuffer(float* vertices, uint32_t size);
        ~GLVertexBuffer() override;

        void Bind() const override;
        void Unbind() const override;

        const BufferLayout& GetLayout() const override { return mLayout; }
        void SetLayout(const BufferLayout& layout) override { mLayout = layout; }

        void SetData(const void* data, uint32_t size) override;

    private:
        uint32_t mID;
        BufferLayout mLayout;
    };
}