#include "nrpch.h"
#include "PhysicsWrappers.h"

#include "NotRed/Script/ScriptEngine.h"

#include <glm/gtx/rotate_vector.hpp>

#include <extensions/PxRigidActorExt.h>

#include "NotRed/Math/Math.h"

#include "PhysicsManager.h"
#include "PhysicsLayer.h"
#include "PhysicsActor.h"

namespace NR
{
    static PhysicsErrorCallback sErrorCallback;
    static physx::PxDefaultAllocator sAllocator;
    static physx::PxFoundation* sFoundation;
    static physx::PxPvd* sPVD;
    static physx::PxPhysics* sPhysics;
    static physx::PxOverlapHit sOverlapBuffer[OVERLAP_MAX_COLLIDERS];

    static ContactListener sContactListener;

    void PhysicsErrorCallback::reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
    {
        const char* errorMessage = NULL;

        switch (code)
        {
        case physx::PxErrorCode::eNO_ERROR:	              errorMessage = "No Error"; break;
        case physx::PxErrorCode::eDEBUG_INFO:             errorMessage = "Info"; break;
        case physx::PxErrorCode::eDEBUG_WARNING:	      errorMessage = "Warning"; break;
        case physx::PxErrorCode::eINVALID_PARAMETER:      errorMessage = "Invalid Parameter"; break;
        case physx::PxErrorCode::eINVALID_OPERATION:      errorMessage = "Invalid Operation"; break;
        case physx::PxErrorCode::eOUT_OF_MEMORY:          errorMessage = "Out Of Memory"; break;
        case physx::PxErrorCode::eINTERNAL_ERROR:         errorMessage = "Internal Error"; break;
        case physx::PxErrorCode::eABORT:                  errorMessage = "Abort"; break;
        case physx::PxErrorCode::ePERF_WARNING:	          errorMessage = "Performance Warning"; break;
        case physx::PxErrorCode::eMASK_ALL:	              errorMessage = "Unknown Error"; break;
        }

        switch (code)
        {
        case physx::PxErrorCode::eNO_ERROR:
        case physx::PxErrorCode::eDEBUG_INFO:
        {
            NR_CORE_INFO("[Physics]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
            break;
        }
        case physx::PxErrorCode::eDEBUG_WARNING:
        case physx::PxErrorCode::ePERF_WARNING:
        {
            NR_CORE_WARN("[Physics]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
            break;
        }
        case physx::PxErrorCode::eINVALID_PARAMETER:
        case physx::PxErrorCode::eINVALID_OPERATION:
        case physx::PxErrorCode::eOUT_OF_MEMORY:
        case physx::PxErrorCode::eINTERNAL_ERROR:
        {
            NR_CORE_ERROR("[Physics]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
            break;
        }
        case physx::PxErrorCode::eABORT:
        case physx::PxErrorCode::eMASK_ALL:
        {
            NR_CORE_FATAL("[Physics]: {0}: {1} at {2} ({3})", errorMessage, message, file, line);
            NR_CORE_ASSERT(false);
            break;
        }
        }
    }

    void ContactListener::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
    {
        PX_UNUSED(constraints);
        PX_UNUSED(count);
    }

    void ContactListener::onWake(physx::PxActor** actors, physx::PxU32 count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            physx::PxActor& actor = *actors[i];
            Entity& entity = *(Entity*)actor.userData;

            NR_CORE_INFO("PhysX Actor waking up: ID: {0}, Name: {1}", entity.GetID(), entity.GetComponent<TagComponent>().Tag);
        }
    }

    void ContactListener::onSleep(physx::PxActor** actors, physx::PxU32 count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            physx::PxActor& actor = *actors[i];
            Entity& entity = *(Entity*)actor.userData;

            NR_CORE_INFO("PhysX Actor going to sleep: ID: {0}, Name: {1}", entity.GetID(), entity.GetComponent<TagComponent>().Tag);
        }
    }

    void ContactListener::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
    {
        Entity& a = *(Entity*)pairHeader.actors[0]->userData;
        Entity& b = *(Entity*)pairHeader.actors[1]->userData;

        if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
        {
            if (ScriptEngine::IsEntityModuleValid(a)) ScriptEngine::CollisionBegin(a);
            if (ScriptEngine::IsEntityModuleValid(b)) ScriptEngine::CollisionBegin(b);
        }
        else if (pairs->flags == physx::PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
        {
            if (ScriptEngine::IsEntityModuleValid(a)) ScriptEngine::CollisionEnd(a);
            if (ScriptEngine::IsEntityModuleValid(b)) ScriptEngine::CollisionEnd(b);
        }
    }

    void ContactListener::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
    {
        Entity& a = *(Entity*)pairs->triggerActor->userData;
        Entity& b = *(Entity*)pairs->otherActor->userData;

        if (pairs->status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            if (ScriptEngine::IsEntityModuleValid(a)) ScriptEngine::TriggerBegin(a);
            if (ScriptEngine::IsEntityModuleValid(b)) ScriptEngine::TriggerBegin(b);
        }
        else if (pairs->status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            if (ScriptEngine::IsEntityModuleValid(a)) ScriptEngine::TriggerEnd(a);
            if (ScriptEngine::IsEntityModuleValid(b)) ScriptEngine::TriggerEnd(b);
        }
    }

    void ContactListener::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
    {
        PX_UNUSED(bodyBuffer);
        PX_UNUSED(poseBuffer);
        PX_UNUSED(count);
    }

    static physx::PxBroadPhaseType::Enum ToPhysicsBroadphaseType(BroadphaseType type)
    {
        switch (type)
        {
        case BroadphaseType::SweepAndPrune:        return physx::PxBroadPhaseType::eSAP;
        case BroadphaseType::MultiBoxPrune:        return physx::PxBroadPhaseType::eMBP;
        case BroadphaseType::AutomaticBoxPrune:    return physx::PxBroadPhaseType::eABP;
        }

        return physx::PxBroadPhaseType::eABP;
    }

    static physx::PxFrictionType::Enum ToPhysXFrictionType(FrictionType type)
    {
        switch (type)
        {
        case FrictionType::Patch:			return physx::PxFrictionType::ePATCH;
        case FrictionType::OneDirectional:	return physx::PxFrictionType::eONE_DIRECTIONAL;
        case FrictionType::TwoDirectional:	return physx::PxFrictionType::eTWO_DIRECTIONAL;
        }

        return physx::PxFrictionType::ePATCH;
    }

    physx::PxScene* PhysicsWrappers::CreateScene()
    {
        physx::PxSceneDesc sceneDesc(sPhysics->getTolerancesScale());

        const PhysicsSettings& settings = PhysicsManager::GetSettings();

        sceneDesc.gravity = ToPhysicsVector(settings.Gravity);
        sceneDesc.broadPhaseType = ToPhysicsBroadphaseType(settings.BroadphaseAlgorithm);
        sceneDesc.cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(1);
        sceneDesc.filterShader = FilterShader;
        sceneDesc.simulationEventCallback = &sContactListener;
        sceneDesc.frictionType = ToPhysXFrictionType(settings.FrictionModel);

        NR_CORE_ASSERT(sceneDesc.isValid());
        return sPhysics->createScene(sceneDesc);
    }

    void PhysicsWrappers::AddBoxCollider(PhysicsActor& actor)
    {
        auto& collider = actor.mEntity.GetComponent<BoxColliderComponent>();

        if (!collider.Material)
        {
            collider.Material = Ref<PhysicsMaterial>::Create(0.6f, 0.6f, 0.0f);
        }

        glm::vec3 size = actor.mEntity.Transform().Scale;
        glm::vec3 colliderSize = collider.Size;

        if (size.x != 0.0f)
        {
            colliderSize.x *= size.x;
        }
        if (size.y != 0.0f)
        {
            colliderSize.y *= size.y;
        }
        if (size.z != 0.0f)
        {
            colliderSize.z *= size.z;
        }

        physx::PxBoxGeometry boxGeometry = physx::PxBoxGeometry(colliderSize.x / 2.0f, colliderSize.y / 2.0f, colliderSize.z / 2.0f);
        physx::PxMaterial* material = sPhysics->createMaterial(collider.Material->StaticFriction, collider.Material->DynamicFriction, collider.Material->Bounciness);
        physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor.mActorInternal, boxGeometry, *material);
        shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !collider.IsTrigger);
        shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, collider.IsTrigger);
        shape->setLocalPose(ToPhysicsTransform(glm::translate(glm::mat4(1.0f), collider.Offset)));
    }

    void PhysicsWrappers::AddSphereCollider(PhysicsActor& actor)
    {
		auto& collider = actor.mEntity.GetComponent<SphereColliderComponent>();

        if (!collider.Material)
        {
            collider.Material = Ref<PhysicsMaterial>::Create(0.6f, 0.6f, 0.0f);
        }

        float colliderRadius = collider.Radius;
        glm::vec3 size = actor.mEntity.Transform().Scale;

        if (size.x != 0.0f)
        {
            colliderRadius *= (size.x / 2.0f);
        }

        physx::PxSphereGeometry sphereGeometry = physx::PxSphereGeometry(colliderRadius);
        physx::PxMaterial* material = sPhysics->createMaterial(collider.Material->StaticFriction, collider.Material->DynamicFriction, collider.Material->Bounciness);
        physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor.mActorInternal, sphereGeometry, *material);
        shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !collider.IsTrigger);
        shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, collider.IsTrigger);
    }

    void PhysicsWrappers::AddCapsuleCollider(PhysicsActor& actor)
    {
        auto& collider = actor.mEntity.GetComponent<CapsuleColliderComponent>();

        if (!collider.Material)
        {
            collider.Material = Ref<PhysicsMaterial>::Create(0.6f, 0.6f, 0.0f);
        }

        float colliderRadius = collider.Radius;
        float colliderHeight = collider.Height;

        glm::vec3 size = actor.mEntity.Transform().Scale;
        if (size.x != 0.0f)
        {
            colliderRadius *= size.x;
        }

        if (size.y != 0.0f)
        {
            colliderHeight *= size.y;
        }

        physx::PxCapsuleGeometry capsuleGeometry = physx::PxCapsuleGeometry(colliderRadius, colliderHeight / 2.0f);
        physx::PxMaterial* material = sPhysics->createMaterial(collider.Material->StaticFriction, collider.Material->DynamicFriction, collider.Material->Bounciness);
        physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor.mActorInternal, capsuleGeometry, *material);
        shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !collider.IsTrigger);
        shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, collider.IsTrigger);
        shape->setLocalPose(physx::PxTransform(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0, 0, 1))));
    }

    void PhysicsWrappers::AddMeshCollider(PhysicsActor& actor)
    {
        auto& collider = actor.mEntity.GetComponent<MeshColliderComponent>();

        if (!collider.Material)
        {
            collider.Material = Ref<PhysicsMaterial>::Create(0.6f, 0.6f, 0.0f);
        }

        glm::vec3 size = actor.mEntity.Transform().Scale;
        physx::PxMaterial* material = sPhysics->createMaterial(collider.Material->StaticFriction, collider.Material->DynamicFriction, collider.Material->Bounciness);
        physx::PxMaterial* materials[] = { material };

        if (collider.IsConvex)
        {
            std::vector<physx::PxShape*> shapes = CreateConvexMesh(collider, size);

            for (auto shape : shapes)
            {

                shape->setMaterials(materials, 1);
                shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !collider.IsTrigger);
                shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, collider.IsTrigger);
                actor.AddCollisionShape(shape);
            }
        }
        else
        {
            std::vector<physx::PxShape*> shapes = CreateTriangleMesh(collider, size);

            for (auto shape : shapes)
            {
                shape->setMaterials(materials, 1);
                shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !collider.IsTrigger);
                shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, collider.IsTrigger);
                actor.AddCollisionShape(shape);
            }
        }
    }

    std::vector<physx::PxShape*> PhysicsWrappers::CreateConvexMesh(MeshColliderComponent& collider, const glm::vec3& size, bool invalidateOld)
    {
        std::vector<physx::PxShape*> shapes;

        collider.ProcessedMeshes.clear();

        physx::PxTolerancesScale sc;
        physx::PxCookingParams newParams(sc);
        newParams.planeTolerance = 0.0f;
        newParams.meshPreprocessParams = physx::PxMeshPreprocessingFlags(physx::PxMeshPreprocessingFlag::eWELD_VERTICES);
        newParams.meshWeldTolerance = 0.01f;

        if (invalidateOld)
        {
            PhysicsMeshSerializer::Delete(collider.CollisionMesh->GetFilePath());
        }

        if (!PhysicsMeshSerializer::IsSerialized(collider.CollisionMesh->GetFilePath()))
        {
            const std::vector<Vertex>& vertices = collider.CollisionMesh->GetStaticVertices();
            const std::vector<Index>& indices = collider.CollisionMesh->GetIndices();

            for (const auto& submesh : collider.CollisionMesh->GetSubmeshes())
            {
                physx::PxConvexMeshDesc convexDesc;
                convexDesc.points.count = submesh.VertexCount;
                convexDesc.points.stride = sizeof(Vertex);
                convexDesc.points.data = &vertices[submesh.BaseVertex];
                convexDesc.indices.count = submesh.IndexCount / 3;
                convexDesc.indices.data = &indices[submesh.BaseIndex / 3];
                convexDesc.indices.stride = sizeof(Index);
                convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX | physx::PxConvexFlag::eSHIFT_VERTICES;

                physx::PxDefaultMemoryOutputStream buf;
                physx::PxConvexMeshCookingResult::Enum result;
                if (!PxCookConvexMesh(newParams, convexDesc, buf, &result))
                {
                    NR_CORE_ERROR("Failed to cook convex mesh {0}", submesh.MeshName);
                    continue;
                }

                PhysicsMeshSerializer::SerializeMesh(collider.CollisionMesh->GetFilePath(), buf, submesh.MeshName);
                physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
                physx::PxConvexMesh* convexMesh = sPhysics->createConvexMesh(input);
                physx::PxConvexMeshGeometry convexGeometry = physx::PxConvexMeshGeometry(convexMesh, physx::PxMeshScale(ToPhysicsVector(size)));
                convexGeometry.meshFlags = physx::PxConvexMeshGeometryFlag::eTIGHT_BOUNDS;
                physx::PxMaterial* material = sPhysics->createMaterial(0, 0, 0); 
                physx::PxShape* shape = sPhysics->createShape(convexGeometry, *material, true);
                shape->setLocalPose(ToPhysicsTransform(submesh.Transform));
                shapes.push_back(shape);
            }
        }
        else
        {
            for (const auto& submesh : collider.CollisionMesh->GetSubmeshes())
            {
                physx::PxDefaultMemoryInputData meshData = PhysicsMeshSerializer::DeserializeMesh(collider.CollisionMesh->GetFilePath(), submesh.MeshName);
                physx::PxConvexMesh* convexMesh = sPhysics->createConvexMesh(meshData);
                physx::PxConvexMeshGeometry convexGeometry = physx::PxConvexMeshGeometry(convexMesh, physx::PxMeshScale(ToPhysicsVector(size)));
                convexGeometry.meshFlags = physx::PxConvexMeshGeometryFlag::eTIGHT_BOUNDS;
                physx::PxMaterial* material = sPhysics->createMaterial(0, 0, 0);
                physx::PxShape* shape = sPhysics->createShape(convexGeometry, *material, true);
                shape->setLocalPose(ToPhysicsTransform(submesh.Transform));
                shapes.push_back(shape);
            }
        }

        if (collider.ProcessedMeshes.size() <= 0)
        {
            for (auto shape : shapes)
            {
                physx::PxGeometryHolder geometryHolder = shape->getGeometry();
                physx::PxConvexMesh* mesh = geometryHolder.convexMesh().convexMesh;

                // Based On: https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/ThirdParty/PhysX3/NvCloth/samples/SampleBase/renderer/ConvexRenderMesh.cpp
                const uint32_t nbPolygons = mesh->getNbPolygons();
                const physx::PxVec3* convexVertices = mesh->getVertices();
                const physx::PxU8* convexIndices = mesh->getIndexBuffer();

                uint32_t nbVertices = 0;
                uint32_t nbFaces = 0;

                std::vector<Vertex> collisionVertices;
                std::vector<Index> collisionIndices;
                uint32_t vertCounter = 0;
                uint32_t indexCounter = 0;

                for (uint32_t i = 0; i < nbPolygons; ++i)
                {
                    physx::PxHullPolygon polygon;
                    mesh->getPolygonData(i, polygon);
                    nbVertices += polygon.mNbVerts;
                    nbFaces += (polygon.mNbVerts - 2) * 3;

                    uint32_t vI0 = vertCounter;

                    for (uint32_t vI = 0; vI < polygon.mNbVerts; vI++)
                    {
                        Vertex v;
                        v.Position = FromPhysicsVector(convexVertices[convexIndices[polygon.mIndexBase + vI]]);
                        collisionVertices.push_back(v);
                        ++vertCounter;
                    }

                    for (uint32_t vI = 1; vI < uint32_t(polygon.mNbVerts) - 1; ++vI)
                    {
                        Index index;
                        index.V1 = uint32_t(vI0);
                        index.V2 = uint32_t(vI0 + vI + 1);
                        index.V3 = uint32_t(vI0 + vI);
                        collisionIndices.push_back(index);
                        ++indexCounter;
                    }

                    collider.ProcessedMeshes.push_back(Ref<Mesh>::Create(collisionVertices, collisionIndices, FromPhysicsTransform(shape->getLocalPose())));
                }
            }
        }

        return shapes;
    }

    std::vector<physx::PxShape*> PhysicsWrappers::CreateTriangleMesh(MeshColliderComponent& collider, const glm::vec3& scale, bool invalidateOld)
    {
        std::vector<physx::PxShape*> shapes;

        collider.ProcessedMeshes.clear();

        if (invalidateOld)
        {
            PhysicsMeshSerializer::Delete(collider.CollisionMesh->GetFilePath());
        }

        if (!PhysicsMeshSerializer::IsSerialized(collider.CollisionMesh->GetFilePath()))
        {
            const std::vector<Vertex>& vertices = collider.CollisionMesh->GetStaticVertices();
            const std::vector<Index>& indices = collider.CollisionMesh->GetIndices();

            for (const auto& submesh : collider.CollisionMesh->GetSubmeshes())
            {
                physx::PxTolerancesScale sc;
                physx::PxCookingParams newParams(sc);

                physx::PxTriangleMeshDesc triangleDesc;
                triangleDesc.points.count = submesh.VertexCount;
                triangleDesc.points.stride = sizeof(Vertex);
                triangleDesc.points.data = &vertices[submesh.BaseVertex];
                triangleDesc.triangles.count = submesh.IndexCount / 3;
                triangleDesc.triangles.data = &indices[submesh.BaseIndex / 3];
                triangleDesc.triangles.stride = sizeof(Index);

                physx::PxDefaultMemoryOutputStream buf;
                physx::PxTriangleMeshCookingResult::Enum result;
                if (!PxCookTriangleMesh(newParams, triangleDesc, buf, &result))
                {
                    NR_CORE_ERROR("Failed to cook triangle mesh: {0}", submesh.MeshName);
                    continue;
                }

                PhysicsMeshSerializer::SerializeMesh(collider.CollisionMesh->GetFilePath(), buf, submesh.MeshName);

                glm::vec3 submeshTranslation, submeshRotation, submeshScale;
                Math::DecomposeTransform(submesh.LocalTransform, submeshTranslation, submeshRotation, submeshScale);

                physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
                physx::PxTriangleMesh* trimesh = sPhysics->createTriangleMesh(input);
                physx::PxTriangleMeshGeometry triangleGeometry = physx::PxTriangleMeshGeometry(trimesh, physx::PxMeshScale(ToPhysicsVector(submeshScale * scale)));
                physx::PxMaterial* material = sPhysics->createMaterial(0, 0, 0);
                physx::PxShape* shape = sPhysics->createShape(triangleGeometry, *material, true);
                shape->setLocalPose(ToPhysicsTransform(submeshTranslation, submeshRotation));
                shapes.push_back(shape);
            }
        }
        else
        {
            for (const auto& submesh : collider.CollisionMesh->GetSubmeshes())
            {
                glm::vec3 submeshTranslation, submeshRotation, submeshScale;
                Math::DecomposeTransform(submesh.LocalTransform, submeshTranslation, submeshRotation, submeshScale);

                physx::PxDefaultMemoryInputData meshData = PhysicsMeshSerializer::DeserializeMesh(collider.CollisionMesh->GetFilePath(), submesh.MeshName);
                physx::PxTriangleMesh* trimesh = sPhysics->createTriangleMesh(meshData);
                physx::PxTriangleMeshGeometry triangleGeometry = physx::PxTriangleMeshGeometry(trimesh, physx::PxMeshScale(ToPhysicsVector(submeshScale * scale)));
                physx::PxMaterial* material = sPhysics->createMaterial(0, 0, 0);
                physx::PxShape* shape = sPhysics->createShape(triangleGeometry, *material, true);
                shape->setLocalPose(ToPhysicsTransform(submeshTranslation, submeshRotation));
                shapes.push_back(shape);
            }
        }

        if (collider.ProcessedMeshes.size() <= 0)
        {
            for (auto shape : shapes)
            {
                physx::PxGeometryHolder geometryHolder = shape->getGeometry();
                physx::PxTriangleMeshGeometry triangleGeometry = geometryHolder.triangleMesh();
                physx::PxTriangleMesh* mesh = triangleGeometry.triangleMesh;

                const uint32_t nbVerts = mesh->getNbVertices();
                const physx::PxVec3* triangleVertices = mesh->getVertices();
                const uint32_t nbTriangles = mesh->getNbTriangles();
                const physx::PxU16* tris = (const physx::PxU16*)mesh->getTriangles();

                std::vector<Vertex> vertices;
                std::vector<Index> indices;

                for (uint32_t v = 0; v < nbVerts; ++v)
                {
                    Vertex v1;
                    v1.Position = FromPhysicsVector(triangleVertices[v]);
                    vertices.push_back(v1);
                }

                for (uint32_t tri = 0; tri < nbTriangles; ++tri)
                {
                    Index index;
                    index.V1 = tris[3 * tri + 0];
                    index.V2 = tris[3 * tri + 1];
                    index.V3 = tris[3 * tri + 2];
                    indices.push_back(index);
                }

                glm::mat4 scale = glm::scale(glm::mat4(1.0f), *(glm::vec3*)&triangleGeometry.scale.scale);
                //scale = glm::mat4(1.0f);
                glm::mat4 transform = FromPhysicsTransform(shape->getLocalPose()) * scale;
                collider.ProcessedMeshes.push_back(Ref<Mesh>::Create(vertices, indices, transform));
            }
        }

        return shapes;
    }

    bool PhysicsWrappers::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit* hit)
    {
        physx::PxScene* scene = static_cast<physx::PxScene*>(PhysicsManager::GetPhysicsScene());
        physx::PxRaycastBuffer hitInfo;
        bool result = scene->raycast(ToPhysicsVector(origin), ToPhysicsVector(glm::normalize(direction)), maxDistance, hitInfo);

        if (result)
        {
            Entity& entity = *(Entity*)hitInfo.block.actor->userData;

            hit->EntityID = entity.GetID();
            hit->Position = FromPhysicsVector(hitInfo.block.position);
            hit->Normal = FromPhysicsVector(hitInfo.block.normal);
            hit->Distance = hitInfo.block.distance;
        }

        return result;
    }

    bool PhysicsWrappers::OverlapBox(const glm::vec3& origin, const glm::vec3& halfSize, std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t* count)
    {
        physx::PxScene* scene = static_cast<physx::PxScene*>(PhysicsManager::GetPhysicsScene());

        memset(sOverlapBuffer, 0, sizeof(sOverlapBuffer));
        physx::PxOverlapBuffer buf(sOverlapBuffer, OVERLAP_MAX_COLLIDERS);
        physx::PxBoxGeometry geometry = physx::PxBoxGeometry(halfSize.x, halfSize.y, halfSize.z);
        physx::PxTransform pose = ToPhysicsTransform(glm::translate(glm::mat4(1.0f), origin));

        bool result = scene->overlap(geometry, pose, buf);

        if (result)
        {
            uint32_t bodyCount = buf.nbTouches >= OVERLAP_MAX_COLLIDERS ? OVERLAP_MAX_COLLIDERS : buf.nbTouches;
            memcpy(buffer.data(), buf.touches, bodyCount * sizeof(physx::PxOverlapHit));
            *count = bodyCount;
        }

        return result;
    }

    bool PhysicsWrappers::OverlapCapsule(const glm::vec3& origin, float radius, float halfHeight, std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t* count)
    {
        physx::PxScene* scene = static_cast<physx::PxScene*>(PhysicsManager::GetPhysicsScene());

        memset(sOverlapBuffer, 0, sizeof(sOverlapBuffer));
        physx::PxOverlapBuffer buf(sOverlapBuffer, OVERLAP_MAX_COLLIDERS);
        physx::PxCapsuleGeometry geometry = physx::PxCapsuleGeometry(radius, halfHeight);
        physx::PxTransform pose = ToPhysicsTransform(glm::translate(glm::mat4(1.0f), origin));

        bool result = scene->overlap(geometry, pose, buf);

        if (result)
        {
            uint32_t bodyCount = buf.nbTouches >= OVERLAP_MAX_COLLIDERS ? OVERLAP_MAX_COLLIDERS : buf.nbTouches;
            memcpy(buffer.data(), buf.touches, bodyCount * sizeof(physx::PxOverlapHit));
            *count = bodyCount;
        }

        return result;
    }

    bool PhysicsWrappers::OverlapSphere(const glm::vec3& origin, float radius, std::array<physx::PxOverlapHit, OVERLAP_MAX_COLLIDERS>& buffer, uint32_t* count)
    {
        physx::PxScene* scene = static_cast<physx::PxScene*>(PhysicsManager::GetPhysicsScene());

        memset(sOverlapBuffer, 0, sizeof(sOverlapBuffer));
        physx::PxOverlapBuffer buf(sOverlapBuffer, OVERLAP_MAX_COLLIDERS);
        physx::PxSphereGeometry geometry = physx::PxSphereGeometry(radius);
        physx::PxTransform pose = ToPhysicsTransform(glm::translate(glm::mat4(1.0f), origin));

        bool result = scene->overlap(geometry, pose, buf);

        if (result)
        {
            uint32_t bodyCount = buf.nbTouches >= OVERLAP_MAX_COLLIDERS ? OVERLAP_MAX_COLLIDERS : buf.nbTouches;
            memcpy(buffer.data(), buf.touches, bodyCount * sizeof(physx::PxOverlapHit));
            *count = bodyCount;
        }

        return result;
    }

    void PhysicsWrappers::Initialize()
    {
        NR_CORE_ASSERT(!sFoundation, "PhysicsWrappers::Initializer shouldn't be called more than once!");

        sFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, sAllocator, sErrorCallback);
        NR_CORE_ASSERT(sFoundation, "PxCreateFoundation Failed!");

        sPVD = PxCreatePvd(*sFoundation);
        if (sPVD)
        {
            physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
            sPVD->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
        }

        physx::PxTolerancesScale scale = physx::PxTolerancesScale();
        scale.length = 10;
        sPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *sFoundation, scale, true, sPVD);
        NR_CORE_ASSERT(sPhysics, "PxCreatePhysics Failed!");
    }

    void PhysicsWrappers::Shutdown()
    {
        sPhysics->release();
        sFoundation->release();
    }

    physx::PxPhysics& PhysicsWrappers::GetPhysics()
    {
        return *sPhysics;
    }

    physx::PxAllocatorCallback& PhysicsWrappers::GetAllocator()
    {
        return sAllocator;
    }
}