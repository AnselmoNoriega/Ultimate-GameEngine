#pragma once

#include "NotRed/Asset/Asset.h"

#include "NotRed/Renderer/IndexBuffer.h"
#include "NotRed/Renderer/VertexBuffer.h"

#include "NotRed/Renderer/MaterialAsset.h"

namespace NR
{
    struct ParticleVertex
    {
        glm::vec3 Position;
        float Index;
    };

    struct ParticleIndex
    {
        uint32_t V1, V2, V3;
    };

    struct UBStarParams
    {
        uint32_t NumStars = 75000;

        float MaxRad = 3500.0f;
        float BulgeRad = 1250.0f;

        float AngleOffset = 6.28f;
        float Eccentricity = 0.85f;

        float BaseHeight = 300.0f;
        float Height = 250.0f;

        float MinTemp = 3000.0f;
        float MaxTemp = 9000.0f;
        float DustBaseTemp = 4000.0f;

        float MinStarOpacity = 0.1f;
        float MaxStarOpacity = 0.5f;

        float MinDustOpacity = 0.01f;
        float MaxDustOpacity = 0.05f;

        float Speed = 10.0f;
    };

    struct UBEnvironmentParams
    {
        float Time = 0.0f;

        uint32_t NumStars = 75000;
        float StarSize = 10.0f;

        float DustSize = 500.0f;
        float H2Size = 150.0f;

        float H2DistCheck = 300.0f;
    };

    class Particles : public Asset
    {
    public:
        Particles(int particleCount);

        void Update(float dt);

        void ChangeSize(int particleCount);

        Ref<Material>& GetMaterial() { return mMaterial; }
        const Ref<Material>& GetMaterial() const { return mMaterial; }

    private:
        static UBStarParams& GetStarParams() { return mStarParamsUB; }
        static UBEnvironmentParams& GetEnvironmentParams() { return mEnvironmentParamsUB; }

    private:
        Ref<VertexBuffer> mVertexBuffer;
        Ref<IndexBuffer> mIndexBuffer;

        std::vector<ParticleVertex> mVertices;
        std::vector<ParticleIndex> mIndices;

        Ref<Material> mMaterial;

        // StarData
        static UBStarParams mStarParamsUB;
        static UBEnvironmentParams mEnvironmentParamsUB;

    private:
        friend class SceneRenderer;
    };
}