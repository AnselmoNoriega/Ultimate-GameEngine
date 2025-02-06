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
	// A single animation clip
	class AnimationAsset : public Asset
	{
	public:
		AnimationAsset(const std::string& filename);
		virtual ~AnimationAsset() = default;

		const std::string& GetFilePath() const { return mFilePath; }

		static AssetType GetStaticType() { return AssetType::Animation; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		const ozz::animation::Skeleton& GetSkeleton() const { NR_CORE_ASSERT(mSkeleton, "Attempted to access null skeleton!"); return *mSkeleton; }
		const ozz::animation::Animation& GetAnimation() const { NR_CORE_ASSERT(mAnimation, "Attempted to access null animation!"); return *mAnimation; }

	private:
		ozz::unique_ptr<ozz::animation::Skeleton> mSkeleton;
		ozz::unique_ptr<ozz::animation::Animation> mAnimation;

		std::string mFilePath;
	};


	// Controls which animation (or animations) is playing on a mesh
	class AnimationController : public Asset
	{
	public:
		AnimationController(Ref<AnimationAsset> animationAsset);
		AnimationController(const Ref<AnimationController>& other);
		virtual ~AnimationController() = default;

		void Update(float dt);

		bool IsAnimationPlaying() { return mAnimationPlaying; }
		void SetAnimationPlaying(bool value) { mAnimationPlaying = value; }

		Ref<AnimationAsset> GetAnimationAsset() { return mAnimationAsset; }
		Ref<AnimationAsset> GetAnimationAsset() const { return mAnimationAsset; }
		void SetAnimationAsset(Ref<AnimationAsset> animationAsset);

		static AssetType GetStaticType() { return AssetType::AnimationController; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		ozz::span<const char* const> GetJointNames() { return mAnimationAsset ? mAnimationAsset->GetSkeleton().joint_names() : ozz::span<const char* const>(); }

		const ozz::vector<ozz::math::Float4x4>& GetModelSpaceTransforms() { return mModelSpaceTransforms; }

	private:
		Ref<AnimationAsset> mAnimationAsset;  // Only one animation for now.  Later there will be a bunch of different ones, comprising the "states" of the animation controller

		// Animation
		float mAnimationTime = 0.0f;
		float mTimeMultiplier = 1.0f;
		bool mAnimationPlaying = true;

		ozz::animation::SamplingJob::Context mSamplingContext;
		ozz::vector<ozz::math::SoaTransform> mLocalSpaceTransforms;
		ozz::vector<ozz::math::Float4x4> mModelSpaceTransforms;
	};
}