#include "nrpch.h" 
#include "Animation.h"

#include "NotRed/Asset/AssimpLog.h"
#include "NotRed/Asset/OZZImporterAssimp.h"
#include "NotRed/Debug/Profiler.h"

#include <glm/gtc/type_ptr.hpp>

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

	glm::mat4 Mat4FromFloat4x4(const ozz::math::Float4x4& float4x4)
	{
		glm::mat4 result;
		ozz::math::StorePtr(float4x4.cols[0], glm::value_ptr(result[0]));
		ozz::math::StorePtr(float4x4.cols[1], glm::value_ptr(result[1]));
		ozz::math::StorePtr(float4x4.cols[2], glm::value_ptr(result[2]));
		ozz::math::StorePtr(float4x4.cols[3], glm::value_ptr(result[3]));
		return result;
	}

	SkeletonAsset::SkeletonAsset(const std::string& filename)
		: mFilePath(filename)
	{
		LogStream::Initialize();

		NR_CORE_INFO("Loading skeleton: {0}", filename.c_str());

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filename, sAnimationImportFlags);
		ozz::animation::offline::RawSkeleton rawSkeleton;
		ozz::math::Float4x4 transform;
		if (OZZImporterAssimp::ExtractRawSkeleton(scene, rawSkeleton, transform))
		{
			mSkeletonTransform = Mat4FromFloat4x4(transform);
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
		ozz::math::Float4x4 skeletonTransform;
		if (OZZImporterAssimp::ExtractRawSkeleton(scene, rawSkeleton, skeletonTransform))
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

	glm::mat4 AnimationController::Update(float dt)
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

			SampleAnimation();
			if (mRootMotionMode != RootMotionMode::Default)
			{
				// extract root transform so we can fiddle with it...
				ozz::math::SoaTransform soaTransform = mLocalSpaceTransforms[0];
				ozz::math::SoaFloat3 soaTranslation = soaTransform.translation;
				ozz::math::SoaQuaternion soaRotation = soaTransform.rotation;
				ozz::math::SoaFloat3 soaScale = soaTransform.scale;

				const glm::vec3& rootTranslation = GetSkeletonAsset()->GetRootTranslation();
				soaTranslation = ozz::math::SoaFloat3::Load(
					ozz::math::simd_float4::Load(ozz::math::GetX(soaTranslation.x), ozz::math::GetY(soaTranslation.x), ozz::math::GetZ(soaTranslation.x), ozz::math::GetW(soaTranslation.x)),
					ozz::math::simd_float4::Load(ozz::math::GetX(soaTranslation.y), ozz::math::GetY(soaTranslation.y), ozz::math::GetZ(soaTranslation.y), ozz::math::GetW(soaTranslation.y)),
					ozz::math::simd_float4::Load(rootTranslation.z, ozz::math::GetY(soaTranslation.z), ozz::math::GetZ(soaTranslation.z), ozz::math::GetW(soaTranslation.z))
				);

				mLocalSpaceTransforms[0] = {
					soaTranslation,
					soaRotation,
					soaScale
				};
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

		mPreviousAnimationTime = mAnimationTime;
		return mRootMotionMode == RootMotionMode::Apply ? glm::translate(glm::mat4(1.0f), mRootMotion) : glm::mat4(1.0f);
	}

	void AnimationController::SetStateIndex(const uint32_t stateIndex)
	{
		if (stateIndex != mStateIndex) 
		{
			mPreviousAnimationTime = 0.0f;
			mAnimationTime = 1.0f;
			mStateIndex = stateIndex;
			SampleAnimation();
			mRootTransformAtEnd = GetRootTransform();

			mAnimationTime = 0.0;
			SampleAnimation();
			mInvRootTransformAtStart = glm::inverse(GetRootTransform());
			mPreviousInvRootTransform = mInvRootTransformAtStart;
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
		if (mStateIndex == ~0)
		{
			SetStateIndex(0);
		}
	}

	void AnimationController::SampleAnimation()
	{
		ozz::animation::SamplingJob sampling_job;
		sampling_job.animation = &mAnimationAssets[mStateIndex]->GetAnimation();
		sampling_job.context = &mSamplingContext;
		sampling_job.ratio = mAnimationTime;
		sampling_job.output = ozz::make_span(mLocalSpaceTransforms);

		if (!sampling_job.Run())
		{
			NR_CORE_ERROR("ozz animation sampling job failed!");
		}

		// Work out how far the root (joint 0) moved since previous call to SampleAnimation()
		// (bearing in mind that we might have looped back around to the beginning).
		glm::mat4 rootTransform = GetRootTransform();
		glm::mat4 rootMotion;
		if (mAnimationTime > mPreviousAnimationTime)
		{
			rootMotion = rootTransform * mPreviousInvRootTransform;
		}
		else
		{
			rootMotion = (mRootTransformAtEnd * mPreviousInvRootTransform) * (rootTransform * mInvRootTransformAtStart);
		}

		// Return only motion in forward axis (Z).
		// Must scale by skeletonTransform
		mRootMotion = mSkeletonAsset->GetSkeletonTransform() * glm::vec4{ 0.0f, 0.0f, rootMotion[3][2], 0.0f };
		mPreviousInvRootTransform = glm::inverse(rootTransform);
	}

	glm::mat4 AnimationController::GetRootTransform() const
	{
		// Extract the [0]th transform from ozz soa structs.
		// The skeleton is ordered depth first, so we know [0]th joint is the "root"
		// (there could be more than one root, but for now we just care about the the first one)
		// We're extracting from local-space transform, but since it's a root, that's the same thing as model-space.
		// (and we don't necessarily have the model-space transforms yet)
		ozz::math::SoaTransform soaTransform = mLocalSpaceTransforms[0];
		ozz::math::SoaFloat3 soaTranslation = soaTransform.translation;
		ozz::math::SoaQuaternion soaRotation = soaTransform.rotation;
		ozz::math::SoaFloat3 soaScale = soaTransform.scale;

		glm::vec3 translation = { ozz::math::GetX(soaTranslation.x), ozz::math::GetX(soaTranslation.y), ozz::math::GetX(soaTranslation.z) };
		glm::quat rotation = { ozz::math::GetX(soaRotation.w), ozz::math::GetX(soaRotation.x), ozz::math::GetX(soaRotation.y), ozz::math::GetX(soaRotation.z) };
		glm::vec3 scale = { ozz::math::GetX(soaScale.x), ozz::math::GetX(soaScale.y), ozz::math::GetX(soaScale.z) };

		return glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
	}
}