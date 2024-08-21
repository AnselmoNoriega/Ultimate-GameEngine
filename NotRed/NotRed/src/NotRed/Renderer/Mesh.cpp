#include "nrpch.h" 
#include "Mesh.h"

#include <filesystem>
#include <glad/glad.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLmENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

#include "imgui/imgui.h"

#include "NotRed/Renderer/Renderer.h"

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

    static std::string LevelToSpaces(uint32_t level)
    {
        std::string result = "";
        for (uint32_t i = 0; i < level; ++i)
        {
            result += "--";
        }
        return result;
    }

    glm::mat4 AssimpMat4ToMat4(const aiMatrix4x4& matrix)
    {
        glm::mat4 result;
        //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
        result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
        result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
        result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
        result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
        return result;
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
        mBaseMaterial = CreateRef<Material>(mMeshShader);
        mInverseTransform = glm::inverse(AssimpMat4ToMat4(scene->mRootNode->mTransformation));

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
                for (size_t i = 0; i < mesh->mNumVertices; ++i)
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
                for (size_t i = 0; i < mesh->mNumVertices; ++i)
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

        TraverseNodes(scene->mRootNode);

        // Bones
        if (mIsAnimated)
        {
            for (size_t m = 0; m < scene->mNumMeshes; m++)
            {
                aiMesh* mesh = scene->mMeshes[m];
                Submesh& submesh = mSubmeshes[m];
                for (size_t i = 0; i < mesh->mNumBones; ++i)
                {
                    aiBone* bone = mesh->mBones[i];
                    std::string boneName(bone->mName.data);
                    int boneIndex = 0;
                    if (mBoneMapping.find(boneName) == mBoneMapping.end())
                    {
                        // Allocate an index for a new bone
                        boneIndex = mBoneCount;
                        ++mBoneCount;
                        BoneInfo bi;
                        mBoneInfo.push_back(bi);
                        mBoneInfo[boneIndex].BoneOffset = AssimpMat4ToMat4(bone->mOffsetMatrix);
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

        if (scene->HasMaterials())
        {
            mTextures.resize(scene->mNumMaterials);
            mMaterials.resize(scene->mNumMaterials);
            for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
            {
                auto aiMaterial = scene->mMaterials[i];
                auto aiMaterialName = aiMaterial->GetName();

                auto mi = CreateRef<MaterialInstance>(mBaseMaterial);
                mMaterials[i] = mi;

                NR_CORE_INFO("Material Name = {0}; Index = {1}", aiMaterialName.data, i);
                aiString aiTexPath;
                uint32_t textureCount = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
                NR_CORE_TRACE("  TextureCount = {0}", textureCount);

                aiColor3D aiColor;
                aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor);
                NR_CORE_TRACE("COLOR = {0}, {1}, {2}", aiColor.r, aiColor.g, aiColor.b);

                if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS)
                {
                    // TODO: Temp - this should be handled by Hazel's filesystem
                    std::filesystem::path path = modelsDirectory;
                    auto parentPath = path.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();

                    auto texture = Texture2D::Create(texturePath, true);
                    if (texture->Loaded())
                    {
                        mTextures[i] = texture;
                        NR_CORE_TRACE("  Texture Path = {0}", texturePath);
                        mi->Set("uAlbedoTexture", mTextures[i]);
                        mi->Set("uAlbedoTexToggle", 1.0f);
                    }
                    else
                    {
                        NR_CORE_ERROR("Could not load texture: {0}", texturePath);
                        //mi->Set("uAlbedoTexToggle", 0.0f);
                        mi->Set("uAlbedoColor", glm::vec3{ aiColor.r, aiColor.g, aiColor.b });
                    }
                }
                else
                {
                    mi->Set("uAlbedoTexToggle", 0.0f);
                    mi->Set("uAlbedoColor", glm::vec3{ aiColor.r, aiColor.g, aiColor.b });
                }

                for (uint32_t i = 0; i < aiMaterial->mNumProperties; ++i)
                {
                    auto prop = aiMaterial->mProperties[i];
                    NR_CORE_TRACE("Material Property:");
                    NR_CORE_TRACE("  Name = {0}", prop->mKey.data);

                    switch (prop->mSemantic)
                    {
                    case aiTextureType_NONE:
                        NR_CORE_TRACE("  Semantic = aiTextureType_NONE");
                        break;
                    case aiTextureType_DIFFUSE:
                        NR_CORE_TRACE("  Semantic = aiTextureType_DIFFUSE");
                        break;
                    case aiTextureType_SPECULAR:
                        NR_CORE_TRACE("  Semantic = aiTextureType_SPECULAR");
                        break;
                    case aiTextureType_AMBIENT:
                        NR_CORE_TRACE("  Semantic = aiTextureType_AMBIENT");
                        break;
                    case aiTextureType_EMISSIVE:
                        NR_CORE_TRACE("  Semantic = aiTextureType_EMISSIVE");
                        break;
                    case aiTextureType_HEIGHT:
                        NR_CORE_TRACE("  Semantic = aiTextureType_HEIGHT");
                        break;
                    case aiTextureType_NORMALS:
                        NR_CORE_TRACE("  Semantic = aiTextureType_NORMALS");
                        break;
                    case aiTextureType_SHININESS:
                        NR_CORE_TRACE("  Semantic = aiTextureType_SHININESS");
                        break;
                    case aiTextureType_OPACITY:
                        NR_CORE_TRACE("  Semantic = aiTextureType_OPACITY");
                        break;
                    case aiTextureType_DISPLACEMENT:
                        NR_CORE_TRACE("  Semantic = aiTextureType_DISPLACEMENT");
                        break;
                    case aiTextureType_LIGHTMAP:
                        NR_CORE_TRACE("  Semantic = aiTextureType_LIGHTMAP");
                        break;
                    case aiTextureType_REFLECTION:
                        NR_CORE_TRACE("  Semantic = aiTextureType_REFLECTION");
                        break;
                    case aiTextureType_UNKNOWN:
                        NR_CORE_TRACE("  Semantic = aiTextureType_UNKNOWN");
                        break;
                    }

                    if (prop->mType == aiPTI_String)
                    {
                        uint32_t strLength = *(uint32_t*)prop->mData;
                        std::string str(prop->mData + 4, strLength);
                        NR_CORE_TRACE("  Value = {0}", str);

                        std::string key = prop->mKey.data;
                        if (key == "$raw.ReflectionFactor|file")
                        {
                            std::filesystem::path path = modelsDirectory;
                            auto parentPath = path.parent_path();
                            parentPath /= str;
                            std::string texturePath = parentPath.string();

                            auto texture = Texture2D::Create(texturePath);
                            if (texture->Loaded())
                            {
                                NR_CORE_TRACE("  Metalness map path = {0}", texturePath);
                                mi->Set("uMetalnessTexture", texture);
                                mi->Set("uMetalnessTexToggle", 1.0f);
                            }
                            else
                            {
                                NR_CORE_ERROR("Could not load texture: {0}", texturePath);
                                mi->Set("uMetalness", 0.5f);
                                mi->Set("uMetalnessTexToggle", 1.0f);
                            }
                        }
                    }
                }


                // Normal maps
                if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS)
                {
                    std::filesystem::path path = filename;
                    auto parentPath = path.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();

                    auto texture = Texture2D::Create(texturePath);
                    if (texture->Loaded())
                    {
                        NR_CORE_TRACE("  Normal map path = {0}", texturePath);
                        mi->Set("uNormalTexture", texture);
                        mi->Set("uNormalTexToggle", 1.0f);
                    }
                    else
                    {
                        NR_CORE_ERROR("Could not load texture: {0}", texturePath);
                    }
                }

                // Roughness map
                if (aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) == AI_SUCCESS)
                {
                    std::filesystem::path path = filename;
                    auto parentPath = path.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();

                    auto texture = Texture2D::Create(texturePath);
                    if (texture->Loaded())
                    {
                        NR_CORE_TRACE("  Roughness map path = {0}", texturePath);
                        mi->Set("uRoughnessTexture", texture);
                        mi->Set("uRoughnessTexToggle", 1.0f);
                    }
                    else
                    {
                        NR_CORE_ERROR("Could not load texture: {0}", texturePath);
                        mi->Set("uRoughnessTexToggle", 1.0f);
                        mi->Set("uRoughness", 0.5f);
                    }
                }

                // Metalness map
                if (aiMaterial->Get("$raw.ReflectionFactor|file", aiPTI_String, 0, aiTexPath) == AI_SUCCESS)
                {
                    std::filesystem::path path = filename;
                    auto parentPath = path.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();

                    auto texture = Texture2D::Create(texturePath);
                    if (texture->Loaded())
                    {
                        NR_CORE_TRACE("  Metalness map path = {0}", texturePath);
                        mi->Set("uMetalnessTexture", texture);
                        mi->Set("uMetalnessTexToggle", 1.0f);
                    }
                    else
                    {
                        NR_CORE_ERROR("Could not load texture: {0}", texturePath);
                        mi->Set("uMetalness", 0.5f);
                        mi->Set("uMetalnessTexToggle", 1.0f);
                    }
                }
            }
        }

        mVertexArray = VertexArray::Create();
        if (mIsAnimated)
        {
            auto vb = VertexBuffer::Create(mAnimatedVertices.data(), mAnimatedVertices.size() * sizeof(AnimatedVertex));
            vb->SetLayout({
                { ShaderDataType::Float3, "aPosition" },
                { ShaderDataType::Float3, "aNormal" },
                { ShaderDataType::Float3, "aTangent" },
                { ShaderDataType::Float3, "aBinormal" },
                { ShaderDataType::Float2, "aTexCoord" },
                { ShaderDataType::Int4,   "aBoneIDs" },
                { ShaderDataType::Float4, "aBoneWeights" },
                });
            mVertexArray->AddVertexBuffer(vb);
        }
        else
        {
            auto vb = VertexBuffer::Create(mStaticVertices.data(), mStaticVertices.size() * sizeof(Vertex));
            vb->SetLayout({
                { ShaderDataType::Float3, "aPosition" },
                { ShaderDataType::Float3, "aNormal" },
                { ShaderDataType::Float3, "aTangent" },
                { ShaderDataType::Float3, "aBinormal" },
                { ShaderDataType::Float2, "aTexCoord" },
                });
            mVertexArray->AddVertexBuffer(vb);
        }

        auto ib = IndexBuffer::Create(mIndices.data(), mIndices.size() * sizeof(Index));
        mVertexArray->SetIndexBuffer(ib);

        mScene = scene;
    }

    Mesh::~Mesh()
    {
    }

    uint32_t Mesh::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; ++i)
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

        for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; ++i)
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

        for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; ++i)
        {
            if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
            {
                return i;
            }
        }

        return 0;
    }

    void Mesh::TraverseNodes(aiNode* node, const glm::mat4& parentTransform, uint32_t level)
    {
        glm::mat4 transform = parentTransform * AssimpMat4ToMat4(node->mTransformation);
        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            uint32_t mesh = node->mMeshes[i];
            mSubmeshes[mesh].Transform = transform;
        }

        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            TraverseNodes(node->mChildren[i], transform, level + 1);
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

    void Mesh::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& parentTransform)
    {
        std::string name(pNode->mName.data);
        const aiAnimation* animation = mScene->mAnimations[0];
        glm::mat4 nodeTransform(AssimpMat4ToMat4(pNode->mTransformation));
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

        glm::mat4 transform = parentTransform * nodeTransform;

        if (mBoneMapping.find(name) != mBoneMapping.end())
        {
            uint32_t BoneIndex = mBoneMapping[name];
            mBoneInfo[BoneIndex].FinalTransformation = mInverseTransform * transform * mBoneInfo[BoneIndex].BoneOffset;
        }

        for (uint32_t i = 0; i < pNode->mNumChildren; ++i)
        {
            ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], transform);
        }
    }

    const aiNodeAnim* Mesh::FindNodeAnim(const aiAnimation* animation, const std::string& nodeName)
    {
        for (uint32_t i = 0; i < animation->mNumChannels; ++i)
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

    void Mesh::Update(float dt)
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
    }

    void Mesh::DumpVertexBuffer()
    {
        NR_CORE_TRACE("------------------------------------------------------");
        NR_CORE_TRACE("Vertex Buffer Dump");
        NR_CORE_TRACE("Mesh: {0}", mFilePath);
        if (mIsAnimated)
        {
            for (size_t i = 0; i < mAnimatedVertices.size(); ++i)
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
            for (size_t i = 0; i < mStaticVertices.size(); ++i)
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