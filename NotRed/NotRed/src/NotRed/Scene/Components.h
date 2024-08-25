#pragma once

#include <glm/glm.hpp>

#include "NotRed/Core/UUID.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Scene/SceneCamera.h"

namespace NR
{
	struct IDComponent
	{
		UUID ID = 0;
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent& other)
			: Tag(other.Tag) {}
		TagComponent(const std::string& tag)
			: Tag(tag) {}

		operator std::string& () { return Tag; }
		operator const std::string& () const { return Tag; }
	};

	struct TransformComponent
	{
		glm::mat4 Transform;

		TransformComponent() = default;
		TransformComponent(const TransformComponent& other)
			: Transform(other.Transform) {}
		TransformComponent(const glm::mat4& transform)
			: Transform(transform) {}

		operator glm::mat4& () { return Transform; }
		operator const glm::mat4& () const { return Transform; }
	};

	struct MeshComponent
	{
		Ref<Mesh> MeshObj;

		MeshComponent() = default;
		MeshComponent(const MeshComponent& other)
			: MeshObj(other.MeshObj) {}
		MeshComponent(const Ref<Mesh>& mesh)
			: MeshObj(mesh) {}

		operator Ref<Mesh>() { return MeshObj; }
	};

	struct ScriptComponent
	{
		std::string ModuleName;

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent& other)
			: ModuleName(other.ModuleName) {}
		ScriptComponent(const std::string& moduleName)
			: ModuleName(moduleName) {}
	};

	struct CameraComponent
	{
		SceneCamera CameraObj;
		bool Primary = true;

		CameraComponent() = default;
		CameraComponent(const CameraComponent& other)
			: CameraObj(other.CameraObj), Primary(other.Primary) {}

		operator SceneCamera& () { return CameraObj; }
		operator const SceneCamera& () const { return CameraObj; }
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture;
		float TilingFactor = 1.0f;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent& other)
			: Color(other.Color), Texture(other.Texture), TilingFactor(other.TilingFactor) {}
	};
}