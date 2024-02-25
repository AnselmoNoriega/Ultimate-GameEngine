#pragma once

#include "NotRed/Renderer/BufferLayout.h"

namespace NR
{

    class VertexBuffer
    {
    public:
        virtual ~VertexBuffer() {}

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual const  BufferLayout& GetLayout() const = 0;
        virtual void SetLayout(const BufferLayout& layout) = 0;

        static VertexBuffer* Create(float* vertices, uint32_t size);
    };
}