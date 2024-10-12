#include "nrpch.h"
#include "AssetSerializer.h"

#include "NotRed/Util/StringUtils.h"
#include "NotRed/Util/FileSystem.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/Renderer.h"

#include "yaml-cpp/yaml.h"

namespace NR
{
	bool TextureSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Texture2D::Create(metadata.FilePath);
		asset->Handle = metadata.Handle;

		bool result = asset.As<Texture2D>()->Loaded();
		if (!result)
		{
			asset->ModifyFlags(AssetFlag::Invalid, true);
		}
		return result;
	}

	bool MeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Ref<Mesh>::Create(metadata.FilePath);
		asset->Handle = metadata.Handle;
		return (asset.As<Mesh>())->GetVertices().size() > 0;
	}

	bool EnvironmentSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		auto [radiance, irradiance] = SceneRenderer::CreateEnvironmentMap(metadata.FilePath);

		if (!radiance || !irradiance)
		{
			return false;
		}

		asset = Ref<Environment>::Create(radiance, irradiance);
		asset->Handle = metadata.Handle;
		return true;
	}

	void PhysicsMaterialSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<PhysicsMaterial> material = asset.As<PhysicsMaterial>();

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "StaticFriction" << material->StaticFriction;
		out << YAML::Key << "DynamicFriction" << material->DynamicFriction;
		out << YAML::Key << "Bounciness" << material->Bounciness;
		out << YAML::EndMap;

		std::ofstream fout(metadata.FilePath);
		fout << out.c_str();
	}

	bool PhysicsMaterialSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(metadata.FilePath);
		if (!stream.is_open())
		{
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());

		float staticFriction = data["StaticFriction"].as<float>();
		float dynamicFriction = data["DynamicFriction"].as<float>();
		float bounciness = data["Bounciness"].as<float>();

		asset = Ref<PhysicsMaterial>::Create(staticFriction, dynamicFriction, bounciness);
		asset->Handle = metadata.Handle;
		return true;
	}
}