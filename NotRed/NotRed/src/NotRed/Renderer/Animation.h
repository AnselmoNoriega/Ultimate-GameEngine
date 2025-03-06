#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>

#include "NotRed/Asset/Asset.h"

namespace NR
{
	// Represents the position and orientation (in model-space) of the root joint of a skeleton.
	// We are not interested in scale.
	struct RootPose {
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };        // aka. Position
		glm::quat Rotation = glm::identity<glm::quat>();     // aka. Orientation
	};

	// Represents the motion (i.e. difference between current pose and a previous one) of the root joint of a skeleton.
	// Only translation and rotation for now (ignore scale).
	using RootMotion = RootPose;


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

		// Overall transform (rotation and scale only) applied to entire skeleton.
		// e.g. if armature was exported from Blender with a non 1.0 scale factor
		const glm::mat3 GetSkeletonTransform() const { return mSkeletonTransform; }

	public:
		static bool AreSameSkeleton(const ozz::animation::Skeleton& a, const ozz::animation::Skeleton& b);

	private:
		glm::mat3 mSkeletonTransform;
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

		// These functions are needed because there is currently no way, via the AssetManager's
		// "TryLoadData" mechanism, to pass parameters other than filename to the constructor.
		bool IsLoaded() { return mIsLoaded; }
		void Load(Ref<SkeletonAsset> skeleton);

		const ozz::animation::Animation& GetAnimation() const { NR_CORE_ASSERT(mAnimation, "Attempted to access null animation!"); return *mAnimation; }

	private:
		ozz::unique_ptr<ozz::animation::Animation> mAnimation;
		std::string mFilePath;
		bool mIsLoaded = false;
	};


	// AnimationState references an animation clip, plus stores extra meta data about that clip
	// such as how we want to extract root motion, looping, etc.
	class AnimationState : public RefCounted
	{
	public:
		virtual ~AnimationState() = default;

		Ref<AnimationAsset> GetAnimationAsset() const { return mAnimationAsset; }
		void SetAnimationAsset(Ref<AnimationAsset> animation) { mAnimationAsset = animation; }

		// Mask can be used to "zero out" some components of root bone transforms so that animations play "in place"
		// So Translation Mask = {1, 1, 0} means that root bone motion in Z axis will be zeroed out.  The animation will not move the character in Z-axis
		// For rotation, only rotation about up-axis (Y) can be zeroed.
		// RootRotationMask = 0 means animation will not rotate the character about Y-axis.  1 means that it will.
		//
		// Usually, you will want the mask here to be the opposite of the Extract Mask (see below).
		// However, it is sometimes useful to zero out components of motion that you are not extracting. (i.e. have them both set to zeros)
		const glm::vec3& GetRootTranslationMask() const { return mRootTranslationMask; }
		void SetRootTranslationMask(const glm::vec3& mask) { mRootTranslationMask = mask; }
		const float GetRootRotationMask() const { return mRootRotationMask; }
		void SetRootRotationMask(const float mask) { mRootRotationMask = mask; }

		// Extract Mask can be used to control which components of the root bone transforms are extracted as "root motion" so
		// that they can be applied to a root motion target.
		// So Translation Extract Mask = {0, 0, 1} means that if the animation moves the character in the Z-axis, that movement will be extracted as "root motion".
		// For rotation, only rotation about up-axis (Y) can be extracted.  RootRotationExtractMask = 1 means extract rotation about Y-axis, 0 means do not.
		const glm::vec3& GetRootTranslationExtractMask() const { return mRootTranslationExtractMask; }
		void SetRootTranslationExtractMask(const glm::vec3& mask) { mRootTranslationExtractMask = mask; }
		const float GetRootRotationExtractMask() const { return mRootRotationExtractMask; }
		void SetRootRotationExtractMask(const float mask) { mRootRotationExtractMask = mask; }

		bool IsLooping() const { return mIsLooping; }
		void SetIsLooping(const bool b) { mIsLooping = b; }

		float GetPlaybackSpeed() const { return mPlaybackSpeed; }
		void SetPlaybackSpeed(const float f) { mPlaybackSpeed = f; }

	private:
		Ref<AnimationAsset> mAnimationAsset;
		glm::vec3 mRootTranslationMask = { 1.0f, 1.0f, 0.0f };         // default is to apply root bone transforms in X and Y only. (Z is extracted as root motion)
		glm::vec3 mRootTranslationExtractMask = { 0.0f, 0.0f, 1.0f };  // default is to extract root bone transform in forwards (Z) axis
		float mRootRotationMask = 1.0f;                                // default is to apply root bone rotation (Y-axis)
		float mRootRotationExtractMask = 0.0f;                         // default is to not extract root bone rotation

		float mPlaybackSpeed = 1.0;
		bool mIsLooping = true;
	};


	// Controls which animation (or animations) is playing on a mesh.
	class AnimationController : public Asset
	{
	public:
		virtual ~AnimationController() = default;

		// Updates model space matrices (aka bone transforms) and returns root motion (will be identity if root motion is not being extracted)
		const RootMotion& Update(float dt, bool extractRootMotion);

		// animation time is a ratio. 0.0 = the start of the animation (for current state),  1.0 = the end of the animation (for current state)
		float GetAnimationTime() const { return mAnimationTime; }
		void SetAnimationTime(const float time) { mAnimationTime = time; }

		bool IsAnimationPlaying() const { return mAnimationPlaying; }
		void SetAnimationPlaying(const bool value) { mAnimationPlaying = value; }

		uint32_t GetStateIndex() { return static_cast<uint32_t>(mStateIndex); }
		void SetStateIndex(const uint32_t stateIndex);

		Ref<SkeletonAsset> GetSkeletonAsset() { return mSkeletonAsset; }
		Ref<SkeletonAsset> GetSkeletonAsset() const { return mSkeletonAsset; }
		void SetSkeletonAsset(Ref<SkeletonAsset> skeletonAsset);

		size_t GetNumStates() const { return mStateNames.size(); }
		const std::string& GetStateName(const size_t stateIndex) { return mStateNames[stateIndex]; }

		Ref<AnimationState> GetAnimationState(const size_t stateIndex) { return mAnimationStates[stateIndex]; }
		Ref<AnimationState> GetAnimationState(const size_t stateIndex) const { return mAnimationStates[stateIndex]; }
		void SetAnimationState(const std::string_view stateName, Ref<AnimationState> state);

		// Returns the motion of root bone (aka joint 0)
		// This is the difference in pose between the most recent call to SampleAnimation() and the one before that.
		// Only the components of root motion per the root motion mask of the current animation state are returned.
		const RootMotion& GetRootMotion() const { return mRootMotion; }

		// Returns the current pose of root bone
		// (e.g. allows for "debugging" animations via imgui widgets)
		const RootPose& GetRootPose() const { return mRootPose; }

		static AssetType GetStaticType() { return AssetType::AnimationController; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		// The bone transforms resulting from a call to OnUpdate()
		// These are the ones to use for mesh skinning
		const ozz::vector<ozz::math::Float4x4>& GetModelSpaceTransforms() { return mModelSpaceTransforms; }

	private:
		// sample current animation clip (aka "state") at current animation time
		void SampleAnimation();

		// Extract the pose of root joint as computed by a call to SampleAnimation()
		RootPose ExtractRootPose() const;

	private:
		ozz::animation::SamplingJob::Context mSamplingContext;
		ozz::vector<ozz::math::SoaTransform> mLocalSpaceTransforms;
		ozz::vector<ozz::math::Float4x4> mModelSpaceTransforms;

		RootPose mRootPoseStart; // root pose at start of animation (for current state)
		RootPose mRootPoseEnd;   // root pose and end of animation (for current state)
		RootPose mRootPose;      // last sampled root pose
		RootMotion mRootMotion;  // "motion" of root (i.e the difference between current sampled root pose and previous sampled root pose)

		ozz::vector<std::string> mStateNames;
		ozz::vector<Ref<AnimationState>> mAnimationStates;

		Ref<SkeletonAsset> mSkeletonAsset;

		float mPreviousAnimationTime = -FLT_MAX;
		float mAnimationTime = 0.0f;
		size_t mStateIndex = ~0;
		bool mAnimationPlaying = true;
	};
}