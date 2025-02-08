#pragma once

#include "NotRed/Asset/Asset.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>

namespace NR
{
	// A skeleton to which an animation clip can be applied
	// Sometimes called "Armature"
	class SkeletonAsset : public Asset
	{
	public:
		SkeletonAsset(const std::string& filename);
		virtual ~SkeletonAsset() = default;

		const std::string& GetFilePath() const { return mFilePath; }
		static AssetType GetStaticType() { return AssetType::Skeleton; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		
		const ozz::animation::Skeleton& GetSkeleton() const { NR_CORE_ASSERT(mSkeleton, "Attempted to access null skeleton!"); return *mSkeleton; }

		const glm::vec3& GetRootTranslation() const { return mRootTranslation; }
		const glm::quat& GetRootRotation() const { return mRootRotation; }
		const glm::vec3& GetRootScale() const { return mRootScale; }
		const glm::mat4 GetRootTransform() const { return mRootTransform; }
		const glm::mat4& GetInverseRootTransform() const { return mInverseRootTransform; }
	
	public:
		static bool AreSameSkeleton(const ozz::animation::Skeleton& a, const ozz::animation::Skeleton& b);
	
	private:
		glm::mat4 mRootTransform;
		glm::mat4 mInverseRootTransform;
		glm::vec3 mRootTranslation;
		glm::quat mRootRotation;
		glm::vec3 mRootScale;

		std::string mFilePath;
		
		ozz::unique_ptr<ozz::animation::Skeleton> mSkeleton;
	};

	// A single animation clip
	class AnimationAsset : public Asset
	{
	public:
		AnimationAsset(const std::string& filename);
		virtual ~AnimationAsset() = default;

		const std::string& GetFilePath() const { return mFilePath; }

		static AssetType GetStaticType() { return AssetType::Animation; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		bool IsLoaded() { return mIsLoaded; }
		void Load(Ref<SkeletonAsset> skeleton);

		const ozz::animation::Animation& GetAnimation() const { NR_CORE_ASSERT(mAnimation, "Attempted to access null animation!"); return *mAnimation; }

	private:
		ozz::unique_ptr<ozz::animation::Skeleton> mSkeleton;
		ozz::unique_ptr<ozz::animation::Animation> mAnimation;

		std::string mFilePath;
		bool mIsLoaded = false;
	};

	// Controls which animation (or animations) is playing on a mesh
	class AnimationController : public Asset
	{
	public:
		enum class RootMotionMode
		{
			Default     /* Do neither of the following*/,
			InPlace     /* If root bone is transformed by animation, remove that transformation, effectively making the animation play "in place" */,
			Apply       /* If root bone is transformed by animation, return that transformation from the OnUpdate() method.  Allowing caller to, for example, apply that transformation to the entity's transform component. */
		};

	public:
		virtual ~AnimationController() = default;

		// Updates model space matrices and returns root transform (or identity if root transform is not being applied)
		// Transform returned contains motion in the forward (Z-axis) direction only.
		// Translations in other axes, rotations, and scale are not returned.
		glm::mat4 Update(float dt);
		RootMotionMode GetRootMotionMode() const { return mRootMotionMode; }

		void SetRootMotionMode(const RootMotionMode mode) { mRootMotionMode = mode; }

		bool IsAnimationPlaying() const { return mAnimationPlaying; }
		void SetAnimationPlaying(const bool value) { mAnimationPlaying = value; }
		uint32_t GetStateIndex() { return static_cast<uint32_t>(mStateIndex); }
		void SetStateIndex(const uint32_t stateIndex);
		Ref<SkeletonAsset> GetSkeletonAsset() { return mSkeletonAsset; }
		Ref<SkeletonAsset> GetSkeletonAsset() const { return mSkeletonAsset; }
		void SetSkeletonAsset(Ref<SkeletonAsset> skeletonAsset);

		size_t GetNumStates() const { return mStateNames.size(); }
		const std::string& GetStateName(const size_t stateIndex) { return mStateNames[stateIndex]; }
		Ref<AnimationAsset> GetAnimationAsset(const size_t stateIndex) { return mAnimationAssets[stateIndex]; }
		Ref<AnimationAsset> GetAnimationAsset(const size_t stateIndex) const { return mAnimationAssets[stateIndex]; }
		void SetAnimationAsset(const std::string_view stateName, Ref<AnimationAsset> animationAsset);

		static AssetType GetStaticType() { return AssetType::AnimationController; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		const ozz::vector<ozz::math::Float4x4>& GetModelSpaceTransforms() { return mModelSpaceTransforms; }

	private:
		void SampleAnimation();
		
		glm::mat4 GetRootTransform() const;

	private:
		glm::mat4 mInvRootTransformAtStart = glm::mat4(1.0f);
		glm::mat4 mRootTransformAtEnd = glm::mat4(1.0f);
		glm::mat4 mPreviousInvRootTransform = glm::mat4(1.0f);

		ozz::vector<std::string> mStateNames;
		ozz::vector<Ref<AnimationAsset>> mAnimationAssets;
		Ref<SkeletonAsset> mSkeletonAsset;
		RootMotionMode mRootMotionMode = RootMotionMode::Apply;

		float mPreviousAnimationTime = 0.0f;
		float mAnimationTime = 0.0f;
		float mTimeMultiplier = 1.0f;
		size_t mStateIndex = ~0;
		bool mAnimationPlaying = true;

		ozz::animation::SamplingJob::Context mSamplingContext;
		ozz::vector<ozz::math::SoaTransform> mLocalSpaceTransforms;
		ozz::vector<ozz::math::Float4x4> mModelSpaceTransforms;
	};
}