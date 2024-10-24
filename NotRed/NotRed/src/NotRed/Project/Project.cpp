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
		sActiveProject = project;
		AssetManager::Init();
	}

	void Project::Deserialized()
	{
	}
}