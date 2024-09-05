#include "nrpch.h"
#include "PhysicsUtil.h"

#include <filesystem>

namespace NR
{
    physx::PxTransform ToPhysicsTransform(const TransformComponent& transform)
    {
        physx::PxQuat r = ToPhysicsQuat(glm::normalize(glm::quat(transform.Rotation)));
        physx::PxVec3 p = ToPhysicsVector(transform.Translation);
        return physx::PxTransform(p, r);
    }

    physx::PxTransform ToPhysicsTransform(const glm::vec3& translation, const glm::vec3& rotation)
    {
        return physx::PxTransform(ToPhysicsVector(translation), ToPhysicsQuat(glm::quat(rotation)));
    }

    physx::PxTransform ToPhysicsTransform(const glm::mat4& transform)
    {
        physx::PxQuat r = ToPhysicsQuat(glm::normalize(glm::toQuat(transform)));
        physx::PxVec3 p = ToPhysicsVector(glm::vec3(transform[3]));
        return physx::PxTransform(p, r);
    }

    physx::PxMat44 ToPhysicsMatrix(const glm::mat4& matrix)
    {
        return *(physx::PxMat44*)&matrix;
    }

    physx::PxVec3 ToPhysicsVector(const glm::vec3& vector)
    {
        return *(physx::PxVec3*)&vector;
    }

    physx::PxVec4 ToPhysicsVector(const glm::vec4& vector)
    {
        return *(physx::PxVec4*)&vector;
    }

    physx::PxQuat ToPhysicsQuat(const glm::quat& quat)
    {
        return physx::PxQuat(quat.x, quat.y, quat.z, quat.w);
    }

    glm::mat4 FromPhysicsTransform(const physx::PxTransform& transform)
    {
        glm::quat rotation = FromPhysicsQuat(transform.q);
        glm::vec3 position = FromPhysicsVector(transform.p);
        return glm::translate(glm::mat4(1.0F), position) * glm::toMat4(rotation);
    }

    glm::mat4 FromPhysicsMatrix(const physx::PxMat44& matrix)
    {
        return *(glm::mat4*)&matrix;
    }

    glm::vec3 FromPhysicsVector(const physx::PxVec3& vector)
    {
        return *(glm::vec3*)&vector;
    }

    glm::vec4 FromPhysicsVector(const physx::PxVec4& vector)
    {
        return *(glm::vec4*)&vector;
    }

    glm::quat FromPhysicsQuat(const physx::PxQuat& quat)
    {
        return *(glm::quat*)&quat;
    }

    physx::PxFilterFlags FilterShader(
        physx::PxFilterObjectAttributes attributes0,
        physx::PxFilterData filterData0,
        physx::PxFilterObjectAttributes attributes1,
        physx::PxFilterData filterData1,
        physx::PxPairFlags& pairFlags,
        const void* constantBlock,
        physx::PxU32 constantBlockSize
    )
    {
        if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
        {
            pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
            return physx::PxFilterFlag::eDEFAULT;
        }

        pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

        if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
        {
            pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
            pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
            return physx::PxFilterFlag::eDEFAULT;
        }

        return physx::PxFilterFlag::eSUPPRESS;
    }

    void PhysicsMeshSerializer::Delete(const std::string& filepath)
    {
        std::filesystem::path p = filepath;
        std::filesystem::path path = p.parent_path() / (p.filename().string() + ".pxm");

        size_t lastDot = path.filename().string().find_first_of(".");
        lastDot = lastDot == std::string::npos ? path.filename().string().length() - 1 : lastDot;
        std::string dirName = p.filename().string().substr(0, lastDot);

        if (IsSerialized(filepath))
        {
            std::filesystem::remove_all(p.parent_path() / dirName);
        }
    }

    void PhysicsMeshSerializer::SerializeMesh(const std::string& filepath, const physx::PxDefaultMemoryOutputStream& data, const std::string& submeshName)
    {
        std::filesystem::path p = filepath;
        std::filesystem::path path = p.parent_path() / (p.filename().string() + ".pxm");

        size_t lastDot = path.filename().string().find_first_of(".");
        lastDot = lastDot == std::string::npos ? path.filename().string().length() - 1 : lastDot;
        std::string dirName = p.filename().string().substr(0, lastDot);

        if (submeshName.length() > 0)
        {
            path = p.parent_path() / dirName / (submeshName + ".pxm");
        }

        std::filesystem::create_directory(p.parent_path() / dirName);
        std::string cachedFilepath = path.string();

        NR_CORE_INFO("Serializing {0}", submeshName);

        FILE* f = fopen(cachedFilepath.c_str(), "wb");
        if (f)
        {
            NR_CORE_INFO("File Created");
            fwrite(data.getData(), sizeof(physx::PxU8), data.getSize() / sizeof(physx::PxU8), f);
            fclose(f);
        }
        else
        {
            NR_CORE_INFO("File Already Exists");
        }
    }

    bool PhysicsMeshSerializer::IsSerialized(const std::string& filepath)
    {
        std::filesystem::path p = filepath;
        size_t lastDot = p.filename().string().find_first_of(".");
        lastDot = lastDot == std::string::npos ? p.filename().string().length() - 1 : lastDot;
        std::string dirName = p.filename().string().substr(0, lastDot);
        auto path = p.parent_path() / dirName;
        return std::filesystem::is_directory(path);
    }

    static physx::PxU8* sMeshDataBuffer;

    physx::PxDefaultMemoryInputData PhysicsMeshSerializer::DeserializeMesh(const std::string& filepath, const std::string& submeshName)
    {
        std::filesystem::path p = filepath;
        size_t lastDot = p.filename().string().find_first_of(".");
        lastDot = lastDot == std::string::npos ? p.filename().string().length() - 1 : lastDot;
        std::string dirName = p.filename().string().substr(0, lastDot);
        auto path = p.parent_path() / dirName;

        if (submeshName.length() > 0)
        {
            path = p.parent_path() / dirName / (submeshName + ".pxm");
        }

        FILE* f = fopen(path.string().c_str(), "rb");
        uint32_t size;

        if (f)
        {
            fseek(f, 0, SEEK_END);
            size = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (sMeshDataBuffer)
            {
                delete[] sMeshDataBuffer;
            }

            sMeshDataBuffer = new physx::PxU8[size / sizeof(physx::PxU8)];
            fread(sMeshDataBuffer, sizeof(physx::PxU8), size / sizeof(physx::PxU8), f);
            fclose(f);
        }

        return physx::PxDefaultMemoryInputData(sMeshDataBuffer, size);
    }
}