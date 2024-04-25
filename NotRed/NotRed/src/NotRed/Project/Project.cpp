#include "nrpch.h"
#include "Project.h"

#include "ProjectSerializer.h"

namespace NR
{
    Ref<Project> Project::New()
    {
        sCurrentProject = CreateRef<Project>();
        return sCurrentProject;
    }
    
    Ref<Project> Project::Load(const std::filesystem::path& path)
    {
        Ref<Project> project = CreateRef<Project>();

        ProjectSerializer serializer(project);
        if (serializer.Deserialize(path))
        {
            project->mProjectDirectory = path.parent_path();
            sCurrentProject = project;
            return sCurrentProject;
        }

        return nullptr;
    }

    bool Project::SaveAs(const std::filesystem::path& path)
    {
        ProjectSerializer serializer(sCurrentProject);
        if (serializer.Serialize(path))
        {
            sCurrentProject->mProjectDirectory = path.parent_path();
            return true;
        }

        return false;
    }
}
