#include "nrpch.h"
#include "Particles.h"

#include "NotRed/Renderer/Renderer.h"

namespace NR
{
    UBStarParams Particles::mStarParamsUB;
    UBEnvironmentParams Particles::mEnvironmentParamsUB;

    Particles::Particles(int particleCount)
    {
        {
            for (uint32_t i = 0; i < particleCount; ++i)
            {
                ParticleVertex leftBot;
                leftBot.Position = { -0.5f, 0.0f, -0.5f };
                leftBot.Index = (float)i;
                mVertices.push_back(leftBot);

                ParticleVertex rightBot;
                rightBot.Position = { 0.5f, 0.0f, -0.5f };
                rightBot.Index = (float)i;
                mVertices.push_back(rightBot);

                ParticleVertex leftTop;
                leftTop.Position = { -0.5f, 0.0f,  0.5f };
                leftTop.Index = (float)i;
                mVertices.push_back(leftTop);

                ParticleVertex rightTop;
                rightTop.Position = { 0.5f, 0.0f,  0.5f };
                rightTop.Index = (float)i;
                mVertices.push_back(rightTop);

                unsigned int index = i * 4;
                ParticleIndex indexA = { index + 0, index + 1, index + 2 };
                ParticleIndex indexB = { index + 1, index + 3, index + 2 };
                mIndices.push_back(indexA);
                mIndices.push_back(indexB);
            }
        }

        mMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Particle"), "Particle-Effect");
        mMaterial->Set("uGalaxySpecs.StarColor", glm::vec3(1.0f, 1.0f, 1.0f));
        mMaterial->Set("uGalaxySpecs.DustColor", glm::vec3(0.388f, 0.333f, 1.0f));
        mMaterial->Set("uGalaxySpecs.h2RegionColor", glm::vec3(0.8f, 0.071f, 0.165f));

        mVertexBuffer = VertexBuffer::Create(mVertices.data(), mVertices.size() * sizeof(ParticleVertex));
        mIndexBuffer = IndexBuffer::Create(mIndices.data(), mIndices.size() * sizeof(ParticleIndex));
    }

    void Particles::Update(float dt)
    {
        mEnvironmentParamsUB.Time += dt;
    }

    void Particles::ChangeSize(int particleCount)
    {
        mVertices.clear();
        mIndices.clear();

        {
            for (uint32_t i = 0; i < particleCount; ++i)
            {
                ParticleVertex leftBot;
                leftBot.Position = { -0.5f, 0.0f, -0.5f };
                leftBot.Index = (float)i;
                mVertices.push_back(leftBot);

                ParticleVertex rightBot;
                rightBot.Position = { 0.5f, 0.0f, -0.5f };
                rightBot.Index = (float)i;
                mVertices.push_back(rightBot);

                ParticleVertex leftTop;
                leftTop.Position = { -0.5f, 0.0f,  0.5f };
                leftTop.Index = (float)i;
                mVertices.push_back(leftTop);

                ParticleVertex rightTop;
                rightTop.Position = { 0.5f, 0.0f,  0.5f };
                rightTop.Index = (float)i;
                mVertices.push_back(rightTop);

                unsigned int index = i * 4;
                ParticleIndex indexA = { index + 0, index + 1, index + 2 };
                ParticleIndex indexB = { index + 1, index + 3, index + 2 };
                mIndices.push_back(indexA);
                mIndices.push_back(indexB);
            }
        }

        mVertexBuffer = VertexBuffer::Create(mVertices.data(), mVertices.size() * sizeof(ParticleVertex));
        mIndexBuffer = IndexBuffer::Create(mIndices.data(), mIndices.size() * sizeof(ParticleIndex));
    }
}