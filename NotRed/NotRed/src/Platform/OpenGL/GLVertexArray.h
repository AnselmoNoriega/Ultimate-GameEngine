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

        void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexbuffer) override;
        void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) override;

        const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const override { return mVertexBuffers; }
        const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const override { return mIndexBuffer; }

    private:
        uint32_t mID;

        std::vector<std::shared_ptr<VertexBuffer>> mVertexBuffers;
        std::shared_ptr<IndexBuffer> mIndexBuffer;
    };
}

