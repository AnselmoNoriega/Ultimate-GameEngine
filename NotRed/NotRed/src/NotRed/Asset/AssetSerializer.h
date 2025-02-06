#pragma once

#include "AssetMetadata.h"

namespace NR
{
    class AssetSerializer
    {
    public:
        virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
        virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;
    };

    class TextureSerializer : public AssetSerializer
    {
    public:
        void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}
        bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class MeshAssetSerializer : public AssetSerializer
    {
    public:
        void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}
        bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class MaterialAssetSerializer : public AssetSerializer
    {
    public:
        virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
        virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class EnvironmentSerializer : public AssetSerializer
    {
    public:
        void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}
        bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class PhysicsMaterialSerializer : public AssetSerializer
    {
    public:
        void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
        bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class AudioFileSourceSerializer : public AssetSerializer
    {
    public:
        void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
        bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class SoundConfigSerializer : public AssetSerializer
    {
    public:
        void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
        bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class PrefabSerializer : public AssetSerializer
    {
    public:
        void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
        bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class SceneAssetSerializer : public AssetSerializer
    {
    public:
        void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
        bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class AnimationAssetSerializer : public AssetSerializer
    {
    public:
        void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}
        bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class AnimationControllerSerializer : public AssetSerializer
    {
    public:
        void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
        bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };
}