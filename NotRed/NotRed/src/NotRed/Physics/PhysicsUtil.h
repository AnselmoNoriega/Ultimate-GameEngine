#pragma once

#include <PxPhysicsAPI.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace NR
{
    physx::PxTransform ToPhysicsTransform(const glm::mat4& matrix);
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

    class ContactListener : public physx::PxSimulationEventCallback
    {
    public:
        virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
        virtual void onWake(physx::PxActor** actors, physx::PxU32 count) override;
        virtual void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
        virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
        virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
        virtual void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;
    };
}