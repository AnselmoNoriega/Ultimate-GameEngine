#pragma once

#include <Entt/include/entt.hpp>

#include "NotRed/Core/UUID.h"

namespace NR
{
	enum class AssetType : int8_t
	{
		Scene, 
		Mesh, Texture, EnvMap, 
		Audio,
		Script,
		PhysicsMat,
		Directory,
		Other,
		None,
		Missing
	};

	using AssetHandle = UUID;

	class Asset : public RefCounted
	{
	public:
		AssetHandle Handle;
		AssetType Type = AssetType::None;

		std::string FilePath;
		std::string FileName;
		std::string Extension;
		AssetHandle ParentDirectory;
		bool IsDataLoaded = false;

		virtual bool operator==(const Asset& other) const
		{
			return Handle == other.Handle;
		}

		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}

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

	class Directory : public Asset
	{
	public:
		std::vector<AssetHandle> ChildDirectories;
		std::vector<AssetHandle> Assets;

		Directory() = default;
	};
}