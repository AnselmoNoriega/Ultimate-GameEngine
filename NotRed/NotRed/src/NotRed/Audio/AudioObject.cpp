#include <nrpch.h>
#include "AudioObject.h"

namespace NR
{
	AudioObject::AudioObject(UUID ID) : mID(ID) {}
	AudioObject::AudioObject(UUID ID, const std::string& debugName) : mID(ID), mDebugName(debugName) {}

	AudioObject::AudioObject(const AudioObject&& other) noexcept
		: mID(other.mID)
		, mTransform(other.mTransform)
		, mDebugName(other.mDebugName)
		, mReleased(other.mReleased)
	{ }

	AudioObject& AudioObject::operator=(const AudioObject&& other) noexcept
	{
		mID = other.mID;
		mTransform = other.mTransform;
		mDebugName = other.mDebugName;
		mReleased = other.mReleased;
		return *this;
	};

	void AudioObject::SetTransform(const Audio::Transform& transform)
	{
		mTransform = transform;
	}

	void AudioObject::SetVelocity(const glm::vec3& velocity)
	{
		mVelocity = velocity;
	}

} // namespace