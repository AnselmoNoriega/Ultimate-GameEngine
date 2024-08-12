#include "nrpch.h"
#include "nrpch.h"

#include "Mesh.h"
#include "VertexArray.h"
#include "Texture.h"

namespace NR
{
    Mesh::Mesh(
        Ref<VertexArray>& vertexArray, 
        Ref<VertexBuffer>& vertexBuffer, 
        std::vector<Ref<Texture2D>>& textures, 
        const uint32_t verticesCount
    )
        : mVertexArray(vertexArray), mVertexBuffer(vertexBuffer)
    {
        mTextures = textures;
        mVerticesData = new std::byte[verticesCount];
    }

    void Mesh::AddVertex(const std::byte* vertexInBytes, uint32_t size)
    {
        std::memmove(mVerticesData, vertexInBytes, size);
    }
}