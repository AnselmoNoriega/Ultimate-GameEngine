#include "nrpch.h"
#include "SceneSerializer.h"

#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>

#include "Components.h"

namespace YAML
{
    template<>
    struct convert<glm::vec3>
    {
        static Node encode(const glm::vec3& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            return node;
        }

        static bool decode(const Node& node, glm::vec3& rhs)
        {
            if (!node.IsSequence() || node.size() != 3)
            {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::vec4>
    {
        static Node encode(const glm::vec4& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            return node;
        }

        static bool decode(const Node& node, glm::vec4& rhs)
        {
            if (!node.IsSequence() || node.size() != 4)
            {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
    };
}

namespace NR
{
    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
        return out;
    }

    SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
        :mScene(scene)
    {
    }

    static void SerializeEntity(YAML::Emitter& out, Entity entity)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "Entity" << YAML::Value << "123456";

        out << YAML::Key << "TagComponent";
        out << YAML::BeginMap;

        out << YAML::Key << "Tag" << YAML::Value << entity.GetComponent<TagComponent>().Tag;

        out << YAML::EndMap;

        out << YAML::Key << "TransformComponent";
        out << YAML::BeginMap;

        auto& tc = entity.GetComponent<TransformComponent>();
        out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
        out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
        out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

        out << YAML::EndMap;


        if (entity.HasComponent<CameraComponent>())
        {
            out << YAML::Key << "CameraComponent";
            out << YAML::BeginMap;

            auto& cc = entity.GetComponent<CameraComponent>();
            auto& camera = cc.Camera;

            out << YAML::Key << "CameraInfo";
            out << YAML::BeginMap;
            out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
            out << YAML::Key << "FOV" << YAML::Value << camera.GetFOV();
            out << YAML::Key << "NearClip" << YAML::Value << camera.GetNearClip();
            out << YAML::Key << "FarClip" << YAML::Value << camera.GetFarClip();
            out << YAML::EndMap;

            out << YAML::Key << "Primary" << YAML::Value << cc.IsPrimary;
            out << YAML::Key << "FixedAspectRatio" << YAML::Value << cc.HasFixedAspectRatio;

            out << YAML::EndMap;
        }

        if (entity.HasComponent<SpriteRendererComponent>())
        {
            out << YAML::Key << "SpriteRendererComponent";
            out << YAML::BeginMap;

            out << YAML::Key << "Color" << YAML::Value << entity.GetComponent<SpriteRendererComponent>().Color;

            out << YAML::EndMap;
        }

        out << YAML::EndMap;
    }

    void SceneSerializer::Serialize(const std::string& filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << "Name";
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
        mScene->mRegistry.view<TagComponent>().each([&](auto entityID, auto rn)
            {
                Entity entity = { entityID, mScene.get() };
                if (!entity)
                {
                    return;
                }

                SerializeEntity(out, entity);
            });
        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    void SceneSerializer::SerializeRuntime(const std::string& filepath)
    {
    }

    bool SceneSerializer::Deserialize(const std::string& filepath)
    {
        std::ifstream stream(filepath);
        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        if (!data["Scene"])
        {
            return false;
        }

        std::string sceneName = data["Scene"].as<std::string>();
        NR_CORE_TRACE("Deserializing scene '{0}'", sceneName);

        auto entities = data["Entities"];
        if (entities)
        {
            for (auto entity : entities)
            {
                uint64_t id = entity["Entity"].as<uint64_t>();
                std::string name = entity["TagComponent"]["Tag"].as<std::string>();
                NR_CORE_TRACE("Entity ID = {0}, Name = {1}", id, name);

                Entity loadedEntity = mScene->CreateEntity(name);

                auto tcYaml = entity["TransformComponent"];
                auto& tc = loadedEntity.GetComponent<TransformComponent>();
                tc.Translation = tcYaml["Translation"].as<glm::vec3>();
                tc.Rotation = tcYaml["Rotation"].as<glm::vec3>();
                tc.Scale = tcYaml["Scale"].as<glm::vec3>();

                auto ccYaml = entity["CameraComponent"];
                if (ccYaml)
                {
                    auto& cc = loadedEntity.AddComponent<CameraComponent>();
                    
                    const auto& cameraProps = ccYaml["CameraInfo"];
                    cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraProps["ProjectionType"].as<int>());
                    cc.Camera.SetFOV(cameraProps["FOV"].as<float>());
                    cc.Camera.SetNearClip(cameraProps["NearClip"].as<float>());
                    cc.Camera.SetFarClip(cameraProps["FarClip"].as<float>());

                    cc.IsPrimary = ccYaml["Primary"].as<bool>();
                    cc.HasFixedAspectRatio = ccYaml["FixedAspectRatio"].as<bool>();
                }

                auto scYaml = entity["SpriteRendererComponent"];
                if (ccYaml)
                {
                    auto& sc = loadedEntity.AddComponent<SpriteRendererComponent>();
                    sc.Color = scYaml["Color"].as<glm::vec4>();
                }
            }
        }

        return true;
    }

    bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
    {
        return false;
    }
}