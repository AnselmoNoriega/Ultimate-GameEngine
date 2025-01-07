#pragma once

#include "Audio.h"

#include "NotRed/Core/UUID.h"

#include "glm/glm.hpp"

namespace NR
{
	class AudioObject : public RefCounted
	{
	public:
		friend class MiniAudioEngine;

		AudioObject(UUID ID);
		AudioObject(UUID ID, const std::string& debugName);

		AudioObject(const AudioObject&& other) noexcept;
		AudioObject() = default;
		AudioObject(const AudioObject&) = delete;
		AudioObject& operator=(const AudioObject&) = default;
		AudioObject& operator=(const AudioObject&& other) noexcept;

	public:
		Audio::Transform GetTransform() const { return mTransform; }
		glm::vec3 GetVelocity() const { return mVelocity; }

		void SetTransform(const Audio::Transform& transform);
		void SetVelocity(const glm::vec3& velocity);

		UUID GetID() const { return mID; }
		std::string GetDebugName() const { return mDebugName; }

	private:
		UUID mID;
		Audio::Transform mTransform;
		glm::vec3 mVelocity;
		std::string mDebugName;
		bool mReleased = false;
	};
} // namespace