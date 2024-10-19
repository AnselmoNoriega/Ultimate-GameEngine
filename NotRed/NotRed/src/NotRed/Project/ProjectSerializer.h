#pragma once

#include <string>

#include "Project.h"

namespace NR
{
	class ProjectSerializer
	{
	public:
		ProjectSerializer(Ref<Project> project);

		void Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

	private:
		Ref<Project> mProject;
	};
}