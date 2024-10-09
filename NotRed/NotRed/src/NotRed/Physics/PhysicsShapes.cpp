#include "nrpch.h"
#include "PhysicsShapes.h"

#include <filesystem>

#include <glm/gtc/type_ptr.hpp>

#include "PhysicsInternal.h"
#include "PhysicsActor.h"

#include "NotRed/Math/Math.h"
#include "NotRed/Util/FileSystem.h"

namespace NR
{
	namespace Utils
	{
		static const char* GetCacheDirectory()
		{
			return "Assets/Cache/Colliders/";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
			{
				std::filesystem::create_directories(cacheDirectory);
			}
		}
	}

	void ColliderShape::SetMaterial(const Ref<PhysicsMaterial>& material)
	{
		Ref<PhysicsMaterial> mat = material;
		if (!mat)
		{
			mat = Ref<PhysicsMaterial>::Create(0.6f, 0.6f, 0.0f);
		}

		if (mMaterial != nullptr)
		{
			mMaterial->release();
		}

		mMaterial = PhysicsInternal::GetPhysicsSDK().createMaterial(mat->StaticFriction, mat->DynamicFriction, mat->Bounciness);
	}

	BoxColliderShape::BoxColliderShape(BoxColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset)
		: ColliderShape(ColliderType::Box), mComponent(component)
	{
		SetMaterial(mComponent.Material);

		glm::vec3 colliderSize = entity.Transform().Scale * mComponent.Size;
		physx::PxBoxGeometry geometry = physx::PxBoxGeometry(colliderSize.x / 2.0f, colliderSize.y / 2.0f, colliderSize.z / 2.0f);
		mShape = physx::PxRigidActorExt::createExclusiveShape(actor.GetPhysicsActor(), geometry, *mMaterial);
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mComponent.IsTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mComponent.IsTrigger);
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset + mComponent.Offset, glm::vec3(0.0f)));
	}

	void BoxColliderShape::SetOffset(const glm::vec3& offset)
	{
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset, glm::vec3(0.0f)));
		mComponent.Offset = offset;
	}

	void BoxColliderShape::SetTrigger(bool isTrigger)
	{
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
		mComponent.IsTrigger = isTrigger;
	}

	void BoxColliderShape::DetachFromActor(physx::PxRigidActor* actor)
	{
		actor->detachShape(*mShape);
	}

	void BoxColliderShape::SetFilterData(const physx::PxFilterData& filterData)
	{
		mShape->setSimulationFilterData(filterData);
	}

	SphereColliderShape::SphereColliderShape(SphereColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset)
		: ColliderShape(ColliderType::Sphere), mComponent(component)
	{
		SetMaterial(mComponent.Material);

		auto& actorScale = entity.Transform().Scale;
		float largestComponent = glm::max(actorScale.x, glm::max(actorScale.y, actorScale.z));

		physx::PxSphereGeometry geometry = physx::PxSphereGeometry(largestComponent * mComponent.Radius);
		mShape = physx::PxRigidActorExt::createExclusiveShape(actor.GetPhysicsActor(), geometry, *mMaterial);
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mComponent.IsTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mComponent.IsTrigger);
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset + mComponent.Offset, glm::vec3(0.0f)));
	}

	void SphereColliderShape::SetOffset(const glm::vec3& offset)
	{
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset, glm::vec3(0.0f)));
		mComponent.Offset = offset;
	}

	void SphereColliderShape::SetTrigger(bool isTrigger)
	{
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
		mComponent.IsTrigger = isTrigger;
	}

	void SphereColliderShape::DetachFromActor(physx::PxRigidActor* actor)
	{
		actor->detachShape(*mShape);
	}

	void SphereColliderShape::SetFilterData(const physx::PxFilterData& filterData)
	{
		mShape->setSimulationFilterData(filterData);
	}

	CapsuleColliderShape::CapsuleColliderShape(CapsuleColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset)
		: ColliderShape(ColliderType::Capsule), mComponent(component)
	{
		SetMaterial(mComponent.Material);

		auto& actorScale = entity.Transform().Scale;
		float radiusScale = glm::max(actorScale.x, actorScale.z);

		physx::PxCapsuleGeometry geometry = physx::PxCapsuleGeometry(mComponent.Radius * radiusScale, (mComponent.Height / 2.0f) * actorScale.y);
		mShape = physx::PxRigidActorExt::createExclusiveShape(actor.GetPhysicsActor(), geometry, *mMaterial);
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mComponent.IsTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mComponent.IsTrigger);
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset + mComponent.Offset, glm::vec3(0.0f, 0.0f, physx::PxHalfPi)));
	}

	void CapsuleColliderShape::SetOffset(const glm::vec3& offset)
	{
		mShape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset, glm::vec3(0.0f)));
		mComponent.Offset = offset;
	}

	void CapsuleColliderShape::SetTrigger(bool isTrigger)
	{
		mShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
		mShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
		mComponent.IsTrigger = isTrigger;
	}

	void CapsuleColliderShape::DetachFromActor(physx::PxRigidActor* actor)
	{
		actor->detachShape(*mShape);
	}

	void CapsuleColliderShape::SetFilterData(const physx::PxFilterData& filterData)
	{
		mShape->setSimulationFilterData(filterData);
	}

	ConvexMeshShape::ConvexMeshShape(MeshColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset)
		: ColliderShape(ColliderType::ConvexMesh), mComponent(component)
	{
		NR_CORE_ASSERT(component.IsConvex);

		SetMaterial(mComponent.Material);

		std::vector<MeshColliderData> cookedData;
		cookedData.reserve(component.CollisionMesh->GetSubmeshes().size());

		Utils::CreateCacheDirectoryIfNeeded();
		std::string filepath = Utils::GetCacheDirectory() + std::filesystem::path(component.CollisionMesh->FilePath).filename().string() + "_convex.pxm";
		if (!FileSystem::Exists(filepath))
		{
			CookingFactory::CookConvexMesh(component.CollisionMesh, cookedData);
			SerializeData(filepath, cookedData);
		}
		else
		{
			DeserializeCached(filepath, cookedData);
		}

		NR_CORE_ASSERT(cookedData.size() > 0);

		for (auto& colliderData : cookedData)
		{
			glm::vec3 submeshTranslation, submeshRotation, submeshScale;
			Math::DecomposeTransform(colliderData.Transform, submeshTranslation, submeshRotation, submeshScale);

			physx::PxDefaultMemoryInputData input(colliderData.Data, colliderData.Size);
			physx::PxConvexMesh* convexMesh = PhysicsInternal::GetPhysicsSDK().createConvexMesh(input);
			physx::PxConvexMeshGeometry convexGeometry = physx::PxConvexMeshGeometry(convexMesh, physx::PxMeshScale(PhysicsUtils::ToPhysicsVector(submeshScale * entity.Transform().Scale)));
			convexGeometry.meshFlags = physx::PxConvexMeshGeometryFlag::eTIGHT_BOUNDS;

			physx::PxShape* shape = PhysicsInternal::GetPhysicsSDK().createShape(convexGeometry, *mMaterial, true);
			shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mComponent.IsTrigger);
			shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mComponent.IsTrigger);
			shape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset + submeshTranslation, submeshRotation));

			actor.GetPhysicsActor().attachShape(*shape);

			mShapes.push_back(shape);

			shape->release();
			convexMesh->release();

			delete[] colliderData.Data;
		}

		cookedData.clear();

		CreateDebugMesh();
	}

	void ConvexMeshShape::SetTrigger(bool isTrigger)
	{
		for (auto shape : mShapes)
		{
			shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
			shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
		}

		mComponent.IsTrigger = isTrigger;
	}

	void ConvexMeshShape::DetachFromActor(physx::PxRigidActor* actor)
	{
		for (auto shape : mShapes)
		{
			actor->detachShape(*shape);
		}

		mShapes.clear();
	}

	void ConvexMeshShape::SetFilterData(const physx::PxFilterData& filterData)
	{
		for (auto shape : mShapes)
		{
			shape->setSimulationFilterData(filterData);
		}
	}

	void ConvexMeshShape::SerializeData(const std::string& filepath, const std::vector<MeshColliderData>& data)
	{
		uint32_t bufferSize = 0;
		uint32_t offset = 0;

		for (auto& colliderData : data)
		{
			bufferSize += sizeof(uint32_t);
			bufferSize += colliderData.Size;
			bufferSize += sizeof(glm::mat4);
		}

		Buffer colliderBuffer;
		colliderBuffer.Allocate(bufferSize);

		for (auto& colliderData : data)
		{
			colliderBuffer.Write((void*)&colliderData.Size, sizeof(uint32_t), offset);
			offset += sizeof(uint32_t);
			colliderBuffer.Write(colliderData.Data, colliderData.Size, offset);
			offset += colliderData.Size;
		}

		FileSystem::WriteBytes(filepath, colliderBuffer);
		colliderBuffer.Release();
	}

	void ConvexMeshShape::DeserializeCached(const std::string& filepath, std::vector<MeshColliderData>& outData)
	{
		Buffer colliderBuffer = FileSystem::ReadBytes(filepath);
		uint32_t offset = 0;

		for (const auto& submesh : mComponent.CollisionMesh->GetSubmeshes())
		{
			MeshColliderData& data = outData.emplace_back();

			data.Size = colliderBuffer.Read<uint32_t>(offset);
			offset += sizeof(uint32_t);
			data.Data = colliderBuffer.ReadBytes(data.Size, offset);
			offset += data.Size;
			data.Transform = submesh.Transform;
		}

		colliderBuffer.Release();
	}

	void ConvexMeshShape::CreateDebugMesh()
	{
		if (mComponent.ProcessedMeshes.size() > 0)
		{
			return;
		}

		for (auto shape : mShapes)
		{
			physx::PxGeometryHolder geometryHolder = shape->getGeometry();
			const physx::PxConvexMeshGeometry& convexGeometry = geometryHolder.convexMesh();
			physx::PxConvexMesh* mesh = convexGeometry.convexMesh;

			// Based On: https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/ThirdParty/Physics3/NvCloth/samples/SampleBase/renderer/ConvexRenderMesh.cpp
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

				for (uint32_t vI = 0; vI < polygon.mNbVerts; ++vI)
				{
					Vertex v;
					v.Position = PhysicsUtils::FromPhysicsVector(convexVertices[convexIndices[polygon.mIndexBase + vI]]);
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

				mComponent.ProcessedMeshes.push_back(Ref<Mesh>::Create(collisionVertices, collisionIndices, PhysicsUtils::FromPhysicsTransform(shape->getLocalPose())));
			}
		}
	}
	
	TriangleMeshShape::TriangleMeshShape(MeshColliderComponent& component, const PhysicsActor& actor, Entity entity, const glm::vec3& offset)
		: ColliderShape(ColliderType::TriangleMesh), mComponent(component)
	{
		NR_CORE_ASSERT(!component.IsConvex);

		SetMaterial(mComponent.Material);
		std::vector<MeshColliderData> cookedData;
		cookedData.reserve(component.CollisionMesh->GetSubmeshes().size());

		Utils::CreateCacheDirectoryIfNeeded();

		std::string filepath = Utils::GetCacheDirectory() + std::filesystem::path(component.CollisionMesh->FilePath).filename().string() + "_tri.pxm";
		if (!FileSystem::Exists(filepath))
		{
			CookingFactory::CookTriangleMesh(component.CollisionMesh, cookedData);
			SerializeData(filepath, cookedData);
		}
		else
		{
			DeserializeCached(filepath, cookedData);
		}

		NR_CORE_ASSERT(cookedData.size() > 0);

		for (auto& colliderData : cookedData)
		{
			glm::vec3 submeshTranslation, submeshRotation, submeshScale;
			Math::DecomposeTransform(colliderData.Transform, submeshTranslation, submeshRotation, submeshScale);

			physx::PxDefaultMemoryInputData input(colliderData.Data, colliderData.Size);
			physx::PxTriangleMesh* trimesh = PhysicsInternal::GetPhysicsSDK().createTriangleMesh(input);
			physx::PxTriangleMeshGeometry triangleGeometry = physx::PxTriangleMeshGeometry(trimesh, physx::PxMeshScale(PhysicsUtils::ToPhysicsVector(submeshScale * entity.Transform().Scale)));
			physx::PxShape* shape = PhysicsInternal::GetPhysicsSDK().createShape(triangleGeometry, *mMaterial, true);

			shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !mComponent.IsTrigger);
			shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, mComponent.IsTrigger);
			shape->setLocalPose(PhysicsUtils::ToPhysicsTransform(offset + submeshTranslation, submeshRotation));

			actor.GetPhysicsActor().attachShape(*shape);
			mShapes.push_back(shape);

			shape->release();
			trimesh->release();
			delete[] colliderData.Data;
		}

		cookedData.clear();
		CreateDebugMesh();
	}

	void TriangleMeshShape::SetTrigger(bool isTrigger)
	{
		for (auto shape : mShapes)
		{
			shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
			shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
		}
		mComponent.IsTrigger = isTrigger;
	}

	void TriangleMeshShape::SetFilterData(const physx::PxFilterData& filterData)
	{
		for (auto shape : mShapes)
		{
			shape->setSimulationFilterData(filterData);
		}
	}

	void TriangleMeshShape::DetachFromActor(physx::PxRigidActor* actor)
	{
		for (auto shape : mShapes)
		{
			actor->detachShape(*shape);
		}
		mShapes.clear();
	}

	void TriangleMeshShape::SerializeData(const std::string& filepath, const std::vector<MeshColliderData>& data)
	{
		uint32_t bufferSize = 0;
		uint32_t offset = 0;

		for (auto& colliderData : data)
		{
			bufferSize += sizeof(uint32_t);
			bufferSize += colliderData.Size;
			bufferSize += sizeof(glm::mat4);
		}

		Buffer colliderBuffer;
		colliderBuffer.Allocate(bufferSize);

		for (auto& colliderData : data)
		{
			colliderBuffer.Write((void*)&colliderData.Size, sizeof(uint32_t), offset);
			offset += sizeof(uint32_t);
			colliderBuffer.Write(colliderData.Data, colliderData.Size, offset);
			offset += colliderData.Size;
		}

		FileSystem::WriteBytes(filepath, colliderBuffer);
		colliderBuffer.Release();
	}

	void TriangleMeshShape::DeserializeCached(const std::string& filepath, std::vector<MeshColliderData>& outData)
	{
		Buffer colliderBuffer = FileSystem::ReadBytes(filepath);
		uint32_t offset = 0;

		for (const auto& submesh : mComponent.CollisionMesh->GetSubmeshes())
		{
			MeshColliderData& data = outData.emplace_back();
			data.Size = colliderBuffer.Read<uint32_t>(offset);
			offset += sizeof(uint32_t);
			data.Data = colliderBuffer.ReadBytes(data.Size, offset);

			offset += data.Size;

			data.Transform = submesh.Transform;
		}

		colliderBuffer.Release();
	}

	void TriangleMeshShape::CreateDebugMesh()
	{
		if (mComponent.ProcessedMeshes.size() > 0)
		{
			return;
		}

		for (auto shape : mShapes)
		{
			physx::PxGeometryHolder geometryHolder = shape->getGeometry();
			const physx::PxTriangleMeshGeometry& triangleGeometry = geometryHolder.triangleMesh();
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
				v1.Position = PhysicsUtils::FromPhysicsVector(triangleVertices[v]);
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

			glm::mat4 scale = glm::scale(glm::mat4(1.0f), PhysicsUtils::FromPhysicsVector(triangleGeometry.scale.scale));
			glm::mat4 transform = PhysicsUtils::FromPhysicsTransform(shape->getLocalPose()) * scale;
			mComponent.ProcessedMeshes.push_back(Ref<Mesh>::Create(vertices, indices, transform));
		}
	}
}