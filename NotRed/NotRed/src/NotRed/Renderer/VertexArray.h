#pragma once

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

        virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexbuffer) = 0;
        virtual void SetIndexBuffer(const Ref<IndexBuffer>& vertexbuffer) = 0;

        virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const = 0;
        virtual const Ref<IndexBuffer>& GetIndexBuffer() const = 0;

        static VertexArray* Create();
    };
}