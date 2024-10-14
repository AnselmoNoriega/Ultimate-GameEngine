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
	};

	struct SceneRendererCamera
	{
		Camera CameraObj;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
	};

	class SceneRenderer : public RefCounted
	{
	public:
		SceneRenderer(Ref<Scene> scene);
		~SceneRenderer();

		void Init();

		void SetScene(Ref<Scene> scene);

		void SetViewportSize(uint32_t width, uint32_t height);

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
		void ShadowMapPass();
		void PreDepthPass();
		void LightCullingPass();
		void GeometryPass();
		void CompositePass();
		void BloomBlurPass();

		void CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection) const;

	private:
		Ref<Scene> mScene;
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
			glm::mat4 View;
		} CameraData;

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
		} SceneDataUB;

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
		} RendererDataUB;

		glm::ivec3 mLightCullingWorkGroups;
		Ref<VKComputePipeline> mLightCullingPipeline;

		Ref<UniformBufferSet> mUniformBufferSet;
		Ref<StorageBufferSet> mStorageBufferSet;

		Ref<Shader> ShadowMapShader, ShadowMapAnimShader;
		Ref<RenderPass> ShadowMapRenderPass[4];
		float LightDistance = 0.1f;
		float CascadeSplitLambda = 0.98f;
		glm::vec4 CascadeSplits;
		float CascadeFarPlaneOffset = 15.0f, CascadeNearPlaneOffset = -15.0f;

		bool EnableBloom = false;
		float BloomThreshold = 1.5f;

		glm::vec2 FocusPoint = { 0.5f, 0.5f };

		RendererID ShadowMapSampler;
		Ref<Material> CompositeMaterial;
		Ref<Material> mLightCullingMaterial;

		Ref<Pipeline> mGeometryPipeline;
		Ref<Pipeline> mParticlePipeline;
		Ref<Pipeline> mPreDepthPipeline;
		Ref<Pipeline> mCompositePipeline;
		Ref<Pipeline> mShadowPassPipeline;
		Ref<Material> mShadowPassMaterial;
		Ref<Material> mPreDepthMaterial;
		Ref<Pipeline> mSkyboxPipeline;
		Ref<Material> mSkyboxMaterial;

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

		// Grid
		Ref<Pipeline> mGridPipeline;
		Ref<Shader> mGridShader;
		Ref<Material> mGridMaterial;
		Ref<Material> mOutlineMaterial, OutlineAnimMaterial;
		Ref<Material> mColliderMaterial;

		SceneRendererOptions mOptions;

		uint32_t mViewportWidth = 0, mViewportHeight = 0;
		bool mNeedsResize = false;
		bool mActive = false;
		bool mResourcesCreated = false;
	};

}