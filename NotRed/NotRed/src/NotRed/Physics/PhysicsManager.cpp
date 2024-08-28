#include "nrpch.h"
#include "PhysicsManager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#define PHYSX_DEBUGGER 1

namespace NR
{
    static physx::PxSimulationFilterShader sDefaultFilterShader = physx::PxDefaultSimulationFilterShader;

    static std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4& transform)
    {
        glm::vec3 scale, translation, skew;
        glm::vec4 perspective;
        glm::quat orientation;
        glm::decompose(transform, scale, orientation, translation, skew, perspective);

        return { translation, orientation, scale };
    }

    static physx::PxFilterFlags FilterShader(
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
        }

        return physx::PxFilterFlag::eDEFAULT;
    }


    void PhysicsManager::Init()
    {
        NR_CORE_ASSERT(!sPXFoundation, "PhysXManager::Init shouldn't be called more than once!");

        sPXFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, sPXAllocator, sPXErrorCallback);
        NR_CORE_ASSERT(sPXFoundation, "PxCreateFoundation Failed!");

#if PHYSX_DEBUGGER
        sPXPvd = PxCreatePvd(*sPXFoundation);
        physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
        sPXPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
#endif
        sPXPhysicsFactory = PxCreatePhysics(PX_PHYSICS_VERSION, *sPXFoundation, physx::PxTolerancesScale(), true, sPXPvd);
        NR_CORE_ASSERT(sPXPhysicsFactory, "PxCreatePhysics Failed!");
    }

    void PhysicsManager::Shutdown()
    {
        sPXPhysicsFactory->release();
        sPXFoundation->release();
    }

    physx::PxSceneDesc PhysicsManager::CreateSceneDesc()
    {
        physx::PxSceneDesc sceneDesc(sPXPhysicsFactory->getTolerancesScale());
        if (!sceneDesc.cpuDispatcher)
        {
            physx::PxDefaultCpuDispatcher* mCpuDispatcher = physx::PxDefaultCpuDispatcherCreate(1);
            NR_CORE_ASSERT(mCpuDispatcher);
            sceneDesc.cpuDispatcher = mCpuDispatcher;
        }

        if (!sceneDesc.filterShader)
        {
            sceneDesc.filterShader = FilterShader;
        }

        return sceneDesc;
    }

    physx::PxScene* PhysicsManager::CreateScene(const physx::PxSceneDesc& sceneDesc)
    {
        return sPXPhysicsFactory->createScene(sceneDesc);
    }

    physx::PxRigidActor* PhysicsManager::AddActor(physx::PxScene* scene, const RigidBodyComponent& rigidbody, const glm::mat4& transform)
    {
        physx::PxRigidActor* actor = nullptr;

        if (rigidbody.BodyType == RigidBodyComponent::Type::Static)
        {
            actor = sPXPhysicsFactory->createRigidStatic(CreatePose(transform));
        }
        else if (rigidbody.BodyType == RigidBodyComponent::Type::Dynamic)
        {
            physx::PxRigidDynamic* dynamicActor = sPXPhysicsFactory->createRigidDynamic(CreatePose(transform));
            physx::PxRigidBodyExt::updateMassAndInertia(*dynamicActor, rigidbody.Mass);

            dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X, rigidbody.LockPositionX);
            dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, rigidbody.LockPositionY);
            dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, rigidbody.LockPositionZ);
            dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, rigidbody.LockRotationX);
            dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, rigidbody.LockRotationY);
            dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, rigidbody.LockRotationZ);

            actor = dynamicActor;
        }

        scene->addActor(*actor);

        return actor;
    }

    physx::PxMaterial* PhysicsManager::CreateMaterial(float staticFriction, float dynamicFriction, float restitution)
    {
        return sPXPhysicsFactory->createMaterial(staticFriction, dynamicFriction, restitution);
    }

    physx::PxConvexMesh* PhysicsManager::CreateMeshCollider(const Ref<Mesh>& mesh)
    {
        const auto& vertices = mesh->GetStaticVertices();
        const auto& indices = mesh->GetIndices();

        physx::PxTolerancesScale scale;
        physx::PxCookingParams params(scale);

        physx::PxConvexMeshDesc convexDesc;
        convexDesc.points.count = vertices.size();
        convexDesc.points.stride = sizeof(Vertex);
        convexDesc.points.data = vertices.data();
        convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;
        const physx::PxConvexMeshDesc desc = convexDesc;

        physx::PxDefaultMemoryOutputStream buf;
        if (!PxCookConvexMesh(params, desc, buf))
        {
            return nullptr;
        }
        physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
        return sPXPhysicsFactory->createConvexMesh(input);
    }

    physx::PxTransform PhysicsManager::CreatePose(const glm::mat4& transform)
    {
        auto [translation, rotationQuat, scale] = DecomposeTransform(transform);
        glm::vec3 rotation = glm::eulerAngles(rotationQuat);

        physx::PxTransform physxTransform(physx::PxVec3(translation.x, translation.y, translation.z));
        physxTransform.rotate(physx::PxVec3(rotation.x, rotation.y, rotation.z));
        return physxTransform;
    }

    void PhysicsManager::SetCollisionFilters(physx::PxRigidActor* actor, uint32_t filterGroup, uint32_t filterMask)
    {
        physx::PxFilterData filterData;
        filterData.word0 = filterGroup; // word0 = own ID
        filterData.word1 = filterMask;  // word1 = ID mask to filter pairs that trigger a contact callback;
        const physx::PxU32 numShapes = actor->getNbShapes();
        physx::PxShape** shapes = (physx::PxShape**)sPXAllocator.allocate(sizeof(physx::PxShape*) * numShapes, "", "", 0);
        actor->getShapes(shapes, numShapes);
        for (physx::PxU32 i = 0; i < numShapes; i++)
        {
            physx::PxShape* shape = shapes[i];
            shape->setSimulationFilterData(filterData);
        }
        sPXAllocator.deallocate(shapes);
    }

    physx::PxDefaultErrorCallback PhysicsManager::sPXErrorCallback;
    physx::PxDefaultAllocator PhysicsManager::sPXAllocator;
    physx::PxFoundation* PhysicsManager::sPXFoundation;
    physx::PxPhysics* PhysicsManager::sPXPhysicsFactory;
    physx::PxPvd* PhysicsManager::sPXPvd;

}