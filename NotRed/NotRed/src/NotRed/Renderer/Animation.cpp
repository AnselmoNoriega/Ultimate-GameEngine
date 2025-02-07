#include "nrpch.h" 
#include "Animation.h"

#include "NotRed/Asset/AssimpLog.h"
#include "NotRed/Asset/OZZImporterAssimp.h"
#include "NotRed/Debug/Profiler.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/span.h>

#include <filesystem>

namespace NR
{
	static const uint32_t sAnimationImportFlags =
		aiProcess_CalcTangentSpace |        // Create binormals/tangents just in case
		aiProcess_Triangulate |             // Make sure we're triangles
		aiProcess_SortByPType |             // Split meshes by primitive type
		aiProcess_GenNormals |              // Make sure we have legit normals
		aiProcess_GenUVCoords |             // Convert UVs if required 
		aiProcess_OptimizeMeshes |          // Batch draws where possible
		aiProcess_JoinIdenticalVertices |
		aiProcess_LimitBoneWeights |        // If more than N (=4) bone weights, discard least influencing bones and renormalise sum to 1
		aiProcess_ValidateDataStructure;    // Validation


	SkeletonAsset::SkeletonAsset(const std::string& filename)
		: mFilePath(filename)
	{
		LogStream::Initialize();

		NR_CORE_INFO("Loading skeleton: {0}", filename.c_str());

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filename, sAnimationImportFlags);
		ozz::animation::offline::RawSkeleton rawSkeleton;
		if (OZZImporterAssimp::ExtractRawSkeleton(scene, rawSkeleton))
		{
			ozz::animation::offline::SkeletonBuilder builder;
			mSkeleton = builder(rawSkeleton);
			if (!mSkeleton)
			{
				NR_CORE_ERROR("Failed to build runtime skeleton from file '{0}'", filename);
			}
		}
		else
		{
			NR_CORE_ERROR("No skeleton in file '{0}'", filename);
		}
	}

	bool SkeletonAsset::AreSameSkeleton(const ozz::animation::Skeleton& a, const ozz::animation::Skeleton& b)
	{
		ozz::span<const char* const> jointsA = a.joint_names();
		ozz::span<const char* const> jointsB = b.joint_names();
		bool areSame = false;
		if (jointsA.size() == jointsB.size())
		{
			areSame = true;
			for (size_t jointIndex = 0; jointIndex < jointsA.size(); ++jointIndex)
			{
				if (strcmp(jointsA[jointIndex], jointsB[jointIndex]) != 0)
				{
					areSame = false;
					break;
				}
			}
		}
		return areSame;
	}

	AnimationAsset::AnimationAsset(const std::string& filename)
		: mFilePath(filename)
	{}

	void AnimationAsset::Load(Ref<SkeletonAsset> skeleton)
	{
		LogStream::Initialize();
		NR_CORE_INFO("Loading animation: {0}", mFilePath);
		if (!skeleton || !skeleton->IsValid())
		{
			NR_CORE_ERROR("Invalid skeleton passed to animation asset for file '{0}'", mFilePath);
			ModifyFlags(AssetFlag::Invalid);
			return;
		}

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(mFilePath, sAnimationImportFlags);
		if (!scene || !scene->HasAnimations())
		{
			NR_CORE_ERROR("Failed to load animation from file '{0}'", mFilePath);
			ModifyFlags(AssetFlag::Invalid);
			return;
		}

		ozz::animation::offline::RawSkeleton rawSkeleton;
		if (OZZImporterAssimp::ExtractRawSkeleton(scene, rawSkeleton))
		{
			ozz::animation::offline::SkeletonBuilder builder;
			ozz::unique_ptr<ozz::animation::Skeleton> localSkeleton = builder(rawSkeleton);
			if (localSkeleton)
			{
				if (!SkeletonAsset::AreSameSkeleton(skeleton->GetSkeleton(), *localSkeleton))
				{
					NR_CORE_ERROR("Skeleton found in animation file '{0}' differs from expected.  All animations in an animation controller must share the same skeleton!", mFilePath);
					ModifyFlags(AssetFlag::Invalid);
					return;
				}
			}
		}

		auto animationNames = OZZImporterAssimp::GetAnimationNames(scene);
		if (animationNames.empty()) 
		{
			NR_CORE_ERROR("Failed to load animation from file: {0}", mFilePath);
			ModifyFlags(AssetFlag::Invalid);
			return;
		}

		if (animationNames.size() > 1) 
		{
			NR_CORE_WARN("Hazel currently supports only one animation per file. Only the first of {0} will be loaded from file '{1}'", animationNames.size(), mFilePath);
		}

		ozz::animation::offline::RawAnimation rawAnimation;
		aiString animationName;
		if (OZZImporterAssimp::ExtractRawAnimation(animationNames.front(), scene, skeleton->GetSkeleton(), 0.0f, rawAnimation))
		{
			if (rawAnimation.Validate()) 
			{
				ozz::animation::offline::AnimationBuilder builder;
				mAnimation = builder(rawAnimation);
				if (!mAnimation)
				{
					NR_CORE_ERROR("Failed to build runtime animation for '{}' from file '{}'", animationNames.front(), mFilePath);
					ModifyFlags(AssetFlag::Invalid);
					return;
				}
				// later, add support for "user tracks" here.  User tracks are animations of things other than joint transforms.
			}
			else
			{
				NR_CORE_ERROR("Validation failed for animation '{}' from file '{}'", animationNames.front(), mFilePath);
				ModifyFlags(AssetFlag::Invalid);
				return;
			}
		}
		else
		{
			NR_CORE_ERROR("Failed to extract animation '{}' from file '{}'", animationNames.front(), mFilePath);
			ModifyFlags(AssetFlag::Invalid);
			return;
		}

		mIsLoaded = true;
	}

	void AnimationController::Update(float dt)
	{
		NR_PROFILE_FUNC();
		// first compute animation time in range 0.0f (beginning of animation) to 1.0f (end of animation)
		// for now assume animation always loops (later this will be part of the animation controller)
		constexpr bool loop = true;

		if (mAnimationAssets[mStateIndex] && mAnimationPlaying)
		{
			mAnimationTime = mAnimationTime + dt * mTimeMultiplier / mAnimationAssets[mStateIndex]->GetAnimation().duration();
			if (loop)
			{
				mAnimationTime = mAnimationTime - floorf(mAnimationTime);
			}
			else
			{
				mAnimationTime = ozz::math::Clamp(0.0f, mAnimationTime, 1.0f);
			}

			ozz::animation::SamplingJob sampling_job;
			sampling_job.animation = &mAnimationAssets[mStateIndex]->GetAnimation();
			sampling_job.context = &mSamplingContext;
			sampling_job.ratio = mAnimationTime;
			sampling_job.output = ozz::make_span(mLocalSpaceTransforms);
			if (!sampling_job.Run())
			{
				NR_CORE_ERROR("ozz animation sampling job failed!");
			}

			ozz::animation::LocalToModelJob ltm_job;
			ltm_job.skeleton = &mSkeletonAsset->GetSkeleton();
			ltm_job.input = ozz::make_span(mLocalSpaceTransforms);
			ltm_job.output = ozz::make_span(mModelSpaceTransforms);
			if (!ltm_job.Run()) 
			{
				NR_CORE_ERROR("ozz animation convertion to model space failed!");
			}
		}
	}

	void AnimationController::SetSkeletonAsset(Ref<SkeletonAsset> skeletonAsset)
	{
		mSkeletonAsset = skeletonAsset;
		if (mSkeletonAsset && mSkeletonAsset->IsValid())
		{
			mSamplingContext.Resize(mSkeletonAsset->GetSkeleton().num_joints());
			mLocalSpaceTransforms.resize(mSkeletonAsset->GetSkeleton().num_soa_joints());
			mModelSpaceTransforms.resize(mSkeletonAsset->GetSkeleton().num_joints());
		}
	}

	void AnimationController::SetAnimationAsset(const std::string_view stateName, Ref<AnimationAsset> animationAsset)
	{
		if (mSkeletonAsset && mSkeletonAsset->IsValid() && animationAsset && animationAsset->IsValid() && !animationAsset->IsLoaded())
		{
			animationAsset->Load(mSkeletonAsset);
		}

		size_t existingIndex = ~0;
		for (size_t i = 0; i < GetNumStates(); ++i) 
		{
			if (mStateNames[i] == stateName) 
			{
				break;
			}
		}

		if (existingIndex != ~0)
		{
			mAnimationAssets[existingIndex] = animationAsset;
		}
		else
		{
			mStateNames.emplace_back(stateName);
			mAnimationAssets.emplace_back(animationAsset);
		}
	}
}