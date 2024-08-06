#pragma once

namespace NR
{
    class VertexArray;
    class VertexBuffer;
    class Texture2D;

    class Mesh
    {
    public:
        Mesh() = default;
        Mesh(
            Ref<VertexArray> vertexArray,
            Ref<VertexBuffer> vertexBuffer,
            std::vector<Ref<Texture2D>>& textures,
            const uint32_t verticesCount
        );

        void AddVertex(const std::byte* vertexInBytes, uint32_t size);

    private:
        Ref<VertexArray> mVertexArray;
        Ref<VertexBuffer> mVertexBuffer;

        std::vector<Ref<Texture2D>> mTextures;

        std::byte* mVerticesData;
    };
}