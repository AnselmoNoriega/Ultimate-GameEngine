#include "nrpch.h"
#include "nrpch.h"

#include "Mesh.h"
#include "VertexArray.h"

namespace NR
{
    Mesh::Mesh(Ref<VertexArray> vertexArray, Ref<VertexBuffer> vertexBuffer, const uint32_t verticesCount)
        : mVertexArray(vertexArray), mVertexBuffer(vertexBuffer)
    {
        mVerticesData = new std::byte[verticesCount];
    }

    void Mesh::AddVertex(const std::byte* vertexInBytes, uint32_t size)
    {
        std::memmove(mVerticesData, vertexInBytes, size);
    }
}