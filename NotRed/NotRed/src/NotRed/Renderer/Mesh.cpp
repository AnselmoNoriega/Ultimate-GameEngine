#include "nrpch.h" 
#include "Mesh.h"

#include <filesystem>
#include <glad/glad.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>

#include "NotRed/Asset/AssimpLog.h"
#include "NotRed/Asset/OZZImporterAssimp.h"

#include "imgui/imgui.h"

#include "NotRed/Debug/Profiler.h"

#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/VertexBuffer.h"

#include "NotEditor/src/Panels/ContentBrowserPanel.h"

namespace NR
{

#define MESH_DEBUG_LOG 1
#if MESH_DEBUG_LOG
#define NR_MESH_LOG(...) NR_CORE_TRACE(__VA_ARGS__)
#else
#define NR_MESH_LOG(...)
#endif

    static const uint32_t sMeshImportFlags =
        aiProcess_CalcTangentSpace |        // Create binormals/tangents
        aiProcess_Triangulate |             // Make sure we're triangles
        aiProcess_SortByPType |             // Split meshes by primitive type
        aiProcess_GenNormals |              // Make sure we have normals
        aiProcess_GenUVCoords |             // Convert UVs if required 
        aiProcess_OptimizeMeshes |          // Batch draws where possible
        aiProcess_JoinIdenticalVertices |   // Remove vertices that are identical
        aiProcess_LimitBoneWeights |        // If more than 4 bone weights, discard least influencing ones and renormalize sum to 1
        aiProcess_ValidateDataStructure;    // Validation

    static std::string LevelToSpaces(uint32_t level)
    {
        std::string result = "";
        for (uint32_t i = 0; i < level; ++i)
        {
            result += "--";
        }
        return result;
    }

