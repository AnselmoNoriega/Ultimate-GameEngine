#pragma once

#include "NotRed/Asset/Asset.h"

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
	
	public:
		static bool AreSameSkeleton(const ozz::animation::Skeleton& a, const ozz::animation::Skeleton& b);
	
	private:
		ozz::unique_ptr<ozz::animation::Skeleton> mSkeleton;
		std::string mFilePath;
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
		virtual ~AnimationController() = default;

		void Update(float dt);

		bool IsAnimationPlaying() const { return mAnimationPlaying; }
		void SetAnimationPlaying(const bool value) { mAnimationPlaying = value; }
		uint32_t GetStateIndex() { return static_cast<uint32_t>(mStateIndex); }
		void SetStateIndex(const uint32_t stateIndex) { mStateIndex = stateIndex; }
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
		ozz::vector<std::string> mStateNames;
		ozz::vector<Ref<AnimationAsset>> mAnimationAssets;
		Ref<SkeletonAsset> mSkeletonAsset;

		float mAnimationTime = 0.0f;
		float mTimeMultiplier = 1.0f;
		size_t mStateIndex = 0;
		bool mAnimationPlaying = true;

		ozz::animation::SamplingJob::Context mSamplingContext;
		ozz::vector<ozz::math::SoaTransform> mLocalSpaceTransforms;
		ozz::vector<ozz::math::Float4x4> mModelSpaceTransforms;
	};
}