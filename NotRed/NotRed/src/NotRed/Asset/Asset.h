#pragma once

#include "NotRed/Asset/AssetTypes.h"

#include "NotRed/Core/UUID.h"

namespace NR
{
	using AssetHandle = UUID;

	class Asset : public RefCounted
	{
	public:
		AssetHandle Handle;
		uint16_t Flags = (uint16_t)AssetFlag::None;

		virtual ~Asset() = default;
		virtual AssetType GetAssetType() const { return AssetType::Other; }

		virtual bool operator==(const Asset& other) const
		{
			return Handle == other.Handle;
		}

		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}

		bool IsFlagSet(AssetFlag flag) const { return (uint16_t)flag & Flags; }
		void ModifyFlags(AssetFlag flag, bool add = true)
		{
			if (add)
			{
				Flags |= (uint16_t)flag;
			}
			else
			{
				Flags &= ~(uint16_t)flag;
			}
		}
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

		static AssetType GetStaticType() { return AssetType::PhysicsMat; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};

	class AudioFile : public Asset
	{
	public:
		double Duration;
		uint32_t SamplingRate;
		uint16_t BitDepth;
		uint16_t NumChannels;
		uint64_t FileSize;

		AudioFile() = default;
		AudioFile(double duration, uint32_t samplingRate, uint16_t bitDepth, uint16_t numChannels, uint64_t fileSize)
			: Duration(duration), SamplingRate(samplingRate), BitDepth(bitDepth), NumChannels(numChannels), FileSize(fileSize)
		{}

		static AssetType GetStaticType() { return AssetType::Audio; }
		virtual AssetType GetAssetType() const override { return AssetType::Audio; }
	};
}