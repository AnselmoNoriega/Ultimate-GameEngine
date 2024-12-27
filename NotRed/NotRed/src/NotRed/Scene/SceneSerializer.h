#pragma once

#include "Scene.h"

namespace YAML 
{
	class Emitter;
	class Node;
}

namespace NR
{
	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);

		void Serialize(const std::string& filepath);
		void SerializeRuntime(const std::string& filepath);

		bool Deserialize(const std::string& filepath);
		bool DeserializeRuntime(const std::string& filepath);

	public:
		inline static std::string_view FileFilter = "NotRed Scene (*.nrscene)\0*.nrscene\0";
		inline static std::string_view DefaultExtension = ".nrscene";

	public:
		static void SerializeEntity(YAML::Emitter& out, Entity entity);
		static void DeserializeEntities(YAML::Node& entitiesNode, Ref<Scene> scene);

	private:
		Ref<Scene> mScene;
	};
}