#include "nrpch.h" 
#include "Mesh.h"

#include <filesystem>
#include <glad/glad.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLmENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

#include "imgui/imgui.h"

namespace NR
{
    static const uint32_t sMeshImportFlags =
        aiProcess_CalcTangentSpace |        // Create binormals/tangents
        aiProcess_Triangulate |             // Make sure we're triangles
        aiProcess_SortByPType |             // Split meshes by primitive type
        aiProcess_GenNormals |              // Make sure we have normals
        aiProcess_GenUVCoords |             // Convert UVs if required 
        aiProcess_OptimizeMeshes |          // Batch draws where possible
        aiProcess_ValidateDataStructure;    // Validation

    struct LogStream : public Assimp::LogStream
    {
        static void Initialize()
        {
            if (Assimp::DefaultLogger::isNullLogger())
            {
                Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
                Assimp::DefaultLogger::get()->attachStream(new LogStream, Assimp::Logger::Err | Assimp::Logger::Warn);
            }
        }

        virtual void write(const char* message) override
        {
            NR_CORE_ERROR("Assimp error: {0}", message);
        }
    };

    static glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    Mesh::Mesh(const std::string& filename)
        : mFilePath(filename)
    {
        LogStream::Initialize();

        NR_CORE_INFO("Loading mesh: {0}", filename.c_str());

        mImporter = std::make_unique<Assimp::Importer>();

        std::filesystem::path modelsDirectory = std::filesystem::current_path().parent_path() /
            "NotEditor" / filename;

        const aiScene* scene = mImporter->ReadFile(modelsDirectory.string(), sMeshImportFlags);
        if (!scene || !scene->HasMeshes())
        {
            NR_CORE_ERROR("Failed to load mesh file: {0}", modelsDirectory.string());
        }

        mIsAnimated = scene->mAnimations != nullptr;
        mMeshShader = mIsAnimated ? Renderer::GetShaderLibrary()->Get("PBR_Anim") : Renderer::GetShaderLibrary()->Get("PBR_Static");
        mMaterial.reset(new NR::Material(mMeshShader));

        mInverseTransform = glm::inverse(aiMatrix4x4ToGlm(scene->mRootNode->mTransformation));

        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;

        mSubmeshes.reserve(scene->mNumMeshes);
        for (size_t m = 0; m < scene->mNumMeshes; ++m)
        {
            aiMesh* mesh = scene->mMeshes[m];

            Submesh submesh;
            submesh.BaseVertex = vertexCount;
            submesh.BaseIndex = indexCount;
            submesh.MaterialIndex = mesh->mMaterialIndex;
            submesh.IndexCount = mesh->mNumFaces * 3;
            mSubmeshes.push_back(submesh);

            vertexCount += mesh->mNumVertices;
            indexCount += submesh.IndexCount;

            NR_CORE_ASSERT(mesh->HasPositions(), "Meshes require positions.");
            NR_CORE_ASSERT(mesh->HasNormals(), "Meshes require normals.");

            if (mIsAnimated)
            {
                for (size_t i = 0; i < mesh->mNumVertices; i++)
                {
                    AnimatedVertex vertex;
                    vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
                    vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

                    if (mesh->HasTangentsAndBitangents())
                    {
                        vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
                        vertex.Binormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
                    }

                    if (mesh->HasTextureCoords(0))
                    {
                        vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
                    }

                    mAnimatedVertices.push_back(vertex);
                }
            }
            else
            {
                for (size_t i = 0; i < mesh->mNumVertices; i++)
                {
                    Vertex vertex;
                    vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
                    vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

                    if (mesh->HasTangentsAndBitangents())
                    {
                        vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
                        vertex.Binormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
                    }

                    if (mesh->HasTextureCoords(0))
                    {
                        vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
                    }

                    mStaticVertices.push_back(vertex);
                }
            }

            for (size_t i = 0; i < mesh->mNumFaces; ++i)
            {
                NR_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Must have 3 indices.");
                mIndices.push_back({ mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] });
            }
        }

        NR_CORE_TRACE("NODES:");
        NR_CORE_TRACE("-----------------------------");
        TraverseNodes(scene->mRootNode);
        NR_CORE_TRACE("-----------------------------");

