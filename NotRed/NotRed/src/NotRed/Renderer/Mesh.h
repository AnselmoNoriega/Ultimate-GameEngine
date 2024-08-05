#pragma once

namespace NR
{
    class VertexArray;
    class VertexBuffer;

    class Mesh
    {
    public:
        Mesh() = default;
        Mesh(Ref<VertexArray> vertexArray, Ref<VertexBuffer> vertexBuffer, const uint32_t verticesCount);

        void AddVertex(const std::byte* vertexInBytes, uint32_t size);

    private:
        Ref<VertexArray> mVertexArray;
        Ref<VertexBuffer> mVertexBuffer;

        std::byte* mVerticesData;
    };
}