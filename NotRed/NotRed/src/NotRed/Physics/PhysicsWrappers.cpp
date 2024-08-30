#include "nrpch.h"
#include "PhysicsWrappers.h"

#include <glm/gtx/rotate_vector.hpp>

#include "PhysicsManager.h"
#include "PhysicsLayer.h"

#ifdef NR_DEBUG
	#define PHYSX_DEBUGGER 0
#endif

namespace NR
{
	static physx::PxDefaultErrorCallback sErrorCallback;
	static physx::PxDefaultAllocator sAllocator;
	static physx::PxFoundation* sFoundation;
	static physx::PxPhysics* sPhysics;
	static physx::PxPvd* sVisualDebugger = nullptr;

	static physx::PxSimulationFilterShader sFilterShader = physx::PxDefaultSimulationFilterShader;

	static ContactListener sContactListener;

	physx::PxScene* PhysicsWrappers::CreateScene(const SceneParams& sceneParams)
	{
		physx::PxSceneDesc sceneDesc(sPhysics->getTolerancesScale());

		sceneDesc.gravity = ToPhysicsVector(sceneParams.Gravity);
		sceneDesc.cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(1);
		sceneDesc.filterShader = FilterShader;
		sceneDesc.simulationEventCallback = &sContactListener;

		NR_CORE_ASSERT(sceneDesc.isValid());
		return sPhysics->createScene(sceneDesc);
	}