        // Bones
        if (mIsAnimated)
        {
            for (size_t m = 0; m < scene->mNumMeshes; m++)
            {
                aiMesh* mesh = scene->mMeshes[m];
                Submesh& submesh = mSubmeshes[m];

                for (size_t i = 0; i < mesh->mNumBones; i++)
                {
                    aiBone* bone = mesh->mBones[i];
                    std::string boneName(bone->mName.data);
                    int boneIndex = 0;

                    if (mBoneMapping.find(boneName) == mBoneMapping.end())
                    {
                        // Allocate an index for a new bone
                        boneIndex = mBoneCount;
                        mBoneCount++;
                        BoneInfo bi;
                        mBoneInfo.push_back(bi);
                        mBoneInfo[boneIndex].BoneOffset = aiMatrix4x4ToGlm(bone->mOffsetMatrix);
                        mBoneMapping[boneName] = boneIndex;
                    }
                    else
                    {
                        NR_CORE_TRACE("Found existing bone in map");
                        boneIndex = mBoneMapping[boneName];
                    }

                    for (size_t j = 0; j < bone->mNumWeights; j++)
                    {
                        int VertexID = submesh.BaseVertex + bone->mWeights[j].mVertexId;
                        float Weight = bone->mWeights[j].mWeight;
                        mAnimatedVertices[VertexID].AddBoneData(boneIndex, Weight);
                    }
                }
            }
        }

        mVertexBuffer.reset(VertexBuffer::Create());
        if (mIsAnimated)
        {
            mVertexBuffer->SetData(mAnimatedVertices.data(), mAnimatedVertices.size() * sizeof(AnimatedVertex));
        }
        else
        {
            mVertexBuffer->SetData(mStaticVertices.data(), mStaticVertices.size() * sizeof(Vertex));
        }

        mIndexBuffer.reset(IndexBuffer::Create());
        mIndexBuffer->SetData(mIndices.data(), mIndices.size() * sizeof(Index));

