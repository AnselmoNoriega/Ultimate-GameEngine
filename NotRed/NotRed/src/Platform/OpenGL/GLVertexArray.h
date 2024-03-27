#pragma once

#include "NotRed/Renderer/VertexArray.h"

namespace NR
{
    class GLVertexArray : public VertexArray
    {
    public:
        GLVertexArray();
        ~GLVertexArray() override;

        void Bind() const override;
        void Unbind() const override;

        void AddVertexBuffer(const Ref<VertexBuffer>& vertexbuffer) override;
        void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

        const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override { return mVertexBuffers; }
        const Ref<IndexBuffer>& GetIndexBuffer() const override { return mIndexBuffer; }

    private:
        uint32_t mID;
        uint32_t mVertexBufferIndex = 0;

        std::vector<Ref<VertexBuffer>> mVertexBuffers;
        Ref<IndexBuffer> mIndexBuffer;
    };
}

