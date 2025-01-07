#include "nrpch.h"
#include "Project.h"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Audio/AudioEvents/AudioCommandRegistry.h"

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
		}

		sActiveProject = project;
		if (sActiveProject)
		{
			AssetManager::Init();
			AudioCommandRegistry::Init();
		}
	}

	void Project::Deserialized()
	{
	}
}