#include "nrpch.h"
#include "SceneSerializer.h"

#include <iostream>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Entity.h"
#include "Components.h"
#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Renderer/MeshFactory.h"
#include "NotRed/Physics/PhysicsLayer.h"

#include "NotRed/Asset/AssetManager.h"

#include "NotRed/Audio/AudioComponent.h"
#include "NotRed/Audio/AudioEngine.h"

#include "yaml-cpp/yaml.h"

namespace YAML
{
	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
			{
				return false;
			}

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

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

	template<>
	struct convert<glm::quat>
	{
		static Node encode(const glm::quat& rhs)
		{
			Node node;
			node.push_back(rhs.w);
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::quat& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
			{
				return false;
			}

			rhs.w = node[0].as<float>();
			rhs.x = node[1].as<float>();
			rhs.y = node[2].as<float>();
			rhs.z = node[3].as<float>();
			return true;
		}
	};
}

namespace NR
{
	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

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

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::quat& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.w << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: mScene(scene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		UUID uuid = entity.GetComponent<IDComponent>().ID;
		out << YAML::BeginMap; // Entity

		out << YAML::Key << "Entity";
		out << YAML::Value << uuid;

		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& transform = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Position" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		{
			auto& relationshipComponent = entity.GetComponent<RelationshipComponent>();
			out << YAML::Key << "Parent" << YAML::Value << relationshipComponent.ParentHandle;

			out << YAML::Key << "Children";
			out << YAML::Value << YAML::BeginSeq;

			for (auto child : relationshipComponent.Children)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << child;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}

