#include "nrpch.h"
#include "Project.h"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Audio/AudioEvents/AudioCommandRegistry.h"

#include "NotRed/Physics/3D/PhysicsSystem.h"

namespace NR
{
	Project::Project()
	{
	}

	Project::~Project()
	{
	}

	void Project::SetActive(Ref<Project> project)
	{
		if (sActiveProject)
		{
			AssetManager::Shutdown();
			PhysicsSystem::Shutdown();
		}

		sActiveProject = project;
		if (sActiveProject)
		{
			AssetManager::Init();
			PhysicsSystem::Init();
			AudioCommandRegistry::Init();
		}
	}

	void Project::Deserialized()
	{
	}
}