#pragma once

#include <memory>
#include "NotRed/Renderer/VertexBuffer.h"
#include "NotRed/Renderer/IndexBuffer.h"

namespace NR
{
    class VertexArray
    {
    public:
        virtual ~VertexArray() {}

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexbuffer) = 0;
        virtual void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& vertexbuffer) = 0;

        virtual const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const = 0;
        virtual const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const = 0;

        static VertexArray* Create();
    };
}