		if (entity.HasComponent<ScriptComponent>())
		{
			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap; // ScriptComponent

			auto& moduleName = entity.GetComponent<ScriptComponent>().ModuleName;
			out << YAML::Key << "ModuleName" << YAML::Value << moduleName;

			EntityInstanceData& data = ScriptEngine::GetEntityInstanceData(entity.GetSceneID(), uuid);
			const auto& moduleFieldMap = data.ModuleFieldMap;
			if (moduleFieldMap.find(moduleName) != moduleFieldMap.end())
			{
				const auto& fields = moduleFieldMap.at(moduleName);
				out << YAML::Key << "StoredFields" << YAML::Value;
				out << YAML::BeginSeq;
				for (const auto& [name, field] : fields)
				{
					out << YAML::BeginMap; // Field
					out << YAML::Key << "Name" << YAML::Value << name;
					out << YAML::Key << "Type" << YAML::Value << (uint32_t)field.Type;
					out << YAML::Key << "Data" << YAML::Value;

					switch (field.Type)
					{
					case FieldType::Int:
					{
						out << field.GetStoredValue<int>();
						break;
					}
					case FieldType::UnsignedInt:
					{
						out << field.GetStoredValue<uint32_t>();
						break;
					}
					case FieldType::Float:
					{
						out << field.GetStoredValue<float>();
						break;
					}
					case FieldType::Vec2:
					{
						out << field.GetStoredValue<glm::vec2>();
						break;
					}
					case FieldType::Vec3:
					{
						out << field.GetStoredValue<glm::vec3>();
						break;
					}
					case FieldType::Vec4:
					{
						out << field.GetStoredValue<glm::vec4>();
						break;
					}
					}
					out << YAML::EndMap; // Field
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap; // ScriptComponent
		}

		if (entity.HasComponent<MeshComponent>())
		{
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap; // MeshComponent

			auto mesh = entity.GetComponent<MeshComponent>().MeshObj;
			if (mesh)
			{
				auto meshAsset = mesh->GetMeshAsset();
				out << YAML::Key << "AssetID" << YAML::Value << meshAsset->Handle;
			}
			else
			{
				out << YAML::Key << "AssetID" << YAML::Value << 0;
			}

			out << YAML::EndMap; // MeshComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.CameraObj;
			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap;
			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<DirectionalLightComponent>())
		{
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::BeginMap; // DirectionalLightComponent

			auto& directionalLightComponent = entity.GetComponent<DirectionalLightComponent>();
			out << YAML::Key << "Radiance" << YAML::Value << directionalLightComponent.Radiance;
			out << YAML::Key << "CastShadows" << YAML::Value << directionalLightComponent.CastShadows;
			out << YAML::Key << "SoftShadows" << YAML::Value << directionalLightComponent.SoftShadows;
			out << YAML::Key << "LightSize" << YAML::Value << directionalLightComponent.LightSize;

			out << YAML::EndMap; // DirectionalLightComponent
		}

		if (entity.HasComponent<PointLightComponent>())
		{
			out << YAML::Key << "PointLightComponent";
			out << YAML::BeginMap; // PointLightComponent
			auto& directionalLightComponent = entity.GetComponent<PointLightComponent>();
			out << YAML::Key << "Radiance" << YAML::Value << directionalLightComponent.Radiance;
			out << YAML::Key << "Intensity" << YAML::Value << directionalLightComponent.Intensity;
			out << YAML::Key << "CastShadows" << YAML::Value << directionalLightComponent.CastsShadows;
			out << YAML::Key << "SoftShadows" << YAML::Value << directionalLightComponent.SoftShadows;
			out << YAML::Key << "MinRadius" << YAML::Value << directionalLightComponent.MinRadius;
			out << YAML::Key << "Radius" << YAML::Value << directionalLightComponent.Radius;
			out << YAML::Key << "LightSize" << YAML::Value << directionalLightComponent.LightSize;
			out << YAML::Key << "Falloff" << YAML::Value << directionalLightComponent.Falloff;
			out << YAML::EndMap; // PointLightComponent
		}

		if (entity.HasComponent<SkyLightComponent>())
		{
			out << YAML::Key << "SkyLightComponent";
			out << YAML::BeginMap; // SkyLightComponent

			auto& skyLightComponent = entity.GetComponent<SkyLightComponent>();
			out << YAML::Key << "EnvironmentMap" << YAML::Value << skyLightComponent.SceneEnvironment->Handle;
			out << YAML::Key << "Intensity" << YAML::Value << skyLightComponent.Intensity;
			out << YAML::Key << "Angle" << YAML::Value << skyLightComponent.Angle;

			out << YAML::EndMap; // SkyLightComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;
			if (spriteRendererComponent.Texture)
			{
				out << YAML::Key << "TextureAssetPath" << YAML::Value << "path/to/asset";
			}
			out << YAML::Key << "TilingFactor" << YAML::Value << spriteRendererComponent.TilingFactor;

			out << YAML::EndMap; // SpriteRendererComponent
		}

		if (entity.HasComponent<RigidBody2DComponent>())
		{
			out << YAML::Key << "RigidBody2DComponent";
			out << YAML::BeginMap; // RigidBody2DComponent

			auto& rigidbody2DComponent = entity.GetComponent<RigidBody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << (int)rigidbody2DComponent.BodyType;
			out << YAML::Key << "FixedRotation" << YAML::Value << rigidbody2DComponent.FixedRotation;

			out << YAML::EndMap; // RigidBody2DComponent
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap; // BoxCollider2DComponent

			auto& boxCollider2DComponent = entity.GetComponent<BoxCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << boxCollider2DComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << boxCollider2DComponent.Size;
			out << YAML::Key << "Density" << YAML::Value << boxCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << boxCollider2DComponent.Friction;

			out << YAML::EndMap; // BoxCollider2DComponent
		}

		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			out << YAML::Key << "CircleCollider2DComponent";
			out << YAML::BeginMap; // CircleCollider2DComponent

			auto& circleCollider2DComponent = entity.GetComponent<CircleCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << circleCollider2DComponent.Offset;
			out << YAML::Key << "Radius" << YAML::Value << circleCollider2DComponent.Radius;
			out << YAML::Key << "Density" << YAML::Value << circleCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << circleCollider2DComponent.Friction;

			out << YAML::EndMap; // CircleCollider2DComponent
		}

		if (entity.HasComponent<RigidBodyComponent>())
		{
			out << YAML::Key << "RigidBodyComponent";
			out << YAML::BeginMap; // RigidBodyComponent

			auto& rigidbodyComponent = entity.GetComponent<RigidBodyComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << (int)rigidbodyComponent.BodyType;
			out << YAML::Key << "Mass" << YAML::Value << rigidbodyComponent.Mass;
			out << YAML::Key << "LinearDrag" << YAML::Value << rigidbodyComponent.LinearDrag;
			out << YAML::Key << "AngularDrag" << YAML::Value << rigidbodyComponent.AngularDrag;
			out << YAML::Key << "DisableGravity" << YAML::Value << rigidbodyComponent.DisableGravity;
			out << YAML::Key << "IsKinematic" << YAML::Value << rigidbodyComponent.IsKinematic;
			out << YAML::Key << "Layer" << YAML::Value << rigidbodyComponent.Layer;

			out << YAML::Key << "Constraints";
			out << YAML::BeginMap; // Constraints

			out << YAML::Key << "LockPositionX" << YAML::Value << rigidbodyComponent.LockPositionX;
			out << YAML::Key << "LockPositionY" << YAML::Value << rigidbodyComponent.LockPositionY;
			out << YAML::Key << "LockPositionZ" << YAML::Value << rigidbodyComponent.LockPositionZ;

			out << YAML::Key << "LockRotationX" << YAML::Value << rigidbodyComponent.LockRotationX;
			out << YAML::Key << "LockRotationY" << YAML::Value << rigidbodyComponent.LockRotationY;
			out << YAML::Key << "LockRotationZ" << YAML::Value << rigidbodyComponent.LockRotationZ;

			out << YAML::EndMap;

			out << YAML::EndMap; // RigidBodyComponent
		}

		if (entity.HasComponent<BoxColliderComponent>())
		{
			out << YAML::Key << "BoxColliderComponent";
			out << YAML::BeginMap; // BoxColliderComponent

			auto& boxColliderComponent = entity.GetComponent<BoxColliderComponent>();
			out << YAML::Key << "Offset" << YAML::Value << boxColliderComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << boxColliderComponent.Size;
			out << YAML::Key << "IsTrigger" << YAML::Value << boxColliderComponent.IsTrigger;

			if (boxColliderComponent.Material)
			{
				out << YAML::Key << "Material" << YAML::Value << boxColliderComponent.Material->Handle;
			}
			else
			{
				out << YAML::Key << "Material" << YAML::Value << 0;
			}

			out << YAML::EndMap; // BoxColliderComponent
		}
		//TODO check the other shapes offset
		if (entity.HasComponent<SphereColliderComponent>())
		{
			out << YAML::Key << "SphereColliderComponent";
			out << YAML::BeginMap; // SphereColliderComponent

			auto& sphereColliderComponent = entity.GetComponent<SphereColliderComponent>();
			out << YAML::Key << "Radius" << YAML::Value << sphereColliderComponent.Radius;
			out << YAML::Key << "IsTrigger" << YAML::Value << sphereColliderComponent.IsTrigger;

			if (sphereColliderComponent.Material)
			{
				out << YAML::Key << "Material" << YAML::Value << sphereColliderComponent.Material->Handle;
			}
			else
			{
				out << YAML::Key << "Material" << YAML::Value << 0;
			}

			out << YAML::EndMap; // SphereColliderComponent
		}

		if (entity.HasComponent<CapsuleColliderComponent>())
		{
			out << YAML::Key << "CapsuleColliderComponent";
			out << YAML::BeginMap; // CapsuleColliderComponent

			auto& capsuleColliderComponent = entity.GetComponent<CapsuleColliderComponent>();
			out << YAML::Key << "Radius" << YAML::Value << capsuleColliderComponent.Radius;
			out << YAML::Key << "Height" << YAML::Value << capsuleColliderComponent.Height;
			out << YAML::Key << "IsTrigger" << YAML::Value << capsuleColliderComponent.IsTrigger;

			if (capsuleColliderComponent.Material)
			{
				out << YAML::Key << "Material" << YAML::Value << capsuleColliderComponent.Material->Handle;
			}
			else
			{
				out << YAML::Key << "Material" << YAML::Value << 0;
			}

			out << YAML::EndMap; // CapsuleColliderComponent
		}

		if (entity.HasComponent<MeshColliderComponent>())
		{
			out << YAML::Key << "MeshColliderComponent";
			out << YAML::BeginMap; // MeshColliderComponent

			auto& meshColliderComponent = entity.GetComponent<MeshColliderComponent>();
			if (meshColliderComponent.OverrideMesh)
			{
				out << YAML::Key << "AssetID" << YAML::Value << meshColliderComponent.CollisionMesh->Handle;
			}
			out << YAML::Key << "IsConvex" << YAML::Value << meshColliderComponent.IsConvex;
			out << YAML::Key << "IsTrigger" << YAML::Value << meshColliderComponent.IsTrigger;

			if (meshColliderComponent.Material)
			{
				out << YAML::Key << "Material" << YAML::Value << meshColliderComponent.Material->Handle;
			}
			else
			{
				out << YAML::Key << "Material" << YAML::Value << 0;
			}

			out << YAML::EndMap; // MeshColliderComponent
		}

		if (entity.HasComponent<Audio::AudioComponent>())
		{
			out << YAML::Key << "AudioComponent";
			out << YAML::BeginMap; // AudioComponent

			auto& audioComponent = entity.GetComponent<Audio::AudioComponent>();
			const auto& soundCofig = audioComponent.SoundConfig;

			if (soundCofig.FileAsset)
			{
				out << YAML::Key << "AssetID" << YAML::Value << soundCofig.FileAsset->Handle;
			}

			out << YAML::Key << "IsLooping" << YAML::Value << (bool)soundCofig.Looping;
			out << YAML::Key << "VolumeMultiplier" << YAML::Value << audioComponent.VolumeMultiplier;
			out << YAML::Key << "PitchMultiplier" << YAML::Value << audioComponent.PitchMultiplier;
			out << YAML::Key << "AutoDestroy" << YAML::Value << audioComponent.AutoDestroy;
			out << YAML::Key << "PlayOnAwake" << YAML::Value << audioComponent.PlayOnAwake;
			out << YAML::Key << "MasterReverbSend" << YAML::Value << soundCofig.MasterReverbSend;
			out << YAML::Key << "LPFilterValue" << YAML::Value << soundCofig.LPFilterValue;
			out << YAML::Key << "HPFilterValue" << YAML::Value << soundCofig.HPFilterValue;

			out << YAML::Key << "Spatialization";
			out << YAML::BeginMap; // Spatialization

			auto& spatialConfig = soundCofig.Spatialization;
			out << YAML::Key << "Enabled" << YAML::Value << soundCofig.SpatializationEnabled;
			out << YAML::Key << "AttenuationModel" << YAML::Value << (int)spatialConfig.AttenuationMod;
			out << YAML::Key << "MinGain" << YAML::Value << spatialConfig.MinGain;
			out << YAML::Key << "MaxGain" << YAML::Value << spatialConfig.MaxGain;
			out << YAML::Key << "MinDistance" << YAML::Value << spatialConfig.MinDistance;
			out << YAML::Key << "MaxDistance" << YAML::Value << spatialConfig.MaxDistance;
			out << YAML::Key << "ConeInnerAngle" << YAML::Value << spatialConfig.ConeInnerAngleInRadians;
			out << YAML::Key << "ConeOuterAngle" << YAML::Value << spatialConfig.ConeOuterAngleInRadians;
			out << YAML::Key << "ConeOuterGain" << YAML::Value << spatialConfig.ConeOuterGain;
			out << YAML::Key << "DopplerFactor" << YAML::Value << spatialConfig.DopplerFactor;
			out << YAML::Key << "Rollor" << YAML::Value << spatialConfig.Rolloff;
			out << YAML::Key << "AirAbsorptionEnabled" << YAML::Value << spatialConfig.AirAbsorptionEnabled;

			out << YAML::EndMap; // Spatialization
			out << YAML::EndMap; // AudioComponent
		}

		if (entity.HasComponent<AudioListenerComponent>())
		{
			out << YAML::Key << "AudioListenerComponent";
			out << YAML::BeginMap; // AudioComponent

			auto& audioListenerComponent = entity.GetComponent<AudioListenerComponent>();
			out << YAML::Key << "Active" << YAML::Value << audioListenerComponent.Active;
			out << YAML::Key << "ConeInnerAngle" << YAML::Value << audioListenerComponent.ConeInnerAngleInRadians;
			out << YAML::Key << "ConeOuterAngle" << YAML::Value << audioListenerComponent.ConeOuterAngleInRadians;
			out << YAML::Key << "ConeOuterGain" << YAML::Value << audioListenerComponent.ConeOuterGain;

			out << YAML::EndMap; // AudioComponent
		}

		out << YAML::EndMap; // Entity
	}

