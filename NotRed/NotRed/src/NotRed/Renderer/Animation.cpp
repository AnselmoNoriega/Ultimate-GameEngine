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


	AnimationAsset::AnimationAsset(const std::string& filename)
		: mFilePath(filename)
	{
		LogStream::Initialize();

		NR_CORE_INFO("Loading animation: {0}", filename.c_str());

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filename, sAnimationImportFlags);
		if (!scene || !scene->HasAnimations())
		{
			NR_CORE_ERROR("Failed to load animation from file: {0}", filename);
			ModifyFlags(AssetFlag::Invalid);
			return;
		}

		auto animationNames = OZZImporterAssimp::GetAnimationNames(scene);
		if (animationNames.empty()) {
			// shouldnt ever get here, since we already checked scene.HasAnimations()...
			NR_CORE_ERROR("Failed to load animation from file: {0}", filename);
			ModifyFlags(AssetFlag::Invalid);
			return;
		}

		ozz::animation::offline::RawSkeleton rawSkeleton;
		if (OZZImporterAssimp::ExtractRawSkeleton(scene, rawSkeleton))
		{
			ozz::animation::offline::SkeletonBuilder builder;
			mSkeleton = builder(rawSkeleton);
			if (!mSkeleton)
			{
				NR_CORE_ERROR("Failed to build runtime skeleton from mesh file '{0}'", filename);
			}
		}
		else
		{
			NR_CORE_ERROR("No skeleton in animation file '{0}'", filename);
		}

		if (mSkeleton)
		{
			ozz::animation::offline::RawAnimation rawAnimation;
			aiString animationName;
			if (OZZImporterAssimp::ExtractRawAnimation(animationNames.front(), scene, *mSkeleton, 0.0f, rawAnimation))
			{
				if (rawAnimation.Validate()) 
				{
					ozz::animation::offline::AnimationBuilder builder;
					mAnimation = builder(rawAnimation);
					if (!mAnimation)
					{
						NR_CORE_ERROR("Failed to build runtime animation for '{}' from mesh file '{}'", animationNames.front(), filename);
					}
				}
				else
				{
					NR_CORE_ERROR("Validation failed for animation '{}' from mesh file '{}'", animationNames.front(), filename);
				}
			}
			else
			{
				NR_CORE_ERROR("Failed to extract animation '{}' from mesh file '{}'", animationNames.front(), filename);
			}
		}
	}

	AnimationController::AnimationController(Ref<AnimationAsset> animationAsset)
		: mAnimationAsset(animationAsset)
	{
		if (mAnimationAsset)
		{
			mSamplingContext.Resize(mAnimationAsset->GetSkeleton().num_joints());
			mLocalSpaceTransforms.resize(mAnimationAsset->GetSkeleton().num_soa_joints());
			mModelSpaceTransforms.resize(mAnimationAsset->GetSkeleton().num_joints());
		}
	}

	AnimationController::AnimationController(const Ref<AnimationController>& other)
		: mAnimationAsset(other->mAnimationAsset)
	{
		if (mAnimationAsset)
		{
			mSamplingContext.Resize(mAnimationAsset->GetSkeleton().num_joints());
			mLocalSpaceTransforms.resize(mAnimationAsset->GetSkeleton().num_soa_joints());
			mModelSpaceTransforms.resize(mAnimationAsset->GetSkeleton().num_joints());
		}
	}

	void AnimationController::Update(float dt)
	{
		NR_PROFILE_FUNC();
		// first compute animation time in range 0.0f (beginning of animation) to 1.0f (end of animation)
		// for now assume animation always loops (later this will be part of the animation controller)
		const bool loop = true;

		if (mAnimationAsset && mAnimationPlaying)
		{
			mAnimationTime = mAnimationTime + dt * mTimeMultiplier / mAnimationAsset->GetAnimation().duration();
			if (loop)
			{
				// Wraps the unit interval [0:1], even for negative values (the reason for using floorf).
				mAnimationTime = mAnimationTime - floorf(mAnimationTime);
			}
			else
			{
				mAnimationTime = ozz::math::Clamp(0.0f, mAnimationTime, 1.0f);
			}

			ozz::animation::SamplingJob sampling_job;
			sampling_job.animation = &mAnimationAsset->GetAnimation();
			sampling_job.context = &mSamplingContext;
			sampling_job.ratio = mAnimationTime;
			sampling_job.output = ozz::make_span(mLocalSpaceTransforms);
			if (!sampling_job.Run())
			{
				NR_CORE_ERROR("ozz animation sampling job failed!");
			}

			ozz::animation::LocalToModelJob ltmjob;
			ltmjob.skeleton = &mAnimationAsset->GetSkeleton();
			ltmjob.input = ozz::make_span(mLocalSpaceTransforms);
			ltmjob.output = ozz::make_span(mModelSpaceTransforms);
			if (!ltmjob.Run()) {
				NR_CORE_ERROR("ozz animation convertion to model space failed!");
			}
		}
	}

	void AnimationController::SetAnimationAsset(Ref<AnimationAsset> animationAsset)
	{
		mAnimationAsset = animationAsset;
		if (mAnimationAsset)
		{
			mSamplingContext.Resize(mAnimationAsset->GetSkeleton().num_joints());
			mLocalSpaceTransforms.resize(mAnimationAsset->GetSkeleton().num_soa_joints());
			mModelSpaceTransforms.resize(mAnimationAsset->GetSkeleton().num_joints());
		}
	}
}