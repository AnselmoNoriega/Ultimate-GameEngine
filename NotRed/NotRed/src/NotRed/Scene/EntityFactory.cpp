#include "nrpch.h"
#include "EntityFactory.h"

#include "Scene.h"

namespace NR
{
	void EntityFactory::Fracture(Entity originalEntity)
	{
		/*auto& meshComponent = originalEntity.GetComponent<MeshComponent>();
		Ref<Mesh> originalMesh = meshComponent.MeshObj;
		const auto& submeshes = originalMesh->GetSubmeshes();
		Ref<Scene> scene = Scene::GetScene(originalEntity.GetSceneID());

		for (const auto& submesh : submeshes)
		{
			Entity newEntity = scene->CreateEntity(submesh.MeshName);
			Ref<Mesh> newMesh = Ref<Mesh>::Create(originalMesh, submesh);
			auto& newMeshComponent = newEntity.AddComponent<MeshComponent>();

			if (originalEntity.HasComponent<RigidBodyComponent>())
			{
				auto& rigidbody = newEntity.AddComponent<RigidBodyComponent>(originalEntity.GetComponent<RigidBodyComponent>());
			}
		}*/
	}
}