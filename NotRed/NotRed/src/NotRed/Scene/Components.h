#pragma once

#include <glm/glm.hpp>

#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/Camera.h"

namespace NR
{
	struct TagComponent
	{
		std::string Tag;

		operator std::string& () { return Tag; }
		operator const std::string& () const { return Tag; }
	};

	struct TransformComponent
	{
		glm::mat4 Transform;

		operator glm::mat4& () { return Transform; }
		operator const glm::mat4& () const { return Transform; }
	};

	struct MeshComponent
	{
		Ref<Mesh> MeshObj;

		operator Ref<Mesh>() { return MeshObj; }
	};

	struct ScriptComponent
	{
		std::string ModuleName;
	};

	struct CameraComponent
	{
		Camera CameraObj;
		bool Primary = true;

		operator Camera& () { return CameraObj; }
		operator const Camera& () const { return CameraObj; }
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture;
		float TilingFactor = 1.0f;
	};


}