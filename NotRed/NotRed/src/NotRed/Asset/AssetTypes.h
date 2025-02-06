#pragma once

#include "NotRed/Core/Core.h"

namespace NR
{
    enum class AssetFlag : uint16_t
    {
        None = 0 << 0,
        Missing = 1 << 0,
        Invalid = 1 << 1
    };
    enum class AssetType
    {
        None,
        Scene,
        Prefab,
        MeshAsset,
        Mesh,
        Material,
        Texture,
        EnvMap,
        Audio,
        PhysicsMat,
        SoundConfig,
        SpatializationConfig,
        Animation,
        AnimationController,
        DemoGraph
    };

    namespace Utils
    {
        inline AssetType AssetTypeFromString(const std::string& assetType)
        {
            if (assetType == "None")                    return AssetType::None;
            if (assetType == "Scene")                   return AssetType::Scene;
            if (assetType == "Prefab")                  return AssetType::Prefab;
            if (assetType == "MeshAsset")               return AssetType::MeshAsset;
            if (assetType == "Mesh")                    return AssetType::Mesh;
            if (assetType == "Material")                return AssetType::Material;
            if (assetType == "Texture")                 return AssetType::Texture;
            if (assetType == "EnvMap")                  return AssetType::EnvMap;
            if (assetType == "Audio")                   return AssetType::Audio;
            if (assetType == "PhysicsMat")              return AssetType::PhysicsMat;
            if (assetType == "SoundConfig")             return AssetType::SoundConfig;
            if (assetType == "Animation")               return AssetType::Animation;
            if (assetType == "AnimationController")     return AssetType::AnimationController;
            if (assetType == "DemoGraph")               return AssetType::DemoGraph;

            NR_CORE_ASSERT(false, "Unknown Asset Type");
            return AssetType::None;
        }

        inline const char* AssetTypeToString(AssetType assetType)
        {
            switch (assetType)
            {
            case AssetType::None:                       return "None";
            case AssetType::Scene:                      return "Scene";
            case AssetType::Prefab:                     return "Prefab";
            case AssetType::MeshAsset:                  return "MeshAsset";
            case AssetType::Mesh:                       return "Mesh";
            case AssetType::Material:                   return "Material";
            case AssetType::Texture:                    return "Texture";
            case AssetType::EnvMap:                     return "EnvMap";
            case AssetType::Audio:                      return "Audio";
            case AssetType::PhysicsMat:                 return "PhysicsMat";
            case AssetType::SoundConfig:                return "SoundConfig";
            case AssetType::DemoGraph:                  return "DemoGraph";
            case AssetType::Animation:                  return "Animation";
            case AssetType::AnimationController:        return "AnimationController";
            default:
                NR_CORE_ASSERT(false, "Unknown Asset Type");
                return "None";
            }
        }
    }
}