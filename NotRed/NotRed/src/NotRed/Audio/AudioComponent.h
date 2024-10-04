#pragma once

#include "Sound.h"

namespace NR::Audio
{
	struct SoundSourceUpdateData
	{
		uint64_t entityID;

		float VolumeMultiplier;
		float PitchMultiplier;

		glm::vec3 Position;
		glm::vec3 Velocity;
	};

	struct AudioComponent
	{
		UUID ParentHandle;

		SoundConfig SoundConfig;

		bool PlayOnAwake = false;

		float VolumeMultiplier = 1.0f;
		float PitchMultiplier = 1.0f;

		glm::vec3 SourcePosition = { 0.0f, 0.0f, 0.0f };

		bool AutoDestroy = false;

		std::atomic<bool> MarkedForDestroy = false;

		AudioComponent() = default;

		AudioComponent(const AudioComponent& other)
			:ParentHandle(other.ParentHandle), SoundConfig(other.SoundConfig)
			, AutoDestroy(other.AutoDestroy), PlayOnAwake(other.PlayOnAwake)
			, VolumeMultiplier(other.VolumeMultiplier), PitchMultiplier(other.PitchMultiplier)
			, SourcePosition(other.SourcePosition)
		{
			MarkedForDestroy = other.MarkedForDestroy.load();
		}

		AudioComponent& operator=(const AudioComponent& other)
		{
			ParentHandle = other.ParentHandle;
			SoundConfig = other.SoundConfig;
			AutoDestroy = other.AutoDestroy;
			PlayOnAwake = other.PlayOnAwake;
			VolumeMultiplier = other.VolumeMultiplier;
			PitchMultiplier = other.PitchMultiplier;
			SourcePosition = other.SourcePosition;
			MarkedForDestroy = other.MarkedForDestroy.load();
			return *this;
		}

		AudioComponent(UUID parent)
			: ParentHandle(parent) {}
	};
}