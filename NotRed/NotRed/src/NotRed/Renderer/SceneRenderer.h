#pragma once

#include <map>

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Components.h"
#include "NotRed/Renderer/Mesh.h"
#include "RenderPass.h"

#include "NotRed/Renderer/UniformBufferSet.h"
#include "NotRed/Renderer/RenderCommandBuffer.h"
#include "NotRed/Platform/Vulkan/VKComputePipeline.h"

#include "StorageBufferSet.h"

namespace NR
{
	struct SceneRendererOptions
	{
		bool ShowGrid = true;
		bool ShowBoundingBoxes = false;
		bool ShowSelectedInWireframe = false;
		bool ShowCollidersWireframe = false;

		//HBAO
		bool EnableHBAO = true;
		float HBAOIntensity = 1.6f;
		float HBAORadius = 1.3f;
		float HBAOBias = 0.3f;
		float HBAOBlurSharpness = 20.f;
	};

	struct SceneRendererCamera
	{
		Camera CameraObj;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
	};

	struct SceneRendererSpecification
	{
		bool SwapChainTarget = false;
	};

	class SceneRenderer : public RefCounted
	{
	public:
		SceneRenderer(Ref<Scene> scene, SceneRendererSpecification specification = SceneRendererSpecification());

		void Init();

		void SetScene(Ref<Scene> scene);

		void SetViewportSize(uint32_t width, uint32_t height);

		void UpdateHBAOData();

		void BeginScene(const SceneRendererCamera& camera);
		void EndScene();

