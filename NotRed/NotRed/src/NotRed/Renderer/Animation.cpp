#include "nrpch.h" 
#include "Animation.h"

#include "NotRed/Asset/AssimpLog.h"
#include "NotRed/Asset/OZZImporterAssimp.h"
#include "NotRed/Debug/Profiler.h"
#include "NotRed/Math/Math.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include <glm/gtc/type_ptr.hpp>

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
		//		aiProcess_OptimizeGraph |
		aiProcess_OptimizeMeshes |          // Batch draws where possible
		aiProcess_JoinIdenticalVertices |
		aiProcess_LimitBoneWeights |        // If more than N (=4) bone weights, discard least influencing bones and renormalise sum to 1
		aiProcess_GlobalScale |             // e.g. convert cm to m for fbx import (and other formats where cm is native)
		//		aiProcess_PopulateArmatureData |    // not currently using this data
		aiProcess_ValidateDataStructure;    // Validation 

	namespace Utils 
	{
		glm::mat4 Mat4FromFloat4x4(const ozz::math::Float4x4& float4x4);
	}

	SkeletonAsset::SkeletonAsset(const std::string& filename)
		: mFilePath(filename)
	{
		LogStream::Initialize();

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
		if (animationNames.empty()) {
			// shouldnt ever get here, since we already checked scene.HasAnimations()...
			NR_CORE_ERROR("Failed to load animation from file: {0}", mFilePath);
			ModifyFlags(AssetFlag::Invalid);
			return;
		}

		if (animationNames.size() > 1) {
			NR_CORE_WARN("NotRed currently supports only one animation per file. Only the first of {0} will be loaded from file '{1}'", animationNames.size(), mFilePath);
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


	float AngleAroundYAxis(const glm::quat& quat)
	{
		static glm::vec3 xAxis = { 1.0f, 0.0f, 0.0f };
		static glm::vec3 yAxis = { 0.0f, 1.0f, 0.0f };
		auto rotatedOrthogonal = quat * xAxis;
		auto projected = glm::normalize(rotatedOrthogonal - (yAxis * glm::dot(rotatedOrthogonal, yAxis)));
		return acos(glm::dot(xAxis, projected));
	}


	const RootMotion& AnimationController::Update(float dt, bool isRootMotionEnabled)
	{
		NR_PROFILE_FUNC();
		auto state = mAnimationStates[mStateIndex];
		if (state && state->GetAnimationAsset() && mAnimationPlaying)
		{
			mAnimationTime = mAnimationTime + dt * state->GetPlaybackSpeed() / state->GetAnimationAsset()->GetAnimation().duration();
			if (state->IsLooping())
			{
				// Wraps the unit interval [0:1], even for negative values (the reason for using floorf).
				mAnimationTime = mAnimationTime - floorf(mAnimationTime);
			}
			else
			{
				mAnimationTime = ozz::math::Clamp(0.0f, mAnimationTime, 1.0f);
			}
		}

		// Why is mRootMotion a member variable, rather than just having this function return a local value?
		// The answer is so that clients (e.g. script components) can query what the root motion was later,
		// and independently of what the caller of this function may (or may not) do with the returned
		// root motion.
		mRootMotion = {};

		if (mAnimationTime != mPreviousAnimationTime)
		{
			SampleAnimation();

			if (isRootMotionEnabled)
			{
				// Need to remove root motion from the sampled bone transforms.
				// Otherwise it potentially gets applied twice (once when caller of this function "applies" the returned
				// root motion, and once again by the bone transforms).

				// Extract root transform so we can fiddle with it...
				glm::vec3 translation = mLocalTranslations[0];
				glm::quat rotation = mLocalRotations[0];
				glm::mat4 rootTransform = glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(glm::quat(rotation));

				// remove components of root transform, based on root motion extract mask
				translation = state->GetRootTranslationMask() * translation;
				if (state->GetRootRotationMask() > 0.0f)
				{
					auto angleY = AngleAroundYAxis(rotation);
					rotation = glm::quat(cos(angleY * 0.5f), glm::vec3{ 0.0f, 1.0f, 0.0f } *sin(angleY * 0.5f));
				}
				else
				{
					rotation = glm::identity<glm::quat>();
				}
				glm::mat4 extractTransform = glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(glm::quat(rotation));
				rootTransform = glm::inverse(extractTransform) * rootTransform;
				translation = rootTransform[3];
				rotation = glm::quat_cast(rootTransform);

				// put the modified root transform back into the bone transforms
				mLocalTranslations[0] = translation;
				mLocalRotations[0] = rotation;
			}

			mPreviousAnimationTime = mAnimationTime;
		}

		return mRootMotion;
	}


	void AnimationController::SetStateIndex(const uint32_t stateIndex)
	{
		if (stateIndex != mStateIndex) {
			mPreviousAnimationTime = 0.0f;
			mAnimationTime = 1.0f;
			mStateIndex = stateIndex;

			SampleAnimation();
			mRootPoseEnd = { mLocalTranslations[0], mLocalRotations[0] };

			mAnimationTime = 0.0;
			SampleAnimation();
			mRootPoseStart = { mLocalTranslations[0], mLocalRotations[0] };
			mRootPose = mRootPoseStart;
		}
	}


	void AnimationController::SetSkeletonAsset(Ref<SkeletonAsset> skeletonAsset)
	{
		mSkeletonAsset = skeletonAsset;
		if (mSkeletonAsset && mSkeletonAsset->IsValid())
		{
			mSamplingContext.Resize(mSkeletonAsset->GetSkeleton().num_joints());
			mLocalSpaceSoaTransforms.resize(mSkeletonAsset->GetSkeleton().num_soa_joints());
			mLocalTranslations.resize(mSkeletonAsset->GetSkeleton().num_joints());
			mLocalScales.resize(mSkeletonAsset->GetSkeleton().num_joints());
			mLocalRotations.resize(mSkeletonAsset->GetSkeleton().num_joints());
		}
	}


	void AnimationController::SetAnimationState(const std::string_view stateName, Ref<AnimationState> state)
	{
		// Load animation if it isn't already (we could not do this when animation asset was first de-serialized, since there is no way to pass parameters to the asset manager)
		if (mSkeletonAsset && mSkeletonAsset->IsValid() && state && state->GetAnimationAsset() && state->GetAnimationAsset()->IsValid() && !state->GetAnimationAsset()->IsLoaded())
		{
			state->GetAnimationAsset()->Load(mSkeletonAsset);
		}

		size_t existingIndex = ~0;
		for (size_t i = 0; i < GetNumStates(); ++i) if (mStateNames[i] == stateName) break;

		if (existingIndex != ~0)
		{
			mAnimationStates[existingIndex] = state;
		}
		else
		{
			mStateNames.emplace_back(stateName);
			mAnimationStates.emplace_back(state);
		}
		if (mStateIndex == ~0)
		{
			SetStateIndex(0);
		}
	}


	void AnimationController::SampleAnimation()
	{
		auto state = mAnimationStates[mStateIndex];
		ozz::animation::SamplingJob sampling_job;
		sampling_job.animation = &state->GetAnimationAsset()->GetAnimation();
		sampling_job.context = &mSamplingContext;
		sampling_job.ratio = mAnimationTime;
		sampling_job.output = ozz::make_span(mLocalSpaceSoaTransforms);
		if (!sampling_job.Run())
		{
			NR_CORE_ERROR("ozz animation sampling job failed!");
		}

		for (int i = 0; i < mLocalSpaceSoaTransforms.size(); ++i)
		{
			ozz::math::SimdFloat4 translations[4];
			ozz::math::SimdFloat4 scales[4];
			ozz::math::SimdFloat4 rotations[4];

			ozz::math::Transpose3x4(&mLocalSpaceSoaTransforms[i].translation.x, translations);
			ozz::math::Transpose3x4(&mLocalSpaceSoaTransforms[i].scale.x, scales);
			ozz::math::Transpose4x4(&mLocalSpaceSoaTransforms[i].rotation.x, rotations);

			for (int j = 0; j < 4; ++j)
			{
				auto index = i * 4 + j;
				if (index >= mLocalTranslations.size())
				{
					break;
				}

				ozz::math::Store3PtrU(translations[j], glm::value_ptr(mLocalTranslations[index]));
				ozz::math::Store3PtrU(scales[j], glm::value_ptr(mLocalScales[index]));
				ozz::math::StorePtrU(rotations[j], glm::value_ptr(mLocalRotations[index]));
			}
		}

		// Get pose of root bone right now...
		RootPose rootPoseNew = { mLocalTranslations[0], mLocalRotations[0] };

		// ... and work out how much it has changed since the previous call to SampleAnimation().
		// This change is the "root motion".
		// Bear in mind some tricky cases:
		// 1) The animation might have looped back around to the beginning.
		// 2) We might be playing the animation backwards.
		// 3) We might have paused the animation, and be "debugging" it, stepping backwards (or forwards) deliberately.

		if (mAnimationPlaying)
		{
			const auto& skeleton = GetSkeletonAsset();
			if (state->GetPlaybackSpeed() >= 0.0f)
			{
				// Animation is playing forwards
				if (mAnimationTime >= mPreviousAnimationTime)
				{
					mRootMotion.Translation = state->GetRootTranslationExtractMask() * (rootPoseNew.Translation - mRootPose.Translation);
					mRootMotion.Rotation = (state->GetRootRotationExtractMask() > 0.0)
						? glm::conjugate(mRootPose.Rotation) * rootPoseNew.Rotation
						: glm::identity<glm::quat>()
						;
				}
				else
				{
					mRootMotion.Translation = state->GetRootTranslationExtractMask() * (mRootPoseEnd.Translation - mRootPose.Translation + rootPoseNew.Translation - mRootPoseStart.Translation);
					mRootMotion.Rotation = (state->GetRootRotationExtractMask() > 0.0)
						? (glm::conjugate(mRootPose.Rotation) * mRootPoseEnd.Rotation) * (glm::conjugate(mRootPoseStart.Rotation) * rootPoseNew.Rotation)
						: glm::identity<glm::quat>()
						;
				}
			}
			else
			{
				// Animation is playing backwards
				if (mAnimationTime <= mPreviousAnimationTime)
				{
					mRootMotion.Translation = state->GetRootTranslationExtractMask() * (rootPoseNew.Translation - mRootPose.Translation);
					mRootMotion.Rotation = (state->GetRootRotationExtractMask() > 0.0)
						? glm::conjugate(mRootPose.Rotation) * rootPoseNew.Rotation
						: glm::identity<glm::quat>()
						;
				}
				else
				{
					mRootMotion.Translation = state->GetRootTranslationExtractMask() * (mRootPoseStart.Translation - mRootPose.Translation + rootPoseNew.Translation - mRootPoseEnd.Translation);
					mRootMotion.Rotation = (state->GetRootRotationExtractMask() > 0.0)
						? (glm::conjugate(mRootPose.Rotation) * mRootPoseStart.Rotation) * (glm::conjugate(mRootPoseEnd.Rotation) * rootPoseNew.Rotation)
						: glm::identity<glm::quat>()
						;
				}

			}
		}
		else
		{
			// Animation is paused (but may have been stepped forwards or backwards via ImGui animation "debugging" tools)
			mRootMotion.Translation = state->GetRootTranslationExtractMask() * (rootPoseNew.Translation - mRootPose.Translation);
			mRootMotion.Rotation = (state->GetRootRotationExtractMask() > 0.0)
				? glm::conjugate(mRootPose.Rotation) * rootPoseNew.Rotation
				: glm::identity<glm::quat>()
				;
		}

		mRootPose = rootPoseNew;
	}
}