#pragma once

#include <map>

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Components.h"
#include "NotRed/Renderer/Mesh.h"
#include "RenderPass.h"

#include "NotRed/Renderer/UniformBufferSet.h"
#include "NotRed/Renderer/RenderCommandBuffer.h"
#include "NotRed/Renderer/PipelineCompute.h"

#include "StorageBufferSet.h"

namespace NR
{
	struct SceneRendererOptions
	{
		bool ShowGrid = true;
		bool ShowSelectedInWireframe = false;

		enum class PhysicsColliderView
		{
			None,
			Normal, 
			OnTop
		};
		PhysicsColliderView ShowPhysicsColliders = PhysicsColliderView::None;

		//HBAO
		bool EnableHBAO = true;
		float HBAOIntensity = 1.5f;
		float HBAORadius = 1.0f;
		float HBAOBias = 0.35f;
		float HBAOBlurSharpness = 1.0f;
	};

	struct SceneRendererCamera
	{
		Camera RenderCamera;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
	};

	struct BloomSettings
	{
		bool Enabled = true;
		float Threshold = 1.0f;
		float Knee = 0.1f;
		float UpsampleScale = 1.0f;
		float Intensity = 1.0f;
		float DirtIntensity = 1.0f;
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
		void BeginScene(const SceneRendererCamera& camera, float dt);
		void EndScene();

		void SubmitMesh(Ref<Mesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> overrideMaterial = nullptr); 
		void SubmitParticles(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));
		void SubmitSelectedMesh(Ref<Mesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> overrideMaterial = nullptr);

		void SubmitPhysicsDebugMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		Ref<RenderPass> GetFinalRenderPass();
		Ref<RenderPass> GetExternalCompositeRenderPass() { return mExternalCompositeRenderPass; }
		Ref<Image2D> GetFinalPassImage();

		SceneRendererOptions& GetOptions();

		void SetShadowSettings(float nearPlane, float farPlane, float lambda)
		{
			CascadeNearPlaneOffset = nearPlane;
			CascadeFarPlaneOffset = farPlane;
			CascadeSplitLambda = lambda;
		}

		BloomSettings& GetBloomSettings() { return mBloomSettings; }

		void SetLineWidth(float width);

		void ImGuiRender();

		static void WaitForThreads();

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

		// Post-processing
		void BloomCompute();
		void CompositePass();

		void ClearPass(Ref<RenderPass> renderPass, bool explicitClear = false);

		struct CascadeData
		{
			glm::mat4 ViewProj;
			glm::mat4 View;
			float SplitDepth;
		};