		void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> overrideMaterial = nullptr);
		void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));
		void SubmitParticles(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));
		void SubmitColliderMesh(const BoxColliderComponent& component, const glm::mat4& parentTransform = glm::mat4(1.0F));
		void SubmitColliderMesh(const SphereColliderComponent& component, const glm::mat4& parentTransform = glm::mat4(1.0F));
		void SubmitColliderMesh(const CapsuleColliderComponent& component, const glm::mat4& parentTransform = glm::mat4(1.0F));
		void SubmitColliderMesh(const MeshColliderComponent& component, const glm::mat4& parentTransform = glm::mat4(1.0F));

		Ref<RenderPass> GetFinalRenderPass();
		Ref<Image2D> GetFinalPassImage();

		Ref<RenderPass> GetExternalCompositeRenderPass() { return mExternalCompositeRenderPass; }

		SceneRendererOptions& GetOptions();

		void SetShadowSettings(float nearPlane, float farPlane, float lambda)
		{
			CascadeNearPlaneOffset = nearPlane;
			CascadeFarPlaneOffset = farPlane;
			CascadeSplitLambda = lambda;
		}

		void ImGuiRender();

		static void WaitForThreads();
	private:
		struct CascadeData
		{
			glm::mat4 ViewProj;
			glm::mat4 View;
			float SplitDepth;
		};

	private:
		void FlushDrawList();
		void ClearPass();
		void DeinterleavingPass();
		void HBAOPass();
		void ReinterleavingPass();
		void HBAOBlurPass();
		void ShadowMapPass();
		void PreDepthPass();
		void LightCullingPass();
		void GeometryPass();
		void JumpFloodPass();
		void CompositePass();
		void BloomBlurPass();

		void CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection) const;

	private:
		Ref<Scene> mScene;
		SceneRendererSpecification mSpecification;
		Ref<RenderCommandBuffer> mCommandBuffer;

		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;

			Ref<Environment> SceneEnvironment;
			float SkyboxLod = 0.0f;
			float SceneEnvironmentIntensity;
			LightEnvironment SceneLightEnvironment;
			DirLight ActiveLight;
		} mSceneData;

		Ref<Shader> mCompositeShader;
		Ref<Shader> mBloomBlurShader;
		Ref<Shader> mBloomBlendShader;

		struct UBCamera
		{
			glm::mat4 ViewProjection;
			glm::mat4 InverseViewProjection;
			glm::mat4 Projection;
			glm::mat4 View;
		} CameraDataUB;

		struct UBHBAOData
		{
			glm::vec4 PerspectiveInfo;
			glm::vec2 InvQuarterResolution;
		
			float RadiusToScreen;
			float NegInvR2;
			float NDotVBias;
			float AOMultiplier;
			float PowExponent;
			
			bool IsOrtho;
			char Padding0[3]{ 0, 0, 0 };

			glm::vec4 Float2Offsets[16];
			glm::vec4 Jitters[16];
		} HBAODataUB;

		struct UBScreenData
		{
			glm::vec2 InvFullResolution;
			glm::vec2 FullResolution;
		} ScreenDataUB;

		struct UBShadow
		{
			glm::mat4 ViewProjection[4];
		} ShadowData;

		struct DirLight
		{
			glm::vec3 Direction;
			float Padding = 0.0f;
			glm::vec3 Radiance;
			float Multiplier;
		};

		struct UBPointLights
		{
			uint32_t Count{ 0 };
			glm::vec3 Padding{};
			PointLight PointLights[1024]{};
		} PointLightsUB;

		struct UBScene
		{
			DirLight lights;
			glm::vec3 CameraPosition;
			float EnvironmentMapIntensity = 1.0f;
		} SceneDataUB; 
		
		struct UBStarParams
		{
			uint32_t NumStars;

			float MaxRad;
			float BulgeRad;

			float AngleOffset;
			float Eccentricity;

			float BaseHeight;
			float Height;

			float MinTemp;
			float MaxTemp;
			float DustBaseTemp;

			float MinStarOpacity;
			float MaxStarOpacity;

			float MinDustOpacity;
			float MaxDustOpacity;

			float Speed;
		} StarParamsUB;

		struct UBRendererData
		{
			glm::vec4 CascadeSplits;
			uint32_t TilesCountX{ 0 };
			bool ShowCascades = false;
			char Padding0[3] = { 0,0,0 };
			bool SoftShadows = true;
			char Padding1[3] = { 0,0,0 };
			float LightSize = 0.5f;
			float MaxShadowDistance = 200.0f;
			float ShadowFade = 1.0f;
			bool CascadeFading = true;
			char Padding2[3] = { 0,0,0 };
			float CascadeTransitionFade = 1.0f;
			bool ShowLightComplexity = false;
			char Padding3[3] = { 0,0,0 };
		} RendererDataUB;

		glm::ivec3 mHBAOWorkGroups{ 32.f, 32.f, 16.f };
		
		//HBAO
		Ref<Material> mDeinterleavingMaterial;
		Ref<Material> mReinterleavingMaterial;
		Ref<Material> mHBAOBlurMaterials[2];
		Ref<Material> mHBAOMaterial;
		Ref<Pipeline> mDeinterleavingPipelines[2];
		Ref<Pipeline> mReinterleavingPipeline;
		Ref<Pipeline> mHBAOBlurPipelines[2];

		Ref<VKComputePipeline> mHBAOPipeline;
		Ref<Image2D> mHBAOOutputImage;
		Ref<RenderPass> mHBAORenderPass;

		glm::ivec3 mLightCullingWorkGroups;
		Ref<VKComputePipeline> mLightCullingPipeline;

		glm::ivec3 mParticleGenWorkGroups;
		Ref<VKComputePipeline> mParticleGenPipeline;
		Ref<Pipeline> mParticlePipeline;

		Ref<UniformBufferSet> mUniformBufferSet;
		Ref<StorageBufferSet> mStorageBufferSet;

		Ref<Shader> ShadowMapShader, ShadowMapAnimShader;

		float LightDistance = 0.1f;
		float CascadeSplitLambda = 0.92f;
		glm::vec4 CascadeSplits;
		float CascadeFarPlaneOffset = 50.0f, CascadeNearPlaneOffset = -50.0f;

		bool EnableBloom = false;
		float BloomThreshold = 1.5f;

		glm::vec2 FocusPoint = { 0.5f, 0.5f };

		Ref<Material> CompositeMaterial;
		Ref<Material> mLightCullingMaterial;
		Ref<Material> mParticleGenMaterial;

		Ref<Pipeline> mGeometryPipeline;
		Ref<Pipeline> mSelectedGeometryPipeline;
		Ref<Pipeline> mGeometryWireframePipeline;
		Ref<Pipeline> mPreDepthPipeline;
		Ref<Pipeline> mCompositePipeline;
		Ref<Pipeline> mShadowPassPipelines[4];
		Ref<Material> mShadowPassMaterial;
		Ref<Material> mPreDepthMaterial;
		Ref<Pipeline> mSkyboxPipeline;
		Ref<Material> mSkyboxMaterial;

		Ref<RenderPass> mExternalCompositeRenderPass;

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			Ref<Material> Material;
			glm::mat4 Transform;
		};
		std::vector<DrawCommand> mDrawList;
		std::vector<DrawCommand> mSelectedMeshDrawList;
		std::vector<DrawCommand> mParticlesDrawList;
		std::vector<DrawCommand> mColliderDrawList;
		std::vector<DrawCommand> mShadowPassDrawList;

		// Jump Flood Pass
		Ref<Pipeline> mJumpFloodInitPipeline;
		Ref<Pipeline> mJumpFloodPassPipeline[2];
		Ref<Pipeline> mJumpFloodCompositePipeline;
		Ref<Material> mJumpFloodInitMaterial, mJumpFloodPassMaterial[2];
		Ref<Material> mJumpFloodCompositeMaterial;
		Ref<Material> mSelectedGeometryMaterial;

		std::vector<Ref<FrameBuffer>> mTempFrameBuffers;

		// Grid
		Ref<Pipeline> mGridPipeline;
		Ref<Shader> mGridShader;
		Ref<Material> mGridMaterial;
		Ref<Material> mOutlineMaterial, OutlineAnimMaterial;
		Ref<Material> mColliderMaterial;
		Ref<Material> mWireframeMaterial;

		SceneRendererOptions mOptions;

		uint32_t mViewportWidth = 0, mViewportHeight = 0;
		float mInvViewportWidth = 0.0f, mInvViewportHeight = 0.0f;
		bool mNeedsResize = false;
		bool mActive = false;
		bool mResourcesCreated = false;
	};

}