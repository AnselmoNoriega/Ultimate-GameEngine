#include "nrpch.h"
#include "ShaderCache.h"

#include "NotRed/Core/Hash.h"
#include "yaml-cpp/yaml.h"

namespace NR
{
	static const char* sShaderRegistryPath = "Resources/Cache/Shader/ShaderRegistry.cache";

	bool ShaderCache::HasChanged(const std::filesystem::path& shader, const std::string& source)
	{
		std::map<std::string, uint32_t> shaderCache;
		Deserialize(shaderCache);
		uint32_t hash = Hash::GenerateFNVHash(source.c_str());
		if (shaderCache.find(shader.string()) == shaderCache.end() || shaderCache.at(shader.string()) != hash)
		{
			shaderCache[shader.string()] = hash;
			Serialize(shaderCache);
			return true;
		}
		return false;
	}

	void ShaderCache::Serialize(const std::map<std::string, uint32_t>& shaderCache)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "ShaderRegistry" << YAML::BeginMap;
		for (auto& [filepath, hash] : shaderCache)
		{
			out << YAML::Key << filepath << YAML::Value << hash;
		}
		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(sShaderRegistryPath);
		fout << out.c_str();
	}
	void ShaderCache::Deserialize(std::map<std::string, uint32_t>& shaderCache)
	{
		// Read registry
		std::ifstream stream(sShaderRegistryPath);
		if (!stream.good())
		{
			return;
		}
		std::stringstream strStream;
		strStream << stream.rdbuf();
		YAML::Node data = YAML::Load(strStream.str());
		auto handles = data["ShaderRegistry"];
		if (!handles)
		{
			NR_CORE_ERROR("[ShaderCache] Shader Registry is invalid.");
			return;
		}
		for (auto entry : handles)
		{
			std::string path = entry.first.as<std::string>();
			uint32_t hash = entry.second.as<uint32_t>();
			shaderCache[path] = hash;
		}
	}
}