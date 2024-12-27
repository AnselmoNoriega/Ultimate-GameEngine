#include "nrpch.h"
#include "Project.h"

#include "NotRed/Asset/AssetManager.h"

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
		}
	}

	void Project::Deserialized()
	{
	}
}