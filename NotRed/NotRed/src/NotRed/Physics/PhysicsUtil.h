#pragma once

#include <PxPhysicsAPI.h>

#include <glm/gtc/type_ptr.hpp>

#include "NotRed/Scene/Components.h"

namespace NR
{
    physx::PxTransform ToPhysicsTransform(const TransformComponent& transform);
    physx::PxTransform ToPhysicsTransform(const glm::mat4& transform);
    physx::PxMat44 ToPhysicsMatrix(const glm::mat4& matrix);
    physx::PxVec3 ToPhysicsVector(const glm::vec3& vector);
    physx::PxVec4 ToPhysicsVector(const glm::vec4& vector);
    physx::PxQuat ToPhysicsQuat(const glm::quat& quat);

    glm::mat4 FromPhysicsTransform(const physx::PxTransform& transform);
    glm::mat4 FromPhysicsMatrix(const physx::PxMat44& matrix);
    glm::vec3 FromPhysicsVector(const physx::PxVec3& vector);
    glm::vec4 FromPhysicsVector(const physx::PxVec4& vector);
    glm::quat FromPhysicsQuat(const physx::PxQuat& quat);

    physx::PxFilterFlags FilterShader(
        physx::PxFilterObjectAttributes attributes0, 
        physx::PxFilterData filterData0, 
        physx::PxFilterObjectAttributes attributes1,
        physx::PxFilterData filterData1, 
        physx::PxPairFlags& pairFlags, 
        const void* constantBlock, 
        physx::PxU32 constantBlockSize
    );

    class ConvexMeshSerializer
    {
    public:
        static void Delete(const std::string& filepath);
        static void SerializeMesh(const std::string& filepath, const physx::PxDefaultMemoryOutputStream& data, const std::string& submeshName = "");
        static bool IsSerialized(const std::string& filepath);
        static std::vector<physx::PxDefaultMemoryInputData> DeserializeMesh(const std::string& filepath);
        static void CleanupDataBuffers();
    };
}