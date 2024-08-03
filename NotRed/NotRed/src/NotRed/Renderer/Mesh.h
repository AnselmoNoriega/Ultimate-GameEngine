#pragma once

namespace NR
{
    class Mesh
    {
    public:
        Mesh(Ref<VertexArray> vertexArray, Ref<VertexBuffer> vertexBuffer, uint32_t verticesCount);

        void AddVertex(const std::byte* vertexInBytes, uint32_t size);

    private:
        Ref<VertexArray> mVertexArray;
        Ref<VertexBuffer> mVertexBuffer;

        std::byte* verticesData;
    };
}