#pragma once

#include "NotRed/Core/UUID.h"

namespace NR
{
	enum class AssetType
	{
		Scene, 
		Mesh, Texture, EnvMap, 
		Audio, 
		Script, 
		PhysicsMat, 
		Other
	};

	using AssetHandle = size_t;

	class Asset : public RefCounted
	{
	public:
		AssetHandle Handle;
		AssetType Type;

		std::string FilePath;
		std::string FileName;
		std::string Extension;
		int ParentDirectory;

		virtual ~Asset() = default;
	};

	class PhysicsMaterial : public Asset
	{
	public:
		float StaticFriction;
		float DynamicFriction;
		float Bounciness;

		PhysicsMaterial() = default;
		PhysicsMaterial(float staticFriction, float dynamicFriction, float bounciness)
			: StaticFriction(staticFriction), DynamicFriction(dynamicFriction), Bounciness(bounciness)
		{
		}
	};

}