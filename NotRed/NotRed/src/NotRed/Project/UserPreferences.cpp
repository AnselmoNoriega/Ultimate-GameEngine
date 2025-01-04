#include "nrpch.h"
#include "UserPreferences.h"

#include <fstream>
#include <sstream>

#include "yaml-cpp/yaml.h"

namespace NR
{
	UserPreferencesSerializer::UserPreferencesSerializer(const Ref<UserPreferences>& preferences)
		: mPreferences(preferences)
	{
	}

	UserPreferencesSerializer::~UserPreferencesSerializer()
	{
	}

	void UserPreferencesSerializer::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "UserPrefs" << YAML::Value;
		{
			out << YAML::BeginMap;
			out << YAML::Key << "ShowWelcomeScreen" << YAML::Value << mPreferences->ShowWelcomeScreen;
			if (!mPreferences->StartupProject.empty())
			{
				out << YAML::Key << "StartupProject" << YAML::Value << mPreferences->StartupProject;
			}

			{
				out << YAML::Key << "RecentProjects";
				out << YAML::Value << YAML::BeginSeq;
				for (const auto& [lastOpened, projectConfig] : mPreferences->RecentProjects)
				{
					out << YAML::BeginMap;
					out << YAML::Key << "Name" << YAML::Value << projectConfig.Name;
					out << YAML::Key << "ProjectPath" << YAML::Value << projectConfig.FilePath;
					out << YAML::Key << "LastOpened" << YAML::Value << projectConfig.LastOpened;
					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
		mPreferences->FilePath = filepath.string();
	}

	void UserPreferencesSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		NR_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();
		YAML::Node data = YAML::Load(strStream.str());

		if (!data["UserPrefs"])
		{
			return;
		}

		YAML::Node rootNode = data["UserPrefs"];
		mPreferences->ShowWelcomeScreen = rootNode["ShowWelcomeScreen"].as<bool>();
		mPreferences->StartupProject = rootNode["StartupProject"] ? rootNode["StartupProject"].as<std::string>() : "";

		for (auto recentProject : rootNode["RecentProjects"])
		{
			RecentProject entry;
			entry.Name = recentProject["Name"].as<std::string>();
			entry.FilePath = recentProject["ProjectPath"].as<std::string>();
			entry.LastOpened = recentProject["LastOpened"] ? recentProject["LastOpened"].as<time_t>() : time(NULL);
			mPreferences->RecentProjects[entry.LastOpened] = entry;
		}
		stream.close();
		mPreferences->FilePath = filepath.string();
	}
}