        mScene = scene;
    }

    Mesh::~Mesh()
    {
    }

    uint32_t Mesh::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
        {
            if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
            {
                return i;
            }
        }

        return 0;
    }


    uint32_t Mesh::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        NR_CORE_ASSERT(pNodeAnim->mNumRotationKeys > 0);

        for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
        {
            if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
            {
                return i;
            }
        }

        return 0;
    }


    uint32_t Mesh::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        NR_CORE_ASSERT(pNodeAnim->mNumScalingKeys > 0);

        for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
        {
            if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
            {
                return i;
            }
        }

        return 0;
    }

    void Mesh::TraverseNodes(aiNode* node, int level)
    {
        std::string levelText;
        for (int i = 0; i < level; ++i)
        {
            levelText += "-";
        }
        NR_CORE_TRACE("{0}Node name: {1}", levelText, std::string(node->mName.data));
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            uint32_t mesh = node->mMeshes[i];
            mSubmeshes[mesh].Transform = aiMatrix4x4ToGlm(node->mTransformation);
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            aiNode* child = node->mChildren[i];
            TraverseNodes(child, level + 1);
        }
    }

    glm::vec3 Mesh::InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim)
    {
        if (nodeAnim->mNumPositionKeys == 1)
        {
            auto v = nodeAnim->mPositionKeys[0].mValue;
            return { v.x, v.y, v.z };
        }

        uint32_t positionIndex = FindPosition(animationTime, nodeAnim);
        uint32_t nextPositionIndex = (positionIndex + 1);
        NR_CORE_ASSERT(nextPositionIndex < nodeAnim->mNumPositionKeys);

        float deltaTime = (float)(nodeAnim->mPositionKeys[nextPositionIndex].mTime - nodeAnim->mPositionKeys[positionIndex].mTime);
        float factor = (animationTime - (float)nodeAnim->mPositionKeys[positionIndex].mTime) / deltaTime;
        if (factor < 0.0f)
        {
            factor = 0.0f;
        }
        NR_CORE_ASSERT(factor <= 1.0f, "Factor must be below 1.0f");

        const aiVector3D& start = nodeAnim->mPositionKeys[positionIndex].mValue;
        const aiVector3D& end = nodeAnim->mPositionKeys[nextPositionIndex].mValue;
        aiVector3D delta = end - start;
        auto aiVec = start + factor * delta;
        return { aiVec.x, aiVec.y, aiVec.z };
    }


    glm::quat Mesh::InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim)
    {
        if (nodeAnim->mNumRotationKeys == 1)
        {
            auto v = nodeAnim->mRotationKeys[0].mValue;
            return glm::quat(v.w, v.x, v.y, v.z);
        }

        uint32_t rotationIndex = FindRotation(animationTime, nodeAnim);
        uint32_t nextRotationIndex = (rotationIndex + 1);
        NR_CORE_ASSERT(nextRotationIndex < nodeAnim->mNumRotationKeys);

        float DeltaTime = (float)(nodeAnim->mRotationKeys[nextRotationIndex].mTime - nodeAnim->mRotationKeys[rotationIndex].mTime);
        float factor = (animationTime - (float)nodeAnim->mRotationKeys[rotationIndex].mTime) / DeltaTime;
        if (factor < 0.0f)
        {
            factor = 0.0f;
        }
        NR_CORE_ASSERT(factor <= 1.0f, "Factor must be below 1.0f");

        const aiQuaternion& startRotationQ = nodeAnim->mRotationKeys[rotationIndex].mValue;
        const aiQuaternion& endRotationQ = nodeAnim->mRotationKeys[nextRotationIndex].mValue;
        auto q = aiQuaternion();
        aiQuaternion::Interpolate(q, startRotationQ, endRotationQ, factor);
        q = q.Normalize();
        return glm::quat(q.w, q.x, q.y, q.z);
    }


    glm::vec3 Mesh::InterpolateScale(float animationTime, const aiNodeAnim* nodeAnim)
    {
        if (nodeAnim->mNumScalingKeys == 1)
        {
            auto v = nodeAnim->mScalingKeys[0].mValue;
            return { v.x, v.y, v.z };
        }

        uint32_t index = FindScaling(animationTime, nodeAnim);
        uint32_t nextIndex = (index + 1);
        NR_CORE_ASSERT(nextIndex < nodeAnim->mNumScalingKeys);

        float deltaTime = (float)(nodeAnim->mScalingKeys[nextIndex].mTime - nodeAnim->mScalingKeys[index].mTime);
        float factor = (animationTime - (float)nodeAnim->mScalingKeys[index].mTime) / deltaTime;
        if (factor < 0.0f)
        {
            factor = 0.0f;
        }
        NR_CORE_ASSERT(factor <= 1.0f, "Factor must be below 1.0f");

        const auto& start = nodeAnim->mScalingKeys[index].mValue;
        const auto& end = nodeAnim->mScalingKeys[nextIndex].mValue;
        auto delta = end - start;
        auto aiVec = start + factor * delta;
        return { aiVec.x, aiVec.y, aiVec.z };
    }

    void Mesh::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform)
    {
        std::string name(pNode->mName.data);
        const aiAnimation* animation = mScene->mAnimations[0];
        glm::mat4 nodeTransform(aiMatrix4x4ToGlm(pNode->mTransformation));
        const aiNodeAnim* nodeAnim = FindNodeAnim(animation, name);

        if (nodeAnim)
        {
            glm::vec3 translation = InterpolateTranslation(AnimationTime, nodeAnim);
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, translation.z));

            glm::quat rotation = InterpolateRotation(AnimationTime, nodeAnim);
            glm::mat4 rotationMatrix = glm::toMat4(rotation);

            glm::vec3 scale = InterpolateScale(AnimationTime, nodeAnim);
            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, scale.z));

            nodeTransform = translationMatrix * rotationMatrix * scaleMatrix;
        }

        glm::mat4 transform = ParentTransform * nodeTransform;

        if (mBoneMapping.find(name) != mBoneMapping.end())
        {
            uint32_t BoneIndex = mBoneMapping[name];
            mBoneInfo[BoneIndex].FinalTransformation = mInverseTransform * transform * mBoneInfo[BoneIndex].BoneOffset;
        }

        for (uint32_t i = 0; i < pNode->mNumChildren; i++)
        {
            ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], transform);
        }
    }

    const aiNodeAnim* Mesh::FindNodeAnim(const aiAnimation* animation, const std::string& nodeName)
    {
        for (uint32_t i = 0; i < animation->mNumChannels; i++)
        {
            const aiNodeAnim* nodeAnim = animation->mChannels[i];
            if (std::string(nodeAnim->mNodeName.data) == nodeName)
                return nodeAnim;
        }
        return nullptr;
    }

    void Mesh::BoneTransform(float time)
    {
        ReadNodeHierarchy(time, mScene->mRootNode, glm::mat4(1.0f));
        mBoneTransforms.resize(mBoneCount);
        for (size_t i = 0; i < mBoneCount; ++i)
        {
            mBoneTransforms[i] = mBoneInfo[i].FinalTransformation;
        }
    }

    void Mesh::Render(float dt, Ref<MaterialInstance> materialInstance)
    {
        Render(dt, glm::mat4(1.0f), materialInstance);
    }

    void Mesh::Render(float dt, const glm::mat4& transform, Ref<MaterialInstance> materialInstance)
    {
        if (mIsAnimated)
        {
            if (mAnimationPlaying)
            {
                mWorldTime += dt;

                float ticksPerSecond = (float)(mScene->mAnimations[0]->mTicksPerSecond != 0 ? mScene->mAnimations[0]->mTicksPerSecond : 25.0f) * mTimeMultiplier;
                mAnimationTime += dt * ticksPerSecond;
                mAnimationTime = fmod(mAnimationTime, (float)mScene->mAnimations[0]->mDuration);
            }

            BoneTransform(mAnimationTime);
        }

        if (materialInstance)
        {
            materialInstance->Bind();
        }

        // TODO: Sort this out
        mVertexBuffer->Bind();
        mIndexBuffer->Bind();

        bool materialOverride = !!materialInstance;

        // TODO: replace with render API calls
        NR_RENDER_S2(transform, materialOverride, {
            for (Submesh& submesh : self->mSubmeshes)
            {
                if (self->mIsAnimated)
                {
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (const void*)offsetof(AnimatedVertex, Position));

                    glEnableVertexAttribArray(1);
                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (const void*)offsetof(AnimatedVertex, Normal));

                    glEnableVertexAttribArray(2);
                    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (const void*)offsetof(AnimatedVertex, Tangent));

                    glEnableVertexAttribArray(3);
                    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (const void*)offsetof(AnimatedVertex, Binormal));

                    glEnableVertexAttribArray(4);
                    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (const void*)offsetof(AnimatedVertex, Texcoord));

                    glEnableVertexAttribArray(5);
                    glVertexAttribIPointer(5, 4, GL_INT, sizeof(AnimatedVertex), (const void*)offsetof(AnimatedVertex, IDs));

                    glEnableVertexAttribArray(6);
                    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (const void*)offsetof(AnimatedVertex, Weights));

                    for (size_t i = 0; i < self->mBoneTransforms.size(); i++)
                    {
                        std::string uniformName = std::string("uBoneTransforms[") + std::to_string(i) + std::string("]");
                        self->mMeshShader->SetMat4FromRenderThread(uniformName, self->mBoneTransforms[i]);
                    }
                }
                else
                {
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Position));

                    glEnableVertexAttribArray(1);
                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Normal));

                    glEnableVertexAttribArray(2);
                    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Tangent));

                    glEnableVertexAttribArray(3);
                    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Binormal));

                    glEnableVertexAttribArray(4);
                    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Texcoord));
                }

                if (!materialOverride)
                {
                    self->mMeshShader->SetMat4FromRenderThread("uModelMatrix", transform * submesh.Transform);
                }
                glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
            }
            });
    }
    
    void Mesh::ImGuiRender()
    {
        ImGui::Begin("Mesh Debug");
        if (ImGui::CollapsingHeader(mFilePath.c_str()))
        {
            if (mIsAnimated)
            {
                if (ImGui::CollapsingHeader("Animation"))
                {
                    if (ImGui::Button(mAnimationPlaying ? "Pause" : "Play"))
                        mAnimationPlaying = !mAnimationPlaying;

                    ImGui::SliderFloat("##AnimationTime", &mAnimationTime, 0.0f, (float)mScene->mAnimations[0]->mDuration);
                    ImGui::DragFloat("Time Scale", &mTimeMultiplier, 0.05f, 0.0f, 10.0f);
                }
            }
        }

        ImGui::End();
    }

    void Mesh::DumpVertexBuffer()
    {
        // TODO: Convert to ImGui
        NR_CORE_TRACE("------------------------------------------------------");
        NR_CORE_TRACE("Vertex Buffer Dump");
        NR_CORE_TRACE("Mesh: {0}", mFilePath);
        if (mIsAnimated)
        {
            for (size_t i = 0; i < mAnimatedVertices.size(); i++)
            {
                auto& vertex = mAnimatedVertices[i];
                NR_CORE_TRACE("Vertex: {0}", i);
                NR_CORE_TRACE("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
                NR_CORE_TRACE("Normal: {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
                NR_CORE_TRACE("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
                NR_CORE_TRACE("Tangent: {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
                NR_CORE_TRACE("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
                NR_CORE_TRACE("--");
            }
        }
        else
        {
            for (size_t i = 0; i < mStaticVertices.size(); i++)
            {
                auto& vertex = mStaticVertices[i];
                NR_CORE_TRACE("Vertex: {0}", i);
                NR_CORE_TRACE("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
                NR_CORE_TRACE("Normal: {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
                NR_CORE_TRACE("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
                NR_CORE_TRACE("Tangent: {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
                NR_CORE_TRACE("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
                NR_CORE_TRACE("--");
            }
        }
        NR_CORE_TRACE("------------------------------------------------------");
    }

}