		void CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection) const;

	private:
		Ref<Scene> mScene;
		SceneRendererSpecification mSpecification;
		Ref<RenderCommandBuffer> mCommandBuffer;

		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;

			// Resources
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

		struct UBEnvironmentParams
		{
			float Time = 0.0f;

			uint32_t NumStars = 75000;
			float StarSize = 10.0f;

			float DustSize = 500.0f;
			float H2Size = 150.0f;

			float H2DistCheck = 300.0f;
		} EnvironmentParamsUB;

		struct UBStarParams
		{
			uint32_t NumStars = 75000;

			float MaxRad = 3500.0f;
			float BulgeRad = 1250.0f;

			float AngleOffset = 6.28f;
			float Eccentricity = 0.85f;

			float BaseHeight = 300.0f;
			float Height = 250.0f;

			float MinTemp = 3000.0f;
			float MaxTemp = 9000.0f;
			float DustBaseTemp = 4000.0f;

			float MinStarOpacity = 0.1f;
			float MaxStarOpacity = 0.5f;

			float MinDustOpacity = 0.01f;
			float MaxDustOpacity = 0.05f;

			float Speed = 10.0f;
		} StarParamsUB;

		struct UBScene
		{
			DirLight lights;
			glm::vec3 CameraPosition;
			float EnvironmentMapIntensity = 1.0f;
		} SceneDataUB;

		struct UBRendererData
		{
			glm::vec4 CascadeSplits;
			uint32_t TilesCountX{ 0 };
			bool ShowCascades = false;
			char Padding0[3] = { 0,0,0 }; // Bools are 4-bytes in GLSL
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

		Ref<PipelineCompute> mHBAOPipeline;
		Ref<Image2D> mHBAOOutputImage;
		Ref<RenderPass> mHBAORenderPass;

		glm::ivec3 mLightCullingWorkGroups;

		Ref<UniformBufferSet> mUniformBufferSet;
		Ref<StorageBufferSet> mStorageBufferSet;

		float LightDistance = 0.1f;
		float CascadeSplitLambda = 0.92f;
		glm::vec4 CascadeSplits;
		float CascadeFarPlaneOffset = 50.0f, CascadeNearPlaneOffset = -50.0f;

		bool EnableBloom = false;
		float BloomThreshold = 1.5f;

		glm::vec2 FocusPoint = { 0.5f, 0.5f };

		Ref<Material> CompositeMaterial;
		Ref<Material> mLightCullingMaterial;

		Ref<Pipeline> mGeometryPipeline;
		Ref<Pipeline> mGeometryPipelineAnim;

		Ref<Pipeline> mSelectedGeometryPipeline;
		Ref<Pipeline> mSelectedGeometryPipelineAnim;

		Ref<Pipeline> mGeometryWireframePipeline;
		Ref<Pipeline> mGeometryWireframePipelineAnim;
		Ref<Pipeline> mGeometryWireframeOnTopPipeline;
		Ref<Pipeline> mGeometryWireframeOnTopPipelineAnim;
		Ref<Material> mWireframeMaterial;

		Ref<Pipeline> mPreDepthPipeline;
		Ref<Pipeline> mPreDepthPipelineAnim;
		Ref<Material> mPreDepthMaterial;

		Ref<Pipeline> mCompositePipeline;

		Ref<Pipeline> mShadowPassPipelines[4];
		Ref<Pipeline> mShadowPassPipelinesAnim[4];
		Ref<Material> mShadowPassMaterial;


		Ref<Pipeline> mSkyboxPipeline;
		Ref<Material> mSkyboxMaterial;

		Ref<Pipeline> mDOFPipeline;
		Ref<Material> mDOFMaterial;

		Ref<PipelineCompute> mLightCullingPipeline;

		// Particle Pass
		glm::ivec3 mParticleGenWorkGroups;
		Ref<PipelineCompute> mParticleGenPipeline;
		Ref<Pipeline> mParticlePipeline;
		Ref<Material> mParticleGenMaterial;

		// Jump Flood Pass
		Ref<Pipeline> mJumpFloodInitPipeline;
		Ref<Pipeline> mJumpFloodPassPipeline[2];
		Ref<Pipeline> mJumpFloodCompositePipeline;
		Ref<Material> mJumpFloodInitMaterial, mJumpFloodPassMaterial[2];
		Ref<Material> mJumpFloodCompositeMaterial;

		// Bloom compute
		uint32_t mBloomComputeWorkgroupSize = 4;
		Ref<PipelineCompute> mBloomComputePipeline;
		Ref<Texture2D> mBloomComputeTextures[3];
		Ref<Material> mBloomComputeMaterial;

		Ref<Material> mSelectedGeometryMaterial;

		std::vector<Ref<FrameBuffer>> mTempFrameBuffers;

		Ref<RenderPass> mExternalCompositeRenderPass;

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			Ref<MaterialTable> MaterialTable;
			glm::mat4 Transform;
			Ref<Material> OverrideMaterial;
		};
		std::vector<DrawCommand> mDrawList;
		std::vector<DrawCommand> mSelectedMeshDrawList;
		std::vector<DrawCommand> mColliderDrawList;
		std::vector<DrawCommand> mShadowPassDrawList;
		std::vector<DrawCommand> mParticlesDrawList;

		// Grid
		Ref<Pipeline> mGridPipeline;
		Ref<Material> mGridMaterial;
		Ref<Material> mColliderMaterial;

		SceneRendererOptions mOptions;

		uint32_t mViewportWidth = 0, mViewportHeight = 0;
		float mInvViewportWidth = 0.f, mInvViewportHeight = 0.f;
		bool mNeedsResize = false;
		bool mActive = false;
		bool mResourcesCreated = false;

		float mLineWidth = 2.0f;

		BloomSettings mBloomSettings;
		Ref<Texture2D> mBloomDirtTexture;

		struct GPUTimeQueries
		{
			uint32_t ShadowMapPassQuery = 0;
			uint32_t DepthPrePassQuery = 0;
			uint32_t LightCullingPassQuery = 0;
			uint32_t GeometryPassQuery = 0;
			uint32_t HBAOPassQuery = 0;
			uint32_t BloomComputePassQuery = 0;
			uint32_t JumpFloodPassQuery = 0;
			uint32_t CompositePassQuery = 0;
		} mGPUTimeQueries;
	};
}