	physx::PxRigidActor* PhysicsWrappers::CreateActor(const RigidBodyComponent& rigidbody, const glm::mat4& transform)
	{
		physx::PxRigidActor* actor = nullptr;

		if (rigidbody.BodyType == RigidBodyComponent::Type::Static)
		{
			actor = sPhysics->createRigidStatic(ToPhysicsTransform(transform));
		}
		else if (rigidbody.BodyType == RigidBodyComponent::Type::Dynamic)
		{
			physx::PxRigidDynamic* dynamicActor = sPhysics->createRigidDynamic(ToPhysicsTransform(transform));

			dynamicActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, rigidbody.IsKinematic);

			dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X, rigidbody.LockPositionX);
			dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, rigidbody.LockPositionY);
			dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, rigidbody.LockPositionZ);
			dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, rigidbody.LockRotationX);
			dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, rigidbody.LockRotationY);
			dynamicActor->setRigidDynamicLockFlag(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, rigidbody.LockRotationZ);

			physx::PxRigidBodyExt::updateMassAndInertia(*dynamicActor, rigidbody.Mass);
			actor = dynamicActor;
		}

		return actor;
	}

	void PhysicsWrappers::SetCollisionFilters(const physx::PxRigidActor& actor, uint32_t physicsLayer)
	{
		const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(physicsLayer);

		if (layerInfo.CollidesWith == 0)
		{
			return;
		}

		physx::PxFilterData filterData;
		filterData.word0 = layerInfo.BitValue;
		filterData.word1 = layerInfo.CollidesWith;

		const physx::PxU32 numShapes = actor.getNbShapes();

		physx::PxShape** shapes = (physx::PxShape**)sAllocator.allocate(sizeof(physx::PxShape*) * numShapes, "", "", 0);
		actor.getShapes(shapes, numShapes);

		for (physx::PxU32 i = 0; i < numShapes; ++i)
		{
			shapes[i]->setSimulationFilterData(filterData);
		}

		sAllocator.deallocate(shapes);
	}

	void PhysicsWrappers::AddBoxCollider(physx::PxRigidActor& actor, const physx::PxMaterial& material, const BoxColliderComponent& collider, const glm::vec3& size)
	{
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
		physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(actor, boxGeometry, material);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !collider.IsTrigger);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, collider.IsTrigger);
		shape->setLocalPose(ToPhysicsTransform(glm::translate(glm::mat4(1.0F), collider.Offset)));
	}

	void PhysicsWrappers::AddSphereCollider(physx::PxRigidActor& actor, const physx::PxMaterial& material, const SphereColliderComponent& collider, const glm::vec3& size)
	{
		float colliderRadius = collider.Radius;

		if (size.x != 0.0f)
		{
			colliderRadius *= (size.x / 2.0f);
		}

		physx::PxSphereGeometry sphereGeometry = physx::PxSphereGeometry(colliderRadius);
		physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(actor, sphereGeometry, material);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !collider.IsTrigger);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, collider.IsTrigger);
	}

	void PhysicsWrappers::AddCapsuleCollider(physx::PxRigidActor& actor, const physx::PxMaterial& material, const CapsuleColliderComponent& collider, const glm::vec3& size)
	{
		float colliderRadius = collider.Radius;
		float colliderHeight = collider.Height;

		if (size.x != 0.0f)
		{
			colliderRadius *= size.x;
		}

		if (size.y != 0.0f)
		{
			colliderHeight *= size.y;
		}

		physx::PxCapsuleGeometry capsuleGeometry = physx::PxCapsuleGeometry(colliderRadius, colliderHeight / 2.0f);
		physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(actor, capsuleGeometry, material);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !collider.IsTrigger);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, collider.IsTrigger);
		shape->setLocalPose(physx::PxTransform(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0, 0, 1))));
	}

	void PhysicsWrappers::AddMeshCollider(physx::PxRigidActor& actor, const physx::PxMaterial& material, MeshColliderComponent& collider, const glm::vec3& size)
	{
		physx::PxConvexMeshGeometry convexGeometry = physx::PxConvexMeshGeometry(CreateConvexMesh(collider));
		convexGeometry.meshFlags = physx::PxConvexMeshGeometryFlag::eTIGHT_BOUNDS;
		physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(actor, convexGeometry, material);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !collider.IsTrigger);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, collider.IsTrigger);
	}

	physx::PxConvexMesh* PhysicsWrappers::CreateConvexMesh(MeshColliderComponent & collider)
	{
		std::vector<Vertex> vertices = collider.CollisionMesh->GetStaticVertices();

		physx::PxConvexMeshDesc convexDesc;
		convexDesc.points.count = vertices.size();
		convexDesc.points.stride = sizeof(Vertex);
		convexDesc.points.data = vertices.data();
		convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

		physx::PxTolerancesScale sc;
		physx::PxCookingParams params(sc);

		physx::PxDefaultMemoryOutputStream buf;
		physx::PxConvexMeshCookingResult::Enum result;
		if (!PxCookConvexMesh(params, convexDesc, buf, &result))
		{
			NR_CORE_ASSERT(false);
		}

		physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
		physx::PxConvexMesh* mesh = sPhysics->createConvexMesh(input);

		if (!collider.ProcessedMesh)
		{
			const uint32_t nbPolygons = mesh->getNbPolygons();
			const physx::PxVec3* convexVertices = mesh->getVertices();
			const physx::PxU8* convexIndices = mesh->getIndexBuffer();

			uint32_t nbVertices = 0;
			uint32_t nbFaces = 0;

			for (uint32_t i = 0; i < nbPolygons; ++i)
			{
				physx::PxHullPolygon polygon;
				mesh->getPolygonData(i, polygon);
				nbVertices += polygon.mNbVerts;
				nbFaces += (polygon.mNbVerts - 2) * 3;
			}

			std::vector<Vertex> collisionVertices;
			std::vector<Index> collisionIndices;

			collisionVertices.resize(nbVertices);
			collisionIndices.resize(nbFaces / 3);

			uint32_t vertCounter = 0;
			uint32_t indexCounter = 0;
			for (uint32_t i = 0; i < nbPolygons; ++i)
			{
				physx::PxHullPolygon polygon;
				mesh->getPolygonData(i, polygon);

				uint32_t vI0 = vertCounter;
				for (uint32_t vI = 0; vI < polygon.mNbVerts; ++vI)
				{
					collisionVertices[vertCounter].Position = glm::rotate(FromPhysicsVector(convexVertices[convexIndices[polygon.mIndexBase + vI]]), glm::radians(90.0F), { 1, 0, 0 });
					++vertCounter;
				}

				for (uint32_t vI = 1; vI < uint32_t(polygon.mNbVerts) - 1; ++vI)
				{
					collisionIndices[indexCounter].V1 = uint32_t(vI0);
					collisionIndices[indexCounter].V2 = uint32_t(vI0 + vI + 1);
					collisionIndices[indexCounter].V3 = uint32_t(vI0 + vI);
					++indexCounter;
				}
			}

			collider.ProcessedMesh = Ref<Mesh>::Create(collisionVertices, collisionIndices);
		}

		return mesh;
	}

	physx::PxMaterial* PhysicsWrappers::CreateMaterial(const PhysicsMaterialComponent& material)
	{
		return sPhysics->createMaterial(material.StaticFriction, material.DynamicFriction, material.Bounciness);
	}

	bool PhysicsWrappers::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RaycastHit* hit)
	{
		physx::PxScene* scene = static_cast<physx::PxScene*>(PhysicsManager::GetPhysicsScene());
		physx::PxRaycastBuffer hitInfo;
		bool result = scene->raycast(ToPhysicsVector(origin), ToPhysicsVector(glm::normalize(direction)), maxDistance, hitInfo);

		if (result)
		{
			Entity& entity = *(Entity*)hitInfo.block.actor->userData;

			// NOTE: This should never be the case...
			if (!entity)
			{
				NR_CORE_ASSERT("Physics body with not Entity?");
			}

			hit->EntityID = entity.GetID();
			hit->Position = FromPhysicsVector(hitInfo.block.position);
			hit->Normal = FromPhysicsVector(hitInfo.block.normal);
			hit->Distance = hitInfo.block.distance;
		}

		return result;
	}

	void PhysicsWrappers::Initialize()
	{
		NR_CORE_ASSERT(!sFoundation, "PhysicsWrappers::Initializer shouldn't be called more than once!");

		sFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, sAllocator, sErrorCallback);
		NR_CORE_ASSERT(sFoundation, "PxCreateFoundation Failed!");

#if PHYSX_DEBUGGER
		sVisualDebugger = PxCreatePvd(*sFoundation);
		ConnectVisualDebugger();
#endif

		sPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *sFoundation, physx::PxTolerancesScale(), true, sVisualDebugger);
		NR_CORE_ASSERT(sPhysics, "PxCreatePhysics Failed!");
	}

	void PhysicsWrappers::Shutdown()
	{
		sPhysics->release();
		sFoundation->release();
	}

	void PhysicsWrappers::ConnectVisualDebugger()
	{
#if PHYSX_DEBUGGER
		if (sVisualDebugger->isConnected(false))
		{
			sVisualDebugger->disconnect();
		}

		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
		sVisualDebugger->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
#endif
	}

	void PhysicsWrappers::DisconnectVisualDebugger()
	{
#if PHYSX_DEBUGGER
		if (sVisualDebugger->isConnected(false))
			sVisualDebugger->disconnect();
#endif
	}

}