#pragma once

#include "Sound.h"
#include "AudioEvents/CommandID.h"

namespace NR
{
	struct SoundSourceUpdateData
	{
		uint64_t entityID;

		Audio::Transform Transform;
		glm::vec3 Velocity;

		float VolumeMultiplier;
		float PitchMultiplier;

		glm::vec3 Position;
		glm::vec3 Velocity;
	};

	/*  ======================================================================
		Entity Component, contains data to initialize playback instances
		Playback of Sound Sources controlled via association with AudioComponent
		----------------------------------------------------------------------
	*/
	struct AudioComponent
	{
		UUID ParentHandle;

		std::string StartEvent;
		Audio::CommandID StartCommandID; // Internal

		bool PlayOnAwake = false;
		bool StopIfEntityDestroyed = true;

		float VolumeMultiplier = 1.0f;
		float PitchMultiplier = 1.0f;

		bool AutoDestroy = false;

		std::atomic<bool> MarkedForDestroy = false;

		AudioComponent() = default;

		AudioComponent(const AudioComponent& other)
			:ParentHandle(other.ParentHandle), StartEvent(other.StartEvent), StartCommandID(other.StartCommandID)
			, AutoDestroy(other.AutoDestroy), StopIfEntityDestroyed(other.StopIfEntityDestroyed), PlayOnAwake(other.PlayOnAwake)
			, VolumeMultiplier(other.VolumeMultiplier), PitchMultiplier(other.PitchMultiplier)
		{
			MarkedForDestroy = other.MarkedForDestroy.load();
		}

		AudioComponent& operator=(const AudioComponent& other)
		{
			ParentHandle = other.ParentHandle;
			StartEvent = other.StartEvent;
			StartCommandID = other.StartCommandID;
			AutoDestroy = other.AutoDestroy;
			StopIfEntityDestroyed = other.StopIfEntityDestroyed;
			PlayOnAwake = other.PlayOnAwake;
			VolumeMultiplier = other.VolumeMultiplier;
			PitchMultiplier = other.PitchMultiplier;
			MarkedForDestroy = other.MarkedForDestroy.load();
			return *this;
		}

		AudioComponent(UUID parent)
			: ParentHandle(parent) {}
	};
}