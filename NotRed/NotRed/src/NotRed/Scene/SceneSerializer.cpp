#include "nrpch.h"
#include "SceneSerializer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>

#include "Entity.h"
#include "Components.h"
#include "NotRed/Script/ScriptEngine.h"
#include "NotRed/Physics/PhysicsLayer.h"
#include "NotRed/Physics/CookingFactory.h"
#include "NotRed/Renderer/MeshFactory.h"
#include "NotRed/Audio/AudioComponent.h"
#include "NotRed/Audio/AudioEngine.h"

#include "NotRed/Asset/AssetManager.h"

#include "yaml-cpp/yaml.h"

#include "NotRed/Util/YAMLSerializationHelpers.h"

namespace NR
{
	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: mScene(scene)
	{

	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		UUID uuid = entity.GetComponent<IDComponent>().ID;
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity";
		out << YAML::Value << uuid;

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<RelationshipComponent>())
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

		if (entity.HasComponent<PrefabComponent>())
		{
			out << YAML::Key << "PrefabComponent";
			out << YAML::BeginMap; // PrefabComponent

			auto& prefabComponent = entity.GetComponent<PrefabComponent>();
			out << YAML::Key << "Prefab" << YAML::Value << prefabComponent.PrefabID;
			out << YAML::Key << "Entity" << YAML::Value << prefabComponent.EntityID;

			out << YAML::EndMap; // PrefabComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& transform = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Position" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<ScriptComponent>())
		{
			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap; // ScriptComponent

			ScriptComponent& scriptCoponent = entity.GetComponent<ScriptComponent>();
			auto& moduleName = scriptCoponent.ModuleName;
			out << YAML::Key << "ModuleName" << YAML::Value << moduleName;

			if (!moduleName.empty())
			{
				const auto& moduleFieldMap = scriptCoponent.ModuleFieldMap;
				if (moduleFieldMap.find(moduleName) != moduleFieldMap.end())
				{
					const auto& fields = moduleFieldMap.at(moduleName);
					out << YAML::Key << "StoredFields" << YAML::Value;
					out << YAML::BeginSeq;
					for (const auto& [name, field] : fields)
					{
						if (field.Type == FieldType::None)
						{
							NR_CORE_WARN("C# field {0} not serialized, unknown type", field.Name);
							continue;
						}

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
						case FieldType::String:
						{
							out << field.GetStoredValue<const std::string&>();
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
						case FieldType::Asset:
						{
							out << field.GetStoredValue<UUID>();
							break;
						}
						default:
						{
							NR_CORE_ASSERT(false);
							break;
						}
						}
						out << YAML::EndMap; // Field
					}
					out << YAML::EndSeq;
				}
			}

			out << YAML::EndMap; // ScriptComponent
		}

		if (entity.HasComponent<MeshComponent>())
		{
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap; // MeshComponent

			MeshComponent& mc = entity.GetComponent<MeshComponent>();
			if (mc.MeshObj)
			{
				out << YAML::Key << "AssetID" << YAML::Value << (mc.MeshObj ? mc.MeshObj->Handle : (AssetHandle)0);
			}

			auto materialTable = mc.Materials;
			if (materialTable->GetMaterialCount() > 0)
			{
				out << YAML::Key << "MaterialTable" << YAML::Value << YAML::BeginMap; // MaterialTable

				for (uint32_t i = 0; i < materialTable->GetMaterialCount(); ++i)
				{
					AssetHandle handle = (materialTable->HasMaterial(i) ? materialTable->GetMaterial(i)->Handle : (AssetHandle)0);
					out << YAML::Key << i << YAML::Value << handle;
				}

				out << YAML::EndMap; // MaterialTable
			}

			out << YAML::EndMap; // MeshComponent
		}

		if (entity.HasComponent<ParticleComponent>())
		{
			out << YAML::Key << "ParticleComponent";
			out << YAML::BeginMap; // ParticleComponent

			auto particleComponent = entity.GetComponent<ParticleComponent>();

			out << YAML::Key << "ParticleCount" << YAML::Value << particleComponent.ParticleCount;
			out << YAML::Key << "Velocity" << YAML::Value << particleComponent.Velocity;

			out << YAML::Key << "StarColor" << YAML::Value << particleComponent.StarColor;
			out << YAML::Key << "DustColor" << YAML::Value << particleComponent.DustColor;
			out << YAML::Key << "h2RegionColor" << YAML::Value << particleComponent.h2RegionColor;

			out << YAML::EndMap; // ParticleComponent
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
			out << YAML::EndMap; // Camera
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
			if (skyLightComponent.SceneEnvironment)
			{
				out << YAML::Key << "EnvironmentMap" << YAML::Value << skyLightComponent.SceneEnvironment->Handle;
			}
			out << YAML::Key << "Intensity" << YAML::Value << skyLightComponent.Intensity;			
			out << YAML::Key << "Lod" << YAML::Value << skyLightComponent.Lod;
			out << YAML::Key << "DynamicSky" << YAML::Value << skyLightComponent.DynamicSky;
			if (skyLightComponent.DynamicSky)
			{
				out << YAML::Key << "TurbidityAzimuthInclination" << YAML::Value << skyLightComponent.TurbidityAzimuthInclination;
			}

			out << YAML::EndMap; // SkyLightComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;
			if (spriteRendererComponent.Texture)
				out << YAML::Key << "TextureAssetPath" << YAML::Value << "path/to/asset";
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
			out << YAML::Key << "OverrideMesh" << YAML::Value << meshColliderComponent.OverrideMesh;

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

		if (entity.HasComponent<AudioComponent>())
		{
#define NR_SERIALIZE_PROPERTY(propName, propVal, outputData) outputData << YAML::Key << #propName << YAML::Value << propVal

			out << YAML::Key << "AudioComponent";
			out << YAML::BeginMap; // AudioComponent

			auto& audioComponent = entity.GetComponent<AudioComponent>();
			NR_SERIALIZE_PROPERTY(StartEvent, audioComponent.StartEvent, out);
			NR_SERIALIZE_PROPERTY(StartCommandID, audioComponent.StartCommandID, out);
			NR_SERIALIZE_PROPERTY(PlayOnAwake, audioComponent.PlayOnAwake, out);
			NR_SERIALIZE_PROPERTY(StopIfEntityDestroyed, audioComponent.StopIfEntityDestroyed, out);
			NR_SERIALIZE_PROPERTY(VolumeMultiplier, audioComponent.VolumeMultiplier, out);
			NR_SERIALIZE_PROPERTY(PitchMultiplier, audioComponent.PitchMultiplier, out);
			NR_SERIALIZE_PROPERTY(AutoDestroy, audioComponent.AutoDestroy, out);

			out << YAML::EndMap;
		}

		if (entity.HasComponent<AudioListenerComponent>())
		{
			out << YAML::Key << "AudioListenerComponent";
			out << YAML::BeginMap; // AudioComponent
			auto& audioListenerComponent = entity.GetComponent<AudioListenerComponent>();			
			NR_SERIALIZE_PROPERTY(Active, audioListenerComponent.Active, out);
			NR_SERIALIZE_PROPERTY(ConeInnerAngle, audioListenerComponent.ConeInnerAngleInRadians, out);
			NR_SERIALIZE_PROPERTY(ConeOuterAngle, audioListenerComponent.ConeOuterAngleInRadians, out);
			NR_SERIALIZE_PROPERTY(ConeOuterGain, audioListenerComponent.ConeOuterGain, out);
			out << YAML::EndMap; // AudioComponent

#undef NR_SERIALIZE_PROPERTY
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
		out << YAML::Key << "DirLight" << YAML::Value;
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
		out << YAML::Value << mScene->GetName();

		if (mScene->GetEnvironment())
		{
			SerializeEnvironment(out, mScene);
		}

		out << YAML::Key << "Entities";
		out << YAML::Value << YAML::BeginSeq;
		for (auto entity : mScene->mRegistry.view<IDComponent>())
		{
			SerializeEntity(out, { entity, mScene.Raw() });
		}
		out << YAML::EndSeq;

		// Scene Audio
		AudioEngine::Get().SerializeSceneAudio(out, mScene);

		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
		// Not implemented
		NR_CORE_ASSERT(false);
	}

	void SceneSerializer::DeserializeEntities(YAML::Node& entitiesNode, Ref<Scene> scene)
	{
		for (auto entity : entitiesNode)
		{
			uint64_t uuid = entity["Entity"].as<uint64_t>();

			std::string name;
			auto tagComponent = entity["TagComponent"];
			if (tagComponent)
			{
				name = tagComponent["Tag"].as<std::string>();
			}

			NR_CORE_INFO("Deserialized entity with ID = {0}, name = {1}", uuid, name);

			Entity deserializedEntity = scene->CreateEntityWithID(uuid, name);

			auto& relationshipComponent = deserializedEntity.GetComponent<RelationshipComponent>();
			uint64_t parentHandle = entity["Parent"] ? entity["Parent"].as<uint64_t>() : 0;
			relationshipComponent.ParentHandle = parentHandle;

			auto children = entity["Children"];
			if (children)
			{
				for (auto child : children)
				{
					uint64_t childHandle = child["Handle"].as<uint64_t>();
					relationshipComponent.Children.push_back(childHandle);
				}
			}

			auto prefabComponent = entity["PrefabComponent"];
			if (prefabComponent)
			{
				auto& pb = deserializedEntity.AddComponent<PrefabComponent>();

				pb.PrefabID = prefabComponent["Prefab"].as<uint64_t>();
				pb.EntityID = prefabComponent["Entity"].as<uint64_t>();
			}

			auto transformComponent = entity["TransformComponent"];
			if (transformComponent)
			{
				// Entities always have transforms
				auto& transform = deserializedEntity.GetComponent<TransformComponent>();
				transform.Translation = transformComponent["Position"].as<glm::vec3>();
				const auto& rotationNode = transformComponent["Rotation"];
				// Rotations used to be stored as quaternions
				if (rotationNode.size() == 4)
				{
					glm::quat rotation = transformComponent["Rotation"].as<glm::quat>();
					transform.Rotation = glm::eulerAngles(rotation);
				}
				else
				{
					NR_CORE_ASSERT(rotationNode.size() == 3);
					transform.Rotation = transformComponent["Rotation"].as<glm::vec3>();
				}
				transform.Scale = transformComponent["Scale"].as<glm::vec3>();

				NR_CORE_INFO("  Entity Transform:");
				NR_CORE_INFO("    Translation: {0}, {1}, {2}", transform.Translation.x, transform.Translation.y, transform.Translation.z);
				NR_CORE_INFO("    Rotation: {0}, {1}, {2}", transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
				NR_CORE_INFO("    Scale: {0}, {1}, {2}", transform.Scale.x, transform.Scale.y, transform.Scale.z);
			}

			auto scriptComponent = entity["ScriptComponent"];
			if (scriptComponent)
			{
				std::string moduleName = scriptComponent["ModuleName"].as<std::string>();
				ScriptComponent& sc = deserializedEntity.AddComponent<ScriptComponent>(moduleName);

				NR_CORE_INFO("  Script Module: {0}", moduleName);

				if (ScriptEngine::ModuleExists(moduleName))
				{
					auto storedFields = scriptComponent["StoredFields"];
					if (storedFields)
					{
						for (auto field : storedFields)
						{
							std::string name = field["Name"].as<std::string>();
							auto& moduleFieldMap = sc.ModuleFieldMap;
							auto& publicFields = moduleFieldMap[moduleName];
							
							auto publicFieldIter = publicFields.find(name);
							if (publicFieldIter != publicFields.end())
							{
								auto& publicField = publicFieldIter->second;
								auto dataNode = field["Data"];
								switch (publicField.Type)
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
								case FieldType::Asset:
								{
									publicFields.at(name).SetStoredValue(dataNode.as<UUID>());
									break;
								}
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
					const AssetMetadata& metadata = AssetManager::GetMetadata(assetHandle);
					if (metadata.Type == AssetType::Mesh)
					{
						component.MeshObj = AssetManager::GetAsset<Mesh>(assetHandle);
					}

					else if (metadata.Type == AssetType::MeshAsset)
					{
						// Create new mesh
						Ref<MeshAsset> meshAsset = AssetManager::GetAsset<MeshAsset>(assetHandle);
						std::filesystem::path meshPath = metadata.FilePath;
						std::filesystem::path meshDirectory = Project::GetMeshPath();
						std::string filename = fmt::format("{0}.hmesh", meshPath.stem().string());
						Ref<Mesh> mesh = AssetManager::CreateNewAsset<Mesh>(filename, meshDirectory.string(), meshAsset);
						component.MeshObj = mesh;
						AssetImporter::Serialize(mesh);
					}
				}
				else
				{
					component.MeshObj = Ref<Asset>::Create().As<Mesh>();
					component.MeshObj->ModifyFlags(AssetFlag::Missing, true);

					std::string filepath = meshComponent["AssetPath"] ? meshComponent["AssetPath"].as<std::string>() : "";
					if (filepath.empty())
					{
						NR_CORE_ERROR("Tried to load non-existent mesh in Entity: {0}", deserializedEntity.GetID());
					}
					else
					{
						NR_CORE_ERROR("Tried to load invalid mesh '{0}' in Entity {1}", filepath, deserializedEntity.GetID());
						component.MeshObj->ModifyFlags(AssetFlag::Invalid, true);
					}
				}

				if (meshComponent["MaterialTable"])
				{
					YAML::Node materialTableNode = meshComponent["MaterialTable"];
					for (auto materialEntry : materialTableNode)
					{
						uint32_t index = materialEntry.first.as<uint32_t>();
						AssetHandle materialAsset = materialEntry.second.as<AssetHandle>();
						if (materialAsset && AssetManager::IsAssetHandleValid(materialAsset))
						{
							component.Materials->SetMaterial(index, AssetManager::GetAsset<MaterialAsset>(materialAsset));
						}
					}
				}
			}

			auto particleComponent = entity["ParticleComponent"];
			if (particleComponent)
			{
				int particleCount = particleComponent["ParticleCount"].as<int>();

				auto& component = deserializedEntity.AddComponent<ParticleComponent>(particleCount);

				component.Velocity = particleComponent["Velocity"].as<float>();

				component.StarColor = particleComponent["StarColor"].as<glm::vec3>();
				component.DustColor = particleComponent["DustColor"].as<glm::vec3>();
				component.h2RegionColor = particleComponent["h2RegionColor"].as<glm::vec3>();

				auto mat = component.MeshObj->GetMaterials()->GetMaterial(0)->GetMaterial();
				mat->Set("uGalaxySpecs.StarColor", component.StarColor);
				mat->Set("uGalaxySpecs.DustColor", component.DustColor);
				mat->Set("uGalaxySpecs.h2RegionColor", component.h2RegionColor);
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

				if (skyLightComponent["EnvironmentMap"])
				{
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
						std::string filepath = skyLightComponent["EnvironmentAssetPath"] ? skyLightComponent["EnvironmentAssetPath"].as<std::string>() : "";
						if (filepath.empty())
						{
							NR_CORE_ERROR("Tried to load non-existent environment map in Entity: {0}", deserializedEntity.GetID());
						}
						else
						{
							NR_CORE_ERROR("Tried to load invalid environment map '{0}' in Entity {1}", filepath, deserializedEntity.GetID());
						}
					}
				}

				component.Intensity = skyLightComponent["Intensity"].as<float>();
				auto lod = skyLightComponent["Lod"];
				if (lod)
				{
					component.Lod = lod.as<int>();
				}
				else
				{
					component.Lod = 1;
				}
				if (skyLightComponent["DynamicSky"])
				{
					component.DynamicSky = skyLightComponent["DynamicSky"].as<bool>();
					if (component.DynamicSky)
					{
						component.TurbidityAzimuthInclination = skyLightComponent["TurbidityAzimuthInclination"].as<glm::vec3>();
					}
				}
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
				if (rigidBodyComponent["BodyType"] && (RigidBodyComponent::Type)rigidBodyComponent["BodyType"].as<int>() == RigidBodyComponent::Type::Dynamic)
				{
					auto& component = deserializedEntity.AddComponent<RigidBodyComponent>();
					component.BodyType = (RigidBodyComponent::Type)rigidBodyComponent["BodyType"].as<int>();
					component.Mass = rigidBodyComponent["Mass"].as<float>();
					component.LinearDrag = rigidBodyComponent["LinearDrag"] ? rigidBodyComponent["LinearDrag"].as<float>() : 0.0f;
					component.AngularDrag = rigidBodyComponent["AngularDrag"] ? rigidBodyComponent["AngularDrag"].as<float>() : 0.05f;
					component.DisableGravity = rigidBodyComponent["DisableGravity"] ? rigidBodyComponent["DisableGravity"].as<bool>() : false;
					component.IsKinematic = rigidBodyComponent["IsKinematic"] ? rigidBodyComponent["IsKinematic"].as<bool>() : false;
					component.Layer = rigidBodyComponent["Layer"] ? rigidBodyComponent["Layer"].as<uint32_t>() : 0;
					component.LockPositionX = rigidBodyComponent["Constraints"]["LockPositionX"].as<bool>();
					component.LockPositionY = rigidBodyComponent["Constraints"]["LockPositionY"].as<bool>();
					component.LockPositionZ = rigidBodyComponent["Constraints"]["LockPositionZ"].as<bool>();
					component.LockRotationX = rigidBodyComponent["Constraints"]["LockRotationX"].as<bool>();
					component.LockRotationY = rigidBodyComponent["Constraints"]["LockRotationY"].as<bool>();
					component.LockRotationZ = rigidBodyComponent["Constraints"]["LockRotationZ"].as<bool>();
				}
			}

			auto boxColliderComponent = entity["BoxColliderComponent"];
			if (boxColliderComponent)
			{
				auto& component = deserializedEntity.AddComponent<BoxColliderComponent>();
				component.Offset = boxColliderComponent["Offset"].as<glm::vec3>();
				component.Size = boxColliderComponent["Size"].as<glm::vec3>();
				component.IsTrigger = boxColliderComponent["IsTrigger"] ? boxColliderComponent["IsTrigger"].as<bool>() : false;

				auto material = boxColliderComponent["Material"];
				if (material)
				{
					if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
					{
						component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
					}
					else
					{
						NR_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", deserializedEntity.GetID());
					}
				}

				component.DebugMesh = MeshFactory::CreateBox(component.Size);
			}

			auto sphereColliderComponent = entity["SphereColliderComponent"];
			if (sphereColliderComponent)
			{
				auto& component = deserializedEntity.AddComponent<SphereColliderComponent>();
				component.Radius = sphereColliderComponent["Radius"].as<float>();
				component.IsTrigger = sphereColliderComponent["IsTrigger"] ? sphereColliderComponent["IsTrigger"].as<bool>() : false;

				auto material = sphereColliderComponent["Material"];
				if (material)
				{
					if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
					{
						component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
					}
					else
					{
						NR_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", deserializedEntity.GetID());
					}
				}

				component.DebugMesh = MeshFactory::CreateSphere(component.Radius);
			}

			auto capsuleColliderComponent = entity["CapsuleColliderComponent"];
			if (capsuleColliderComponent)
			{
				auto& component = deserializedEntity.AddComponent<CapsuleColliderComponent>();
				component.Radius = capsuleColliderComponent["Radius"].as<float>();
				component.Height = capsuleColliderComponent["Height"].as<float>();
				component.IsTrigger = capsuleColliderComponent["IsTrigger"] ? capsuleColliderComponent["IsTrigger"].as<bool>() : false;

				auto material = capsuleColliderComponent["Material"];
				if (material)
				{
					if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
					{
						component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
					}
					else
					{
						NR_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", deserializedEntity.GetID());
					}
				}

				component.DebugMesh = MeshFactory::CreateCapsule(component.Radius, component.Height);
			}

			auto meshColliderComponent = entity["MeshColliderComponent"];
			if (meshColliderComponent)
			{
				auto& component = deserializedEntity.AddComponent<MeshColliderComponent>();
				component.IsConvex = meshColliderComponent["IsConvex"] ? meshColliderComponent["IsConvex"].as<bool>() : false;
				component.IsTrigger = meshColliderComponent["IsTrigger"] ? meshColliderComponent["IsTrigger"].as<bool>() : false;

				component.CollisionMesh = deserializedEntity.HasComponent<MeshComponent>() ? deserializedEntity.GetComponent<MeshComponent>().MeshObj : nullptr;
				bool overrideMesh = meshColliderComponent["OverrideMesh"] ? meshColliderComponent["OverrideMesh"].as<bool>() : false;

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
					CookingFactory::CookMesh(component);
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
						NR_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", deserializedEntity.GetID());
					}
				}
			}

			// NOTE(Peter): Compatibility fix for older scenes
			auto physicsMaterialComponent = entity["PhysicsMaterialComponent"];
			if (physicsMaterialComponent)
			{
				Ref<PhysicsMaterial> material = Ref<PhysicsMaterial>::Create();
				material->StaticFriction = physicsMaterialComponent["StaticFriction"].as<float>();
				material->DynamicFriction = physicsMaterialComponent["DynamicFriction"].as<float>();
				material->Bounciness = physicsMaterialComponent["Bounciness"].as<float>();

				if (deserializedEntity.HasComponent<BoxColliderComponent>())
				{
					deserializedEntity.GetComponent<BoxColliderComponent>().Material = material;
				}
				if (deserializedEntity.HasComponent<SphereColliderComponent>())
				{
					deserializedEntity.GetComponent<SphereColliderComponent>().Material = material;
				}
				if (deserializedEntity.HasComponent<CapsuleColliderComponent>())
				{
					deserializedEntity.GetComponent<CapsuleColliderComponent>().Material = material;
				}
				if (deserializedEntity.HasComponent<MeshColliderComponent>())
				{
					deserializedEntity.GetComponent<MeshColliderComponent>().Material = material;
				}
			}

			auto audioComponent = entity["AudioComponent"];
			if (audioComponent)
			{
#define NR_DESERIALIZE_PROPERTY(propName, destination, inputData, defaultValue) destination = inputData[#propName] ? inputData[#propName].as<decltype(defaultValue)>() : defaultValue

				auto& component = deserializedEntity.AddComponent<AudioComponent>();

				NR_DESERIALIZE_PROPERTY(StartEvent, component.StartEvent, audioComponent, std::string(""));
				component.StartCommandID = audioComponent["StartCommandID"] ? Audio::CommandID::FromUnsignedInt(audioComponent["StartCommandID"].as<uint32_t>()) : Audio::CommandID::InvalidID();

				NR_DESERIALIZE_PROPERTY(PlayOnAwake, component.PlayOnAwake, audioComponent, false);
				NR_DESERIALIZE_PROPERTY(StopIfEntityDestroyed, component.StopIfEntityDestroyed, audioComponent, true);
				NR_DESERIALIZE_PROPERTY(VolumeMultiplier, component.VolumeMultiplier, audioComponent, 1.0f);
				NR_DESERIALIZE_PROPERTY(PitchMultiplier, component.PitchMultiplier, audioComponent, 1.0f);
				NR_DESERIALIZE_PROPERTY(AutoDestroy, component.AutoDestroy, audioComponent, false);
			}

			auto audioListener = entity["AudioListenerComponent"];
			if (audioListener)
			{
				auto& component = deserializedEntity.AddComponent<AudioListenerComponent>();
				component.Active = audioListener["Active"] ? audioListener["Active"].as<bool>() : false;
				component.ConeInnerAngleInRadians = audioListener["ConeInnerAngle"] ? audioListener["ConeInnerAngle"].as<float>() : 6.283185f;
				component.ConeOuterAngleInRadians = audioListener["ConeOuterAngle"] ? audioListener["ConeOuterAngle"].as<float>() : 6.283185f;
				component.ConeOuterGain = audioListener["ConeOuterGain"] ? audioListener["ConeOuterGain"].as<float>() : 1.0f;
			}
#undef NR_DESERIALIZE_PROPERTY
		}
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
		mScene->SetName(sceneName);

		auto entities = data["Entities"];
		if (entities)
		{
			DeserializeEntities(entities, mScene);
		}

		auto sceneAudio = data["SceneAudio"];
		if (sceneAudio)
		{
			AudioEngine::Get().DeserializeSceneAudio(sceneAudio);
		}

		// Sort IdComponent by by entity handle (which is essentially the order in which they were created)
		mScene->mRegistry.sort<IDComponent>([this](const auto lhs, const auto rhs)
			{
				auto lhsEntity = mScene->GetEntityMap().find(lhs.ID);
				auto rhsEntity = mScene->GetEntityMap().find(rhs.ID);
				return static_cast<uint32_t>(lhsEntity->second) < static_cast<uint32_t>(rhsEntity->second);
			});

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		// Not implemented
		NR_CORE_ASSERT(false);
		return false;
	}
}