	static void SerializeEnvironment(YAML::Emitter& out, const Ref<Scene>& scene)
	{
		out << YAML::Key << "Environment";
		out << YAML::Value;
		out << YAML::BeginMap; // Environment
		out << YAML::Key << "AssetHandle" << YAML::Value << scene->GetEnvironment()->Handle;
		const auto& light = scene->GetLight();
		out << YAML::Key << "Light" << YAML::Value;
		out << YAML::BeginMap; // Light
		out << YAML::Key << "Direction" << YAML::Value << light.Direction;
		out << YAML::Key << "Radiance" << YAML::Value << light.Radiance;
		out << YAML::Key << "Multiplier" << YAML::Value << light.Multiplier;
		out << YAML::EndMap; // Light
		out << YAML::EndMap; // Environment
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene";
		out << YAML::Value << "Scene Name";

		if (mScene->GetEnvironment())
		{
			SerializeEnvironment(out, mScene);
		}

		out << YAML::Key << "Entities";
		out << YAML::Value << YAML::BeginSeq;
		auto view = mScene->mRegistry.view<IDComponent>();
		view.each([&](entt::entity entt, auto id)
			{
				Entity entity = { entt, mScene.Raw() };
				NR_CORE_ASSERT(entity);

				SerializeEntity(out, entity);
			});
		out << YAML::EndSeq;

		out << YAML::Key << "PhysicsLayers";
		out << YAML::Value << YAML::BeginSeq;
		for (const auto& layer : PhysicsLayerManager::GetLayers())
		{
			if (layer.ID == 0)
			{
				continue;
			}

			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << layer.Name;

			out << YAML::Key << "CollidesWith" << YAML::Value;
			out << YAML::BeginSeq;
			for (const auto& collidingLayer : PhysicsLayerManager::GetLayerCollisions(layer.ID))
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Name" << YAML::Value << collidingLayer.Name;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;

		// Scene Audio
		Audio::AudioEngine::Get().SerializeSceneAudio(out, mScene);

		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
		// Not implemented
		NR_CORE_ASSERT(false);
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream stream(filepath);
		NR_CORE_ASSERT(stream);

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
		{
			return false;
		}

		std::string sceneName = data["Scene"].as<std::string>();
		NR_CORE_INFO("Deserializing scene '{0}'", sceneName);

		std::vector<std::string> missingPaths;

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
				{
					name = tagComponent["Tag"].as<std::string>();
				}

				NR_CORE_INFO("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				Entity deserializedEntity = mScene->CreateEntityWithID(uuid, name);

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					auto& transform = deserializedEntity.GetComponent<TransformComponent>();
					transform.Translation = transformComponent["Position"].as<glm::vec3>();
					transform.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					transform.Scale = transformComponent["Scale"].as<glm::vec3>();

					NR_CORE_INFO("   Entity Transform:");
					NR_CORE_INFO("    Translation: {0}, {1}, {2}", transform.Translation.x, transform.Translation.y, transform.Translation.z);
					NR_CORE_INFO("    Rotation: {0}, {1}, {2}", transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
					NR_CORE_INFO("    Scale: {0}, {1}, {2}", transform.Scale.x, transform.Scale.y, transform.Scale.z);
				}

				{
					auto& relationshipComponent = deserializedEntity.GetComponent<RelationshipComponent>();
					uint64_t parentHandle = entity["Parent"].as<uint64_t>();
					relationshipComponent.ParentHandle = parentHandle;

					auto children = entity["Children"];
					for (auto child : children)
					{
						uint64_t childHandle = child["Handle"].as<uint64_t>();
						relationshipComponent.Children.push_back(childHandle);
					}
				}

				auto scriptComponent = entity["ScriptComponent"];
				if (scriptComponent)
				{
					std::string moduleName = scriptComponent["ModuleName"].as<std::string>();
					deserializedEntity.AddComponent<ScriptComponent>(moduleName);

					NR_CORE_INFO("  Script Module: {0}", moduleName);

					if (ScriptEngine::ModuleExists(moduleName))
					{
						auto storedFields = scriptComponent["StoredFields"];
						if (storedFields)
						{
							for (auto field : storedFields)
							{
								std::string name = field["Name"].as<std::string>();
								std::string typeName = field["TypeName"] ? field["TypeName"].as<std::string>() : "";
								FieldType type = (FieldType)field["Type"].as<uint32_t>();
								EntityInstanceData& data = ScriptEngine::GetEntityInstanceData(mScene->GetID(), uuid);
								auto& moduleFieldMap = data.ModuleFieldMap;
								auto& publicFields = moduleFieldMap[moduleName];
								if (publicFields.find(name) == publicFields.end())
								{
									PublicField pf = { name, typeName, type };
									publicFields.emplace(name, std::move(pf));
								}
								auto dataNode = field["Data"];
								switch (type)
								{
								case FieldType::Float:
								{
									publicFields.at(name).SetStoredValue(dataNode.as<float>());
									break;
								}
								case FieldType::Int:
								{
									publicFields.at(name).SetStoredValue(dataNode.as<int32_t>());
									break;
								}
								case FieldType::UnsignedInt:
								{
									publicFields.at(name).SetStoredValue(dataNode.as<uint32_t>());
									break;
								}
								case FieldType::String:
								{
									NR_CORE_ASSERT(false, "Unimplemented");
									break;
								}
								case FieldType::Vec2:
								{
									publicFields.at(name).SetStoredValue(dataNode.as<glm::vec2>());
									break;
								}
								case FieldType::Vec3:
								{
									publicFields.at(name).SetStoredValue(dataNode.as<glm::vec3>());
									break;
								}
								case FieldType::Vec4:
								{
									publicFields.at(name).SetStoredValue(dataNode.as<glm::vec4>());
									break;
								}
								}
							}
						}
					}
				}

				auto meshComponent = entity["MeshComponent"];
				if (meshComponent)
				{
					auto& component = deserializedEntity.AddComponent<MeshComponent>();

					AssetHandle assetHandle = 0;
					if (meshComponent["AssetPath"])
					{
						assetHandle = AssetManager::GetAssetHandleFromFilePath(meshComponent["AssetPath"].as<std::string>());
					}
					else
					{
						assetHandle = meshComponent["AssetID"].as<uint64_t>();
					}

					if (AssetManager::IsAssetHandleValid(assetHandle))
					{
						component.MeshObj = Ref<Mesh>::Create(AssetManager::GetAsset<MeshAsset>(assetHandle));
					}
					else
					{
						component.MeshObj = Ref<Asset>::Create().As<Mesh>();
						component.MeshObj->ModifyFlags(AssetFlag::Missing, true);

						std::string filepath = meshComponent["AssetPath"] ? meshComponent["AssetPath"].as<std::string>() : "";
						if (filepath.empty())
						{
							NR_CORE_ERROR("Tried to load non-existent mesh in Entity: {0}", (uint64_t)deserializedEntity.GetID());
						}
						else
						{
							NR_CORE_ERROR("Tried to load invalid mesh '{0}' in Entity {1}", filepath, (uint64_t)deserializedEntity.GetID());
							component.MeshObj->ModifyFlags(AssetFlag::Invalid, true);
						}
					}
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					auto& component = deserializedEntity.AddComponent<CameraComponent>();
					const auto& cameraNode = cameraComponent["Camera"];
					component.CameraObj = SceneCamera();

					auto& camera = component.CameraObj;					
					if (cameraNode.IsMap())
					{
						if (cameraNode["ProjectionType"])
						{
							camera.SetProjectionType((SceneCamera::ProjectionType)cameraNode["ProjectionType"].as<int>());
						}
						if (cameraNode["PerspectiveFOV"])
						{
							camera.SetPerspectiveVerticalFOV(cameraNode["PerspectiveFOV"].as<float>());
						}
						if (cameraNode["PerspectiveNear"])
						{
							camera.SetPerspectiveNearClip(cameraNode["PerspectiveNear"].as<float>());
						}
						if (cameraNode["PerspectiveFar"])
						{
							camera.SetPerspectiveFarClip(cameraNode["PerspectiveFar"].as<float>());
						}
						if (cameraNode["OrthographicSize"])
						{
							camera.SetOrthographicSize(cameraNode["OrthographicSize"].as<float>());
						}
						if (cameraNode["OrthographicNear"])
						{
							camera.SetOrthographicNearClip(cameraNode["OrthographicNear"].as<float>());
						}
						if (cameraNode["OrthographicFar"])
						{
							camera.SetOrthographicFarClip(cameraNode["OrthographicFar"].as<float>());
						}
					}

					component.Primary = cameraComponent["Primary"].as<bool>();
				}

				if (auto directionalLightComponent = entity["DirectionalLightComponent"]; directionalLightComponent)
				{
					auto& component = deserializedEntity.AddComponent<DirectionalLightComponent>();
					component.Radiance = directionalLightComponent["Radiance"].as<glm::vec3>();
					component.CastShadows = directionalLightComponent["CastShadows"].as<bool>();
					component.SoftShadows = directionalLightComponent["SoftShadows"].as<bool>();
					component.LightSize = directionalLightComponent["LightSize"].as<float>();
				}

				if (auto pointLightComponent = entity["PointLightComponent"]; pointLightComponent)
				{
					auto& component = deserializedEntity.AddComponent<PointLightComponent>();
					component.Radiance = pointLightComponent["Radiance"].as<glm::vec3>();
					component.Intensity = pointLightComponent["Intensity"].as<float>();
					component.CastsShadows = pointLightComponent["CastShadows"].as<bool>();
					component.SoftShadows = pointLightComponent["SoftShadows"].as<bool>();
					component.LightSize = pointLightComponent["LightSize"].as<float>();
					component.Radius = pointLightComponent["Radius"].as<float>();
					component.MinRadius = pointLightComponent["MinRadius"].as<float>();
					component.Falloff = pointLightComponent["Falloff"].as<float>();
				}

				auto skyLightComponent = entity["SkyLightComponent"];
				if (skyLightComponent)
				{
					auto& component = deserializedEntity.AddComponent<SkyLightComponent>();

					AssetHandle assetHandle = 0;
					if (skyLightComponent["EnvironmentAssetPath"])
					{
						assetHandle = AssetManager::GetAssetHandleFromFilePath(skyLightComponent["EnvironmentAssetPath"].as<std::string>());
					}
					else
					{
						assetHandle = skyLightComponent["EnvironmentMap"].as<uint64_t>();
					}

					if (AssetManager::IsAssetHandleValid(assetHandle))
					{
						component.SceneEnvironment = AssetManager::GetAsset<Environment>(assetHandle);
					}
					else
					{
						std::string filepath = meshComponent["EnvironmentAssetPath"] ? meshComponent["EnvironmentAssetPath"].as<std::string>() : "";
						if (filepath.empty())
						{
							NR_CORE_ERROR("Tried to load non-existent environment map in Entity: {0}", (uint64_t)deserializedEntity.GetID());
						}
						else
						{
							NR_CORE_ERROR("Tried to load invalid environment map '{0}' in Entity {1}", filepath, (uint64_t)deserializedEntity.GetID());
						}
					}
					component.Intensity = skyLightComponent["Intensity"].as<float>();
					component.Angle = skyLightComponent["Angle"].as<float>();
				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto& component = deserializedEntity.AddComponent<SpriteRendererComponent>();
					component.Color = spriteRendererComponent["Color"].as<glm::vec4>();
					component.TilingFactor = spriteRendererComponent["TilingFactor"].as<float>();
				}

				auto rigidBody2DComponent = entity["RigidBody2DComponent"];
				if (rigidBody2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<RigidBody2DComponent>();
					component.BodyType = (RigidBody2DComponent::Type)rigidBody2DComponent["BodyType"].as<int>();
					component.FixedRotation = rigidBody2DComponent["FixedRotation"] ? rigidBody2DComponent["FixedRotation"].as<bool>() : false;
				}

				auto boxCollider2DComponent = entity["BoxCollider2DComponent"];
				if (boxCollider2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<BoxCollider2DComponent>();
					component.Offset = boxCollider2DComponent["Offset"].as<glm::vec2>();
					component.Size = boxCollider2DComponent["Size"].as<glm::vec2>();
					component.Density = boxCollider2DComponent["Density"] ? boxCollider2DComponent["Density"].as<float>() : 1.0f;
					component.Friction = boxCollider2DComponent["Friction"] ? boxCollider2DComponent["Friction"].as<float>() : 1.0f;
				}

				//TODO more offsets
				auto circleCollider2DComponent = entity["CircleCollider2DComponent"];
				if (circleCollider2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<CircleCollider2DComponent>();
					component.Offset = circleCollider2DComponent["Offset"].as<glm::vec2>();
					component.Radius = circleCollider2DComponent["Radius"].as<float>();
					component.Density = circleCollider2DComponent["Density"] ? circleCollider2DComponent["Density"].as<float>() : 1.0f;
					component.Friction = circleCollider2DComponent["Friction"] ? circleCollider2DComponent["Friction"].as<float>() : 1.0f;
				}

				auto rigidBodyComponent = entity["RigidBodyComponent"];
				if (rigidBodyComponent)
				{
					auto& component = deserializedEntity.AddComponent<RigidBodyComponent>();
					component.BodyType = (RigidBodyComponent::Type)rigidBodyComponent["BodyType"].as<int>();
					component.Mass = rigidBodyComponent["Mass"].as<float>();
					component.LinearDrag = rigidBodyComponent["LinearDrag"].as<float>();
					component.AngularDrag = rigidBodyComponent["AngularDrag"].as<float>();
					component.DisableGravity = rigidBodyComponent["DisableGravity"].as<bool>();
					component.IsKinematic = rigidBodyComponent["IsKinematic"].as<bool>();
					component.Layer = rigidBodyComponent["Layer"].as<uint32_t>();

					component.LockPositionX = rigidBodyComponent["Constraints"]["LockPositionX"].as<bool>();
					component.LockPositionY = rigidBodyComponent["Constraints"]["LockPositionY"].as<bool>();
					component.LockPositionZ = rigidBodyComponent["Constraints"]["LockPositionZ"].as<bool>();

					component.LockRotationX = rigidBodyComponent["Constraints"]["LockRotationX"].as<bool>();
					component.LockRotationY = rigidBodyComponent["Constraints"]["LockRotationY"].as<bool>();
					component.LockRotationZ = rigidBodyComponent["Constraints"]["LockRotationZ"].as<bool>();
				}

				auto boxColliderComponent = entity["BoxColliderComponent"];
				if (boxColliderComponent)
				{
					auto& component = deserializedEntity.AddComponent<BoxColliderComponent>();
					component.Offset = boxColliderComponent["Offset"].as<glm::vec3>();
					component.Size = boxColliderComponent["Size"].as<glm::vec3>();
					component.IsTrigger = boxColliderComponent["IsTrigger"].as<bool>();
					component.DebugMesh = MeshFactory::CreateBox(component.Size);

					auto material = boxColliderComponent["Material"];
					if (material)
					{
						if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
						{
							component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
						}
						else
						{
							NR_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", (uint64_t)deserializedEntity.GetID());
						}
					}
				}

				auto sphereColliderComponent = entity["SphereColliderComponent"];
				if (sphereColliderComponent)
				{
					auto& component = deserializedEntity.AddComponent<SphereColliderComponent>();
					component.Radius = sphereColliderComponent["Radius"].as<float>();
					component.IsTrigger = sphereColliderComponent["IsTrigger"].as<bool>();
					component.DebugMesh = MeshFactory::CreateSphere(component.Radius);

					auto material = sphereColliderComponent["Material"];
					if (material)
					{
						if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
						{
							component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
						}
						else
						{
							NR_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", (uint64_t)deserializedEntity.GetID());
						}
					}
				}

				auto capsuleColliderComponent = entity["CapsuleColliderComponent"];
				if (capsuleColliderComponent)
				{
					auto& component = deserializedEntity.AddComponent<CapsuleColliderComponent>();
					component.Radius = capsuleColliderComponent["Radius"].as<float>();
					component.Height = capsuleColliderComponent["Height"].as<float>();
					component.IsTrigger = capsuleColliderComponent["IsTrigger"].as<bool>();

					auto material = capsuleColliderComponent["Material"];
					if (material)
					{
						if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
						{
							component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
						}
						else
						{
							NR_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", (uint64_t)deserializedEntity.GetID());
						}
					}

					component.DebugMesh = MeshFactory::CreateCapsule(component.Radius, component.Height);
				}

				auto meshColliderComponent = entity["MeshColliderComponent"];
				if (meshColliderComponent)
				{
					auto& component = deserializedEntity.AddComponent<MeshColliderComponent>();
					component.IsConvex = meshColliderComponent["IsConvex"].as<bool>();
					component.IsTrigger = meshColliderComponent["IsTrigger"].as<bool>();

					component.CollisionMesh = deserializedEntity.HasComponent<MeshComponent>() ? deserializedEntity.GetComponent<MeshComponent>().MeshObj : nullptr;
					bool overrideMesh = meshColliderComponent["OverrideMesh"].as<bool>();

					if (overrideMesh)
					{
						AssetHandle assetHandle = meshColliderComponent["AssetID"] ? meshColliderComponent["AssetID"].as<uint64_t>() : 0;

						if (AssetManager::IsAssetHandleValid(assetHandle))
						{
							component.CollisionMesh = AssetManager::GetAsset<Mesh>(assetHandle);
						}
						else
						{
							overrideMesh = false;
						}
					}

					if (component.CollisionMesh && !component.CollisionMesh->IsFlagSet(AssetFlag::Missing))
					{
						component.OverrideMesh = overrideMesh;
					}
					else
					{
						NR_CORE_WARN("MeshColliderComponent in use without valid mesh!");
					}


					auto material = meshColliderComponent["Material"];
					if (material)
					{
						if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
						{
							component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
						}
						else
						{
							NR_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", (uint64_t)deserializedEntity.GetID());
						}
					}
				}

				auto audioComponent = entity["AudioComponent"];
				if (audioComponent)
				{
					auto& component = deserializedEntity.AddComponent<Audio::AudioComponent>();
					Audio::SoundConfig soundConfig;

					AssetHandle assetHandle = audioComponent["AssetID"] ? audioComponent["AssetID"].as<uint64_t>() : 0;
					if (AssetManager::IsAssetHandleValid(assetHandle))
					{
						soundConfig.FileAsset = AssetManager::GetAsset<AudioFile>(assetHandle);
					}
					else
					{
						NR_CORE_ERROR("Tried to load invalid Audio File asset in Entity {0}", (uint64_t)deserializedEntity.GetID());
					}

					soundConfig.Looping = audioComponent["IsLooping"].as<bool>();
					component.AutoDestroy = audioComponent["AutoDestroy"].as<bool>();
					component.PlayOnAwake = audioComponent["PlayOnAwake"].as<bool>();
					component.VolumeMultiplier = audioComponent["VolumeMultiplier"].as<float>();
					component.PitchMultiplier = audioComponent["PitchMultiplier"].as<float>();
					soundConfig.VolumeMultiplier = audioComponent["VolumeMultiplier"].as<float>();
					soundConfig.PitchMultiplier = audioComponent["PitchMultiplier"].as<float>();
					soundConfig.MasterReverbSend = audioComponent["MasterReverbSend"] ? audioComponent["MasterReverbSend"].as<float>() : 0.0f;
					soundConfig.LPFilterValue = audioComponent["LPFilterValue"].as<float>();
					soundConfig.HPFilterValue = audioComponent["HPFilterValue"].as<float>();

					auto spConfig = audioComponent["Spatialization"];
					if (spConfig)
					{
						bool spatialEnabled = spConfig["Enabled"].as<bool>();
						soundConfig.SpatializationEnabled = spConfig["Enabled"].as<bool>();

						auto& spatialConfig = soundConfig.Spatialization;
						spatialConfig.AttenuationMod = static_cast<Audio::AttenuationModel>(spConfig["AttenuationModel"].as<int>());

						spatialConfig.MinGain = spConfig["MinGain"].as<float>();
						spatialConfig.MaxGain = spConfig["MaxGain"].as<float>();

						spatialConfig.MinDistance = spConfig["MinDistance"].as<float>();
						spatialConfig.MaxDistance = spConfig["MaxDistance"].as<float>();

						spatialConfig.ConeInnerAngleInRadians = spConfig["ConeInnerAngle"].as<float>();
						spatialConfig.ConeOuterAngleInRadians = spConfig["ConeOuterAngle"].as<float>();

						spatialConfig.ConeOuterGain = spConfig["ConeOuterGain"].as<float>();
						spatialConfig.DopplerFactor = spConfig["DopplerFactor"].as<float>();
						spatialConfig.Rolloff = spConfig["Rollor"] ? spConfig["Rollor"].as<float>() : 1.0f;

						spatialConfig.AirAbsorptionEnabled = spConfig["AirAbsorptionEnabled"].as<bool>();
					}

					component.SoundConfig = soundConfig;
				}

				auto audioListener = entity["AudioListenerComponent"];
				if (audioListener)
				{
					auto& component = deserializedEntity.AddComponent<AudioListenerComponent>();
					component.Active = audioListener["Active"].as<bool>();
					component.ConeInnerAngleInRadians = audioListener["ConeInnerAngle"].as<float>();
					component.ConeOuterAngleInRadians = audioListener["ConeOuterAngle"].as<float>();
					component.ConeOuterGain = audioListener["ConeOuterGain"].as<float>();
				}
			}
		}

		auto physicsLayers = data["PhysicsLayers"];
		if (physicsLayers)
		{
			for (auto layer : physicsLayers)
			{
				PhysicsLayerManager::AddLayer(layer["Name"].as<std::string>(), false);
			}

			for (auto layer : physicsLayers)
			{
				const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(layer["Name"].as<std::string>());

				auto collidesWith = layer["CollidesWith"];
				if (collidesWith)
				{
					for (auto collisionLayer : collidesWith)
					{
						const auto& otherLayer = PhysicsLayerManager::GetLayer(collisionLayer["Name"].as<std::string>());
						PhysicsLayerManager::SetLayerCollision(layerInfo.ID, otherLayer.ID, true);
					}
				}
			}
		}

		auto sceneAudio = data["SceneAudio"];
		if (sceneAudio)
		{
			Audio::AudioEngine::Get().DeserializeSceneAudio(sceneAudio);
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		NR_CORE_ASSERT(false);
		return false;
	}
}