    glm::mat4 Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix)
    {
        glm::mat4 result;
        //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
        result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
        result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
        result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
        result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
        return result;
    }

    MeshAsset::MeshAsset(const std::string& filename)
        : mFilePath(filename)
    {
        LogStream::Initialize();

        NR_MESH_LOG("Loading mesh: {0}", filename.c_str());

        mImporter = std::make_unique<Assimp::Importer>();

        std::filesystem::path modelsDirectory = std::filesystem::current_path().parent_path() /
            "NotEditor" / filename;

        const aiScene* scene = mImporter->ReadFile(modelsDirectory.string(), sMeshImportFlags);
        if (!scene || !scene->HasMeshes())
        {
            NR_CORE_ERROR("Failed to load mesh file: {0}", modelsDirectory.string());
            ModifyFlags(AssetFlag::Invalid);
            return;
        }

        mScene = scene;

        ozz::animation::offline::RawSkeleton rawSkeleton;
        ozz::math::Float4x4 transform;
        if (OZZImporterAssimp::ExtractRawSkeleton(scene, rawSkeleton, transform))
        {
            ozz::animation::offline::SkeletonBuilder builder;
            mSkeleton = builder(rawSkeleton);
            if (!mSkeleton)
            {
                NR_CORE_ERROR("Failed to build runtime skeleton from mesh file '{0}'", filename);
            }
        }
        else
        {
            NR_CORE_INFO("No skeleton found in mesh file '{0}'", filename);
        }

        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;

        mBoundingBox.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
        mBoundingBox.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        mSubmeshes.reserve(scene->mNumMeshes);
        for (unsigned m = 0; m < scene->mNumMeshes; ++m)
        {
            aiMesh* mesh = scene->mMeshes[m];

            Submesh& submesh = mSubmeshes.emplace_back();
            submesh.BaseVertex = vertexCount;
            submesh.BaseIndex = indexCount;
            submesh.VertexCount = mesh->mNumVertices;
            submesh.IndexCount = mesh->mNumFaces * 3;
            submesh.MaterialIndex = mesh->mMaterialIndex;
            submesh.MeshName = mesh->mName.C_Str();

            vertexCount += mesh->mNumVertices;
            indexCount += submesh.IndexCount;

            NR_CORE_ASSERT(mesh->HasPositions(), "Meshes require positions.");
            NR_CORE_ASSERT(mesh->HasNormals(), "Meshes require normals.");

            auto& aabb = submesh.BoundingBox;
            aabb.Min = { FLT_MAX, FLT_MAX, FLT_MAX };
            aabb.Max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
            for (size_t i = 0; i < mesh->mNumVertices; ++i)
            {
                Vertex vertex;
                vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
                vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
                aabb.Min.x = glm::min(vertex.Position.x, aabb.Min.x);
                aabb.Min.y = glm::min(vertex.Position.y, aabb.Min.y);
                aabb.Min.z = glm::min(vertex.Position.z, aabb.Min.z);
                aabb.Max.x = glm::max(vertex.Position.x, aabb.Max.x);
                aabb.Max.y = glm::max(vertex.Position.y, aabb.Max.y);
                aabb.Max.z = glm::max(vertex.Position.z, aabb.Max.z);

                if (mesh->HasTangentsAndBitangents())
                {
                    vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
                    vertex.Binormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
                }

                if (mesh->HasTextureCoords(0))
                {
                    vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
                }

                if (IsRigged())
                {
                    mStaticVertices.push_back(vertex);  
                    mSkinnedVertices.push_back(vertex); 
                }
                else
                {
                    mStaticVertices.push_back(vertex);
                }
            }

            for (size_t i = 0; i < mesh->mNumFaces; ++i)
            {
                NR_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Must have 3 indices.");
                Index index = { mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] };
                mIndices.push_back(index);

                mTriangleCache[m].emplace_back(mStaticVertices[index.V1 + submesh.BaseVertex], mStaticVertices[index.V2 + submesh.BaseVertex], mStaticVertices[index.V3 + submesh.BaseVertex]);
            }
        }

        TraverseNodes(scene->mRootNode);

        for (const auto& submesh : mSubmeshes)
        {
            AABB transformedSubmeshAABB = submesh.BoundingBox;
            glm::vec3 min = glm::vec3(submesh.Transform * glm::vec4(transformedSubmeshAABB.Min, 1.0f));
            glm::vec3 max = glm::vec3(submesh.Transform * glm::vec4(transformedSubmeshAABB.Max, 1.0f));
            mBoundingBox.Min.x = glm::min(mBoundingBox.Min.x, min.x);
            mBoundingBox.Min.y = glm::min(mBoundingBox.Min.y, min.y);
            mBoundingBox.Min.z = glm::min(mBoundingBox.Min.z, min.z);
            mBoundingBox.Max.x = glm::max(mBoundingBox.Max.x, max.x);
            mBoundingBox.Max.y = glm::max(mBoundingBox.Max.y, max.y);
            mBoundingBox.Max.z = glm::max(mBoundingBox.Max.z, max.z);
        }

        // Bones
        if (IsRigged())
        {
            for (size_t m = 0; m < scene->mNumMeshes; ++m)
            {
                aiMesh* mesh = scene->mMeshes[m];
                Submesh& submesh = mSubmeshes[m];

                for (size_t i = 0; i < mesh->mNumBones; ++i)
                {
                    aiBone* bone = mesh->mBones[i];
                    std::string boneName(bone->mName.data);

                    // Find bone in skeleton
                    uint32_t jointIndex = ~0;
                    for (size_t j = 0; j < mSkeleton->joint_names().size(); ++j)
                    {
                        if (boneName == mSkeleton->joint_names()[j])
                        {
                            jointIndex = static_cast<int>(j);
                            break;
                        }
                    }

                    if (jointIndex == ~0)
                    {
                        NR_CORE_ERROR("Could not find mesh bone '{}' in skeleton!", boneName);
                    }

                    uint32_t boneIndex = ~0;
                    for (size_t j = 0; j < mBoneInfo.size(); ++j)
                    {
                        if (mBoneInfo[j].JointIndex == jointIndex)
                        {
                            boneIndex = static_cast<uint32_t>(j);
                            break;
                        }
                    }

                    if (boneIndex == ~0)
                    {
                        // Allocate an index for a new bone
                        boneIndex = static_cast<uint32_t>(mBoneInfo.size());
                        mBoneInfo.emplace_back(Float4x4FromAIMatrix4x4(bone->mOffsetMatrix), jointIndex);
                    }

                    for (size_t j = 0; j < bone->mNumWeights; ++j)
                    {
                        int VertexID = submesh.BaseVertex + bone->mWeights[j].mVertexId;
                        float Weight = bone->mWeights[j].mWeight;
                        mSkinnedVertices[VertexID].AddBoneData(boneIndex, Weight);
                    }
                }
            }
        }

        Ref<Texture2D> whiteTexture = Renderer::GetWhiteTexture();
        if (scene->HasMaterials())
        {
            mMaterials.resize(scene->mNumMaterials);

            for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
            {
                auto aiMaterial = scene->mMaterials[i];
                auto aiMaterialName = aiMaterial->GetName();

                Ref<Material> mi = Material::Create(Renderer::GetShaderLibrary()->Get("PBR_Static"), aiMaterialName.data);
                mMaterials[i] = mi;

                NR_MESH_LOG("Material Name = {0}; Index = {1}", aiMaterialName.data, i);
                aiString aiTexPath;
                uint32_t textureCount = aiMaterial->GetTextureCount(aiTextureType_DIFFUSE);
                NR_MESH_LOG("  TextureCount = {0}", textureCount);

                glm::vec3 albedoColor(0.8f);
                float emission = 0.0f;
                aiColor3D aiColor, aiEmission;
                if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS)
                {
                    albedoColor = { aiColor.r, aiColor.g, aiColor.b };
                }

                if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmission) == AI_SUCCESS)
                {
                    emission = aiEmission.r;
                }

                mi->Set("uMaterialUniforms.AlbedoColor", albedoColor);
                mi->Set("uMaterialUniforms.Emission", emission);

                float shininess, metalness;
                if (aiMaterial->Get(AI_MATKEY_SHININESS, shininess) != aiReturn_SUCCESS)
                {
                    shininess = 80.0f;
                }

                if (aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metalness) != aiReturn_SUCCESS)
                {
                    metalness = 0.0f;
                }

                float roughness = 1.0f - glm::sqrt(shininess / 100.0f);
                NR_MESH_LOG("    COLOR = {0}, {1}, {2}", aiColor.r, aiColor.g, aiColor.b);
                NR_MESH_LOG("    ROUGHNESS = {0}", roughness);
                NR_MESH_LOG("    METALNESS = {0}", metalness);

                bool hasAlbedoMap = aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) == AI_SUCCESS;
                bool fallback = !hasAlbedoMap;
                if (hasAlbedoMap)
                {
                    std::filesystem::path path = modelsDirectory;
                    auto parentPath = path.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();

                    NR_MESH_LOG("    Albedo map path = {0}", texturePath);
                    TextureProperties props;
                    props.StandardRGB = true;
                    auto texture = Texture2D::Create(texturePath, props);
                    if (texture->Loaded())
                    {
                        mi->Set("uAlbedoTexture", texture);
                        mi->Set("uMaterialUniforms.AlbedoColor", glm::vec3(1.0f));
                    }
                    else
                    {
                        NR_CORE_ERROR("Could not load texture: {0}", texturePath);
                        fallback = true;
                    }
                }

                if (fallback)
                {
                    NR_MESH_LOG("    No albedo map");
                    mi->Set("uAlbedoTexture", whiteTexture);
                }

                // Normal maps
                bool hasNormalMap = aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) == AI_SUCCESS;
                fallback = !hasNormalMap;
                if (hasNormalMap)
                {
                    std::filesystem::path path = filename;
                    auto parentPath = path.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();

                    NR_MESH_LOG("    Normal map path = {0}", texturePath);
                    auto texture = Texture2D::Create(texturePath);
                    if (texture->Loaded())
                    {
                        mi->Set("uNormalTexture", texture);
                        mi->Set("uMaterialUniforms.UseNormalMap", true);
                    }
                    else
                    {
                        NR_CORE_ERROR("  Could not load normal map");
                        fallback = true;
                    }
                }

                if (fallback)
                {
                    NR_MESH_LOG("   Mesh has no normal map");
                    mi->Set("uNormalTexture", whiteTexture);
                    mi->Set("uMaterialUniforms.UseNormalMap", false);
                }

                // Roughness map
                bool hasRoughnessMap = aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) == AI_SUCCESS;
                fallback = !hasRoughnessMap;
                if (hasRoughnessMap)
                {
                    std::filesystem::path path = filename;
                    auto parentPath = path.parent_path();
                    parentPath /= std::string(aiTexPath.data);
                    std::string texturePath = parentPath.string();

                    NR_MESH_LOG("    Roughness map path = {0}", texturePath);
                    auto texture = Texture2D::Create(texturePath);
                    if (texture->Loaded())
                    {
                        mi->Set("uRoughnessTexture", texture);
                        mi->Set("uMaterialUniforms.Roughness", 1.0f);
                    }
                    else
                    {
                        NR_CORE_ERROR("    Could not load roughness map}");
                        fallback = true;
                    }
                }

                if (fallback)
                {
                    NR_MESH_LOG("    No roughness map");
                    mi->Set("uRoughnessTexture", whiteTexture);
                    mi->Set("uMaterialUniforms.Roughness", roughness);
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
                        NR_MESH_LOG("  Metalness map path = {0}", texturePath);
                        mi->Set("uMetalnessTexture", texture);
                        mi->Set("uMetalnessTexToggle", 1.0f);
                    }
                    else
                    {
                        NR_CORE_ERROR("Could not load texture: {0}", texturePath);
                    }
                }
                else
                {
                    NR_MESH_LOG("    No metalness texture");
                    //TODO: mi->Set("uMetalness", metalness);
                }

                bool metalnessTextureFound = false;
                for (uint32_t p = 0; p < aiMaterial->mNumProperties; ++p)
                {
                    auto prop = aiMaterial->mProperties[p];

                    if (prop->mType == aiPTI_String)
                    {
                        uint32_t strLength = *(uint32_t*)prop->mData;
                        std::string str(prop->mData + 4, strLength);

                        std::string key = prop->mKey.data;
                        if (key == "$raw.ReflectionFactor|file")
                        {
                            std::filesystem::path path = modelsDirectory;
                            auto parentPath = path.parent_path();
                            parentPath /= str;
                            std::string texturePath = parentPath.string();

                            NR_MESH_LOG("    Metalness map path = {0}", texturePath);
                            auto texture = Texture2D::Create(texturePath);
                            if (texture->Loaded())
                            {
                                metalnessTextureFound = true;
                                mi->Set("uMetalnessTexture", texture);
                                mi->Set("uMaterialUniforms.Metalness", 1.0f);
                            }
                            else
                            {
                                NR_CORE_ERROR("    Could not load Metalness texture");
                            }
                            break;
                        }
                    }
                }

                fallback = !metalnessTextureFound;
                if (fallback)
                {
                    NR_MESH_LOG("    No metalness map");
                    mi->Set("uMetalnessTexture", whiteTexture);
                    mi->Set("uMaterialUniforms.Metalness", metalness);
                }
            }

            NR_MESH_LOG("------------------------");
        }
        else
        {
            auto mi = Material::Create(Renderer::GetShaderLibrary()->Get("PBR_Static"), "NotRed-Default");
            mi->Set("uMaterialUniforms.AlbedoColor", glm::vec3(0.8f));
            mi->Set("uMaterialUniforms.Emission", 0.0f);
            mi->Set("uMaterialUniforms.Metalness", 0.0f);
            mi->Set("uMaterialUniforms.Roughness", 0.8f);
            mi->Set("uMaterialUniforms.UseNormalMap", false);

            mi->Set("uAlbedoTexture", whiteTexture);
            mi->Set("uMetalnessTexture", whiteTexture);
            mi->Set("uRoughnessTexture", whiteTexture);

            mMaterials.push_back(mi);
        }

        if (IsRigged())
        {
            mVertexBuffer = VertexBuffer::Create(mSkinnedVertices.data(), (uint32_t)(mSkinnedVertices.size() * sizeof(SkinnedVertex)));
        }
        else
        {
            mVertexBuffer = VertexBuffer::Create(mStaticVertices.data(), (uint32_t)(mStaticVertices.size() * sizeof(Vertex)));
        }

        mIndexBuffer = IndexBuffer::Create(mIndices.data(), (uint32_t)(mIndices.size() * sizeof(Index)));
    }

    MeshAsset::MeshAsset(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, const glm::mat4& transform)
        : mStaticVertices(vertices), mIndices(indices)
    {
        Submesh submesh;
        submesh.BaseVertex = 0;
        submesh.BaseIndex = 0;
        submesh.IndexCount = (uint32_t)indices.size() * 3u;
        submesh.VertexCount = (uint32_t)vertices.size();
        submesh.Transform = transform;
        mSubmeshes.push_back(submesh);

        mVertexBuffer = VertexBuffer::Create(mStaticVertices.data(), (uint32_t)(mStaticVertices.size() * sizeof(Vertex)));
        mIndexBuffer = IndexBuffer::Create(mIndices.data(), (uint32_t)(mIndices.size() * sizeof(Index)));
    }

    MeshAsset::MeshAsset(
        const std::vector<std::vector<Vertex>>& vertices,
        const std::vector<std::vector<Index>>& indices,
        const std::vector<std::string>& matNames
    )
    {
        mSubmeshes.reserve(vertices.size());
        uint32_t baseVertex = 0;
        uint32_t baseIndex = 0;
        for (int i = 0; i < vertices.size(); ++i)
        {
            Submesh& submesh = mSubmeshes.emplace_back();
            submesh.BaseVertex = baseVertex;
            submesh.BaseIndex = baseIndex;
            submesh.VertexCount = vertices[i].size();
            submesh.IndexCount = indices[i].size() * 3;
            submesh.MaterialIndex = i;

            baseVertex += vertices[i].size();
            baseIndex += indices[i].size() * 3;
        }

        for (auto& innerVec : vertices)
        {
            mStaticVertices.reserve(mStaticVertices.size() + innerVec.size());
            mStaticVertices.insert(mStaticVertices.end(), innerVec.begin(), innerVec.end());
        }
        for (auto& innerVec : indices)
        {
            mIndices.reserve(mIndices.size() + innerVec.size());
            mIndices.insert(mIndices.end(), innerVec.begin(), innerVec.end());
        }

        mVertexBuffer = VertexBuffer::Create(mStaticVertices.data(), (uint32_t)(mStaticVertices.size() * sizeof(Vertex)));
        mIndexBuffer = IndexBuffer::Create(mIndices.data(), (uint32_t)(mIndices.size() * sizeof(Index)));

        Ref<Texture2D> whiteTexture = Renderer::GetWhiteTexture();

        for (int i = 0; i < matNames.size(); ++i)
        {
            AssetHandle matAsset = AssetManager::GetAssetHandleFromFilePath(matNames[i]);
            if (matAsset == 0)
            {
                auto mi = Material::Create(Renderer::GetShaderLibrary()->Get("PBR_Static"), "NotRed-Default");
                mi->Set("uMaterialUniforms.AlbedoColor", glm::vec3(0.8f));
                mi->Set("uMaterialUniforms.Emission", 0.0f);
                mi->Set("uMaterialUniforms.Metalness", 0.0f);
                mi->Set("uMaterialUniforms.Roughness", 0.8f);
                mi->Set("uMaterialUniforms.UseNormalMap", false);

                mi->Set("uAlbedoTexture", whiteTexture);
                mi->Set("uMetalnessTexture", whiteTexture);
                mi->Set("uRoughnessTexture", whiteTexture);

                mMaterials.push_back(mi);
            }
            else
            {
                auto materialAsset = AssetManager::GetAsset<MaterialAsset>(matAsset);
                mMaterials.push_back(materialAsset->GetMaterial());
            }
        }

        if (vertices.size() > matNames.size())
        {
            auto mi = Material::Create(Renderer::GetShaderLibrary()->Get("PBR_Static"), "NotRed-Default");
            mi->Set("uMaterialUniforms.AlbedoColor", glm::vec3(0.8f));
            mi->Set("uMaterialUniforms.Emission", 0.0f);
            mi->Set("uMaterialUniforms.Metalness", 0.0f);
            mi->Set("uMaterialUniforms.Roughness", 0.8f);
            mi->Set("uMaterialUniforms.UseNormalMap", false);

            mi->Set("uAlbedoTexture", whiteTexture);
            mi->Set("uMetalnessTexture", whiteTexture);
            mi->Set("uRoughnessTexture", whiteTexture);

            for (uint32_t i = matNames.size(); i < vertices.size(); ++i)
            {
                mMaterials.push_back(mi);
            }
        }
    }

    MeshAsset::MeshAsset(int particleCount)
    {
        {
            for (uint32_t i = 0; i < particleCount; ++i)
            {
                ParticleVertex leftBot;
                leftBot.Position = { -0.5f, 0.0f, -0.5f };
                leftBot.Index = (float)i;
                mParticleVertices.push_back(leftBot);

                ParticleVertex rightBot;
                rightBot.Position = { 0.5f, 0.0f, -0.5f };
                rightBot.Index = (float)i;
                mParticleVertices.push_back(rightBot);

                ParticleVertex leftTop;
                leftTop.Position = { -0.5f, 0.0f,  0.5f };
                leftTop.Index = (float)i;
                mParticleVertices.push_back(leftTop);

                ParticleVertex rightTop;
                rightTop.Position = { 0.5f, 0.0f,  0.5f };
                rightTop.Index = (float)i;
                mParticleVertices.push_back(rightTop);

                unsigned int index = i * 4;
                Index indexA = { index + 0, index + 1, index + 2 };
                Index indexB = { index + 1, index + 3, index + 2 };
                mIndices.push_back(indexA);
                mIndices.push_back(indexB);
            }
        }

        auto mi = Material::Create(Renderer::GetShaderLibrary()->Get("Particle"), "Particle-Effect");
        mi->Set("uGalaxySpecs.StarColor", glm::vec3(1.0f, 1.0f, 1.0f));
        mi->Set("uGalaxySpecs.DustColor", glm::vec3(0.388f, 0.333f, 1.0f));
        mi->Set("uGalaxySpecs.h2RegionColor", glm::vec3(0.8f, 0.071f, 0.165f));
        mMaterials.push_back(mi);

        Submesh& submesh = mSubmeshes.emplace_back();
        submesh.BaseVertex = 0;
        submesh.BaseIndex = 0;
        submesh.IndexCount = mIndices.size() * 3;
        submesh.VertexCount = mParticleVertices.size();
        submesh.Transform = glm::mat4(1.0f);
        submesh.MaterialIndex = 0;
        submesh.MeshName = "Particles";

        mVertexBuffer = VertexBuffer::Create(mParticleVertices.data(), mParticleVertices.size() * sizeof(ParticleVertex));
        mIndexBuffer = IndexBuffer::Create(mIndices.data(), mIndices.size() * sizeof(Index));
    }

    void MeshAsset::DumpVertexBuffer()
    {
        NR_MESH_LOG("------------------------------------------------------");
        NR_MESH_LOG("Vertex Buffer Dump");
        NR_MESH_LOG("Mesh: {0}", mFilePath);
        if (IsRigged())
        {
            for (size_t i = 0; i < mSkinnedVertices.size(); ++i)
            {
                auto& vertex = mSkinnedVertices[i];
                NR_MESH_LOG("Vertex: {0}", i);
                NR_MESH_LOG("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
                NR_MESH_LOG("Normal: {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
                NR_MESH_LOG("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
                NR_MESH_LOG("Tangent: {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
                NR_MESH_LOG("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
                NR_MESH_LOG("--");
            }
        }
        else
        {
            for (size_t i = 0; i < mStaticVertices.size(); ++i)
            {
                auto& vertex = mStaticVertices[i];
                NR_MESH_LOG("Vertex: {0}", i);
                NR_MESH_LOG("Position: {0}, {1}, {2}", vertex.Position.x, vertex.Position.y, vertex.Position.z);
                NR_MESH_LOG("Normal: {0}, {1}, {2}", vertex.Normal.x, vertex.Normal.y, vertex.Normal.z);
                NR_MESH_LOG("Binormal: {0}, {1}, {2}", vertex.Binormal.x, vertex.Binormal.y, vertex.Binormal.z);
                NR_MESH_LOG("Tangent: {0}, {1}, {2}", vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z);
                NR_MESH_LOG("TexCoord: {0}, {1}", vertex.Texcoord.x, vertex.Texcoord.y);
                NR_MESH_LOG("--");
            }
        }
        NR_MESH_LOG("------------------------------------------------------");
    }

    void MeshAsset::TraverseNodes(aiNode* node, const glm::mat4& parentTransform, uint32_t level)
    {
        glm::mat4 transform = parentTransform * Mat4FromAIMatrix4x4(node->mTransformation);
        mNodeMap[node].resize(node->mNumMeshes);

        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            uint32_t mesh = node->mMeshes[i];
            auto& submesh = mSubmeshes[mesh];
            submesh.NodeName = node->mName.C_Str();
            submesh.Transform = transform;
            mNodeMap[node][i] = mesh;
        }

        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            TraverseNodes(node->mChildren[i], transform, level + 1);
        }
    }

    Mesh::Mesh(Ref<MeshAsset> meshAsset)
        : mMeshAsset(meshAsset)
    {
        SetSubmeshes({});

        const auto& meshMaterials = meshAsset->GetMaterials();
        mMaterials = Ref<MaterialTable>::Create(static_cast<uint32_t>(meshMaterials.size()));
        for (size_t i = 0; i < meshMaterials.size(); ++i)
        {
            mMaterials->SetMaterial(static_cast<uint32_t>(i), Ref<MaterialAsset>::Create(meshMaterials[i]));
        }

        if (mMeshAsset->IsRigged())
        {
            mBoneTransforms.resize(mMeshAsset->mBoneInfo.size());
            mBoneTransformUBs.resize(Renderer::GetConfig().FramesInFlight);
            for (auto& ub : mBoneTransformUBs)
            {
                ub = UniformBuffer::Create(static_cast<uint32_t>(meshAsset->mBoneInfo.size() * sizeof(glm::mat4)), 0);
            }
        }
    }

    Mesh::Mesh(Ref<MeshAsset> meshAsset, const std::vector<uint32_t>& submeshes)
        : mMeshAsset(meshAsset)
    {
        SetSubmeshes(submeshes);

        const auto& meshMaterials = meshAsset->GetMaterials();
        mMaterials = Ref<MaterialTable>::Create(static_cast<uint32_t>(meshMaterials.size()));
        for (size_t i = 0; i < meshMaterials.size(); ++i)
        {
            mMaterials->SetMaterial(static_cast<uint32_t>(i), Ref<MaterialAsset>::Create(meshMaterials[static_cast<uint32_t>(i)]));
        }

        if (mMeshAsset->IsRigged())
        {
            mBoneTransforms.resize(mMeshAsset->mBoneInfo.size());
            mBoneTransformUBs.resize(Renderer::GetConfig().FramesInFlight);
            for (auto& ub : mBoneTransformUBs)
            {
                ub = UniformBuffer::Create(static_cast<uint32_t>(meshAsset->mBoneInfo.size() * sizeof(glm::mat4)), 0);
            }
        }
    }

    Mesh::Mesh(const Ref<Mesh>& other)
        : mMeshAsset(other->mMeshAsset), mMaterials(other->mMaterials)
    {
        SetSubmeshes(other->mSubmeshes);

        if (mMeshAsset->IsRigged())
        {
            mBoneTransforms.resize(mMeshAsset->mBoneInfo.size());
            mBoneTransformUBs.resize(Renderer::GetConfig().FramesInFlight);
            for (auto& ub : mBoneTransformUBs)
            {
                ub = UniformBuffer::Create(static_cast<uint32_t>(mMeshAsset->mBoneInfo.size() * sizeof(glm::mat4)), 0);
            }
        }
    }

    void Mesh::UpdateBoneTransforms(const ozz::vector<ozz::math::Float4x4>& modelSpaceTransforms)
    {
        NR_PROFILE_FUNC();
        if (modelSpaceTransforms.empty())
        {
            for (size_t i = 0; i < mMeshAsset->mBoneInfo.size(); ++i)
            {
                mBoneTransforms[i] = ozz::math::Float4x4::identity();
            }
        }
        else
        {
            for (size_t i = 0; i < mMeshAsset->mBoneInfo.size(); ++i)
            {
                mBoneTransforms[i] = modelSpaceTransforms[mMeshAsset->mBoneInfo[i].JointIndex] * mMeshAsset->mBoneInfo[i].InverseBindPose;
            }
        }
        Ref<Mesh> instance = this;

        Renderer::Submit([instance]() mutable {
            instance->mBoneTransformUBs[Renderer::GetCurrentFrameIndex()]->RT_SetData(instance->mBoneTransforms.data(), static_cast<uint32_t>(instance->mBoneTransforms.size() * sizeof(ozz::math::Float4x4)));
            });
    }

    void Mesh::SetSubmeshes(const std::vector<uint32_t>& submeshes)
    {
        if (!submeshes.empty())
        {
            mSubmeshes = submeshes;
        }
        else
        {
            const auto& submeshes = mMeshAsset->GetSubmeshes();
            mSubmeshes.resize(submeshes.size());
            for (uint32_t i = 0; i < submeshes.size(); ++i)
            {
                mSubmeshes[i] = i;
            }
        }
    }

    void Mesh::AddVertices(const std::vector<Vertex>& vertices, uint32_t index)
    {
        Submesh& submesh = mMeshAsset->mSubmeshes[index];
        if (submesh.BaseVertex == 0 && index != 0)
        {
            submesh.BaseVertex = mMeshAsset->mStaticVertices.size();
        }
        submesh.VertexCount += (uint32_t)vertices.size();

        uint32_t startIndex = submesh.BaseVertex + submesh.VertexCount;

        mMeshAsset->mStaticVertices.insert(
            mMeshAsset->mStaticVertices.begin() + startIndex,
            vertices.begin(),
            vertices.end()
        );

        mMeshAsset->mVertexBuffer = VertexBuffer::Create(
            mMeshAsset->mStaticVertices.data(),
            (uint32_t)(mMeshAsset->mStaticVertices.size() * sizeof(Vertex))
        );
    }

    void Mesh::AddIndices(const std::vector<int>& indices, uint32_t subMeshIndex)
    {
        Submesh& submesh = mMeshAsset->mSubmeshes[subMeshIndex];
        if (submesh.BaseIndex == 0 && subMeshIndex != 0)
        {
            submesh.BaseIndex = mMeshAsset->mIndices.size();
        }
        submesh.IndexCount += indices.size();

        for (int i = 0; i < indices.size(); i += 3)
        {
            Index index = {
                (uint32_t)indices[i + 0],
                (uint32_t)indices[i + 1],
                (uint32_t)indices[i + 2]
            };
            mMeshAsset->mIndices.push_back(index);
            mMeshAsset->mTriangleCache[subMeshIndex].emplace_back(
                mMeshAsset->mStaticVertices[index.V1],
                mMeshAsset->mStaticVertices[index.V2],
                mMeshAsset->mStaticVertices[index.V3]
            );
        }
    }

    void Mesh::SetSubmeshesCount(int count)
    {
        mMeshAsset->mSubmeshes.clear();
        mSubmeshes.clear();

        for (int i = 0; i < count; ++i)
        {
            mMeshAsset->mSubmeshes.emplace_back();
            mSubmeshes.push_back(i);
        }
    }
}