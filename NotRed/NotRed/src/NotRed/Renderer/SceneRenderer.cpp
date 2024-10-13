#include "nrpch.h"
#include "SceneRenderer.h"

#include <glad/glad.h>
#include <imgui.h>

#include <glm/gtc/matrix_transform.hpp>

#include "NotRed/ImGui/ImGui.h"

#include "Renderer.h"
#include "Renderer2D.h"

#include "UniformBuffer.h"

#include "SceneEnvironment.h"

#include "NotRed/Renderer/MeshFactory.h"

namespace NR
{
	static std::vector<std::thread> sThreadPool;

	SceneRenderer::SceneRenderer(Ref<Scene> scene)
		: mScene(scene)
	{
		Init();
	}

	SceneRenderer::~SceneRenderer()
	{
	}

	void SceneRenderer::Init()
	{
		NR_SCOPE_TIMER("SceneRenderer::Init");

		mCommandBuffer = RenderCommandBuffer::Create();

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		mUniformBufferSet = UniformBufferSet::Create(framesInFlight);

		mUniformBufferSet->Create(sizeof(UBCamera), 0);
		mUniformBufferSet->Create(sizeof(UBShadow), 1);
		mUniformBufferSet->Create(sizeof(UBScene), 2);
		mUniformBufferSet->Create(sizeof(UBRendererData), 3);

		mCompositeShader = Renderer::GetShaderLibrary()->Get("HDR");
		CompositeMaterial = Material::Create(mCompositeShader);

		// Shadow pass
		{
			ImageSpecification spec;
			spec.Format = ImageFormat::DEPTH32F;
			spec.Usage = ImageUsage::Attachment;
			spec.Width = 4096;
			spec.Height = 4096;
			spec.Layers = 4; // 4 cascades
			Ref<Image2D> cascadedDepthImage = Image2D::Create(spec);
			cascadedDepthImage->Invalidate();
			cascadedDepthImage->CreatePerLayerImageViews();

			FrameBufferSpecification shadowMapFramebufferSpec;
			shadowMapFramebufferSpec.Width = 4096;
			shadowMapFramebufferSpec.Height = 4096;
			shadowMapFramebufferSpec.Attachments = { ImageFormat::DEPTH32F };
			shadowMapFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			shadowMapFramebufferSpec.Resizable = false;
			shadowMapFramebufferSpec.ExistingImage = cascadedDepthImage;

			// 4 cascades
			for (int i = 0; i < 4; ++i)
			{
				shadowMapFramebufferSpec.ExistingImageLayer = i;

				RenderPassSpecification shadowMapRenderPassSpec;
				shadowMapRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(shadowMapFramebufferSpec);
				shadowMapRenderPassSpec.DebugName = "ShadowMap";
				ShadowMapRenderPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
			}

			auto shadowPassShader = Renderer::GetShaderLibrary()->Get("ShadowMap");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "ShadowPass";
			pipelineSpec.Shader = shadowPassShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float3, "aNormal" },
				{ ShaderDataType::Float3, "aTangent" },
				{ ShaderDataType::Float3, "aBinormal" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpec.RenderPass = ShadowMapRenderPass[0];
			mShadowPassPipeline = Pipeline::Create(pipelineSpec);
			mShadowPassMaterial = Material::Create(shadowPassShader, "ShadowPass");
		}

		// Geometry
		{
			FrameBufferSpecification geoFramebufferSpec;
			geoFramebufferSpec.Width = 1280;
			geoFramebufferSpec.Height = 720;
			geoFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA32F, ImageFormat::Depth };
			geoFramebufferSpec.Samples = 1;
			geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

			Ref<FrameBuffer> framebuffer = FrameBuffer::Create(geoFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float3, "aNormal" },
				{ ShaderDataType::Float3, "aTangent" },
				{ ShaderDataType::Float3, "aBinormal" },
				{ ShaderDataType::Float2, "aTexCoord" },
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("HazelPBR_Static");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFrameBuffer = framebuffer;
			renderPassSpec.DebugName = "Geometry";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "PBR-Static";
			mGeometryPipeline = Pipeline::Create(pipelineSpecification);
		}

		// Composite
		{
			FrameBufferSpecification compFramebufferSpec;
			compFramebufferSpec.Width = 1280;
			compFramebufferSpec.Height = 720;
			compFramebufferSpec.Attachments = { ImageFormat::RGBA };
			compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

			Ref<FrameBuffer> framebuffer = FrameBuffer::Create(compFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("HDR");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFrameBuffer = framebuffer;
			renderPassSpec.DebugName = "Composite";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "HDR";
			mCompositePipeline = Pipeline::Create(pipelineSpecification);
		}

		// Grid
		{
			mGridShader = Renderer::GetShaderLibrary()->Get("Grid");
			const float gridScale = 16.025f;
			const float gridSize = 0.025f;
			mGridMaterial = Material::Create(mGridShader);
			mGridMaterial->Set("uSettings.Scale", gridScale);
			mGridMaterial->Set("uSettings.Size", gridSize);

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Grid";
			pipelineSpec.Shader = mGridShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpec.RenderPass = mGeometryPipeline->GetSpecification().RenderPass;
			mGridPipeline = Pipeline::Create(pipelineSpec);
		}

		// Skybox
		{
			auto skyboxShader = Renderer::GetShaderLibrary()->Get("Skybox");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Skybox";
			pipelineSpec.Shader = skyboxShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpec.RenderPass = mGeometryPipeline->GetSpecification().RenderPass;
			mSkyboxPipeline = Pipeline::Create(pipelineSpec);

			mSkyboxMaterial = Material::Create(skyboxShader);
			mSkyboxMaterial->ModifyFlags(MaterialFlag::DepthTest, false);
		}

		Ref<SceneRenderer> instance = this;
		Renderer::Submit([instance]() mutable
			{
				instance->mResourcesCreated = true;
			});
	}

	void SceneRenderer::SetScene(Ref<Scene> scene)
	{
		NR_CORE_ASSERT(!mActive, "Can't change scenes while rendering");
		mScene = scene;
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (mViewportWidth != width || mViewportHeight != height)
		{
			mViewportWidth = width;
			mViewportHeight = height;
			mNeedsResize = true;
		}
	}

	void SceneRenderer::CalculateCascades(SceneRenderer::CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection)
	{
		struct FrustumBounds
		{
			float r, l, b, t, f, n;
		};

		FrustumBounds frustumBounds[3];

		auto viewProjection = sceneCamera.CameraObj.GetProjectionMatrix() * sceneCamera.ViewMatrix;

		const int SHADOW_MAP_CASCADE_COUNT = 4;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];
		
		float nearClip = 0.1f;
		float farClip = 1000.0f;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i)
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		cascadeSplits[3] = 0.3f;

		// Manually set cascades here
		// cascadeSplits[0] = 0.05f;
		// cascadeSplits[1] = 0.15f;
		// cascadeSplits[2] = 0.3f;
		// cascadeSplits[3] = 1.0f;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] =
			{
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(viewProjection);
			for (uint32_t i = 0; i < 8; ++i)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; ++i)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; ++i)
			{
				frustumCenter += frustumCorners[i];
			}

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; ++i)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -lightDirection;
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + CascadeNearPlaneOffset, maxExtents.z - minExtents.z + CascadeFarPlaneOffset);

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			const float ShadowMapResolution = 4096.0f;
			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			// Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

	void SceneRenderer::BeginScene(const SceneRendererCamera& camera)
	{
		NR_CORE_ASSERT(mScene);
		NR_CORE_ASSERT(!mActive);
		mActive = true;

		if (!mResourcesCreated)
		{
			return;
		}

		mSceneData.SceneCamera = camera;
		mSceneData.SceneEnvironment = mScene->mEnvironment;
		mSceneData.SceneEnvironmentIntensity = mScene->mEnvironmentIntensity;
		mSceneData.ActiveLight = mScene->mLight;
		mSceneData.SceneLightEnvironment = mScene->mLightEnvironment;
		mSceneData.SkyboxLod = mScene->mSkyboxLod;
		mSceneData.ActiveLight = mScene->mLight;

		if (mNeedsResize)
		{
			mGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(mViewportWidth, mViewportHeight);
			mCompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(mViewportWidth, mViewportHeight);
			mNeedsResize = false;			
			
			Renderer::Submit([=]
				{
					auto shader = sData->LightCullingMaterial->GetShader();
					shader->ResizeStorageBuffer(14, sData->LightCullingWorkGroups.x * sData->LightCullingWorkGroups.y * 4 * 1024);
				});
		}

		// Update uniform buffers
		UBCamera& cameraData = CameraData;
		UBScene& sceneData = SceneDataUB;
		UBShadow& shadowData = ShadowData;
		UBRendererData& rendererData = RendererDataUB;

		auto& sceneCamera = mSceneData.SceneCamera;
		auto viewProjection = sceneCamera.CameraObj.GetProjectionMatrix() * sceneCamera.ViewMatrix;
		glm::vec3 cameraPosition = glm::inverse(sceneCamera.ViewMatrix)[3];

		auto inverseVP = glm::inverse(viewProjection);
		cameraData.ViewProjection = viewProjection;
		cameraData.InverseViewProjection = inverseVP;
		cameraData.View = sceneCamera.ViewMatrix;
		Ref<SceneRenderer> instance = this;
		Renderer::Submit([instance, cameraData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->mUniformBufferSet->Get(0, 0, bufferIndex)->RT_SetData(&cameraData, sizeof(cameraData));
			});

		const auto& directionalLight = mSceneData.SceneLightEnvironment.DirectionalLights[0];
		sceneData.lights.Direction = directionalLight.Direction;
		sceneData.lights.Radiance = directionalLight.Radiance;
		sceneData.lights.Multiplier = directionalLight.Multiplier;
		sceneData.CameraPosition = cameraPosition;
		Renderer::Submit([instance, sceneData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->mUniformBufferSet->Get(2, 0, bufferIndex)->RT_SetData(&sceneData, sizeof(sceneData));
			});

		CascadeData cascades[4];
		CalculateCascades(cascades, sceneCamera, directionalLight.Direction);

		for (int i = 0; i < 4; ++i)
		{
			CascadeSplits[i] = cascades[i].SplitDepth;
			shadowData.ViewProjection[i] = cascades[i].ViewProj;
		}

		Renderer::Submit([instance, shadowData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->mUniformBufferSet->Get(1, 0, bufferIndex)->RT_SetData(&shadowData, sizeof(shadowData));
			});

		rendererData.CascadeSplits = CascadeSplits;
		Renderer::Submit([instance, rendererData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->mUniformBufferSet->Get(3, 0, bufferIndex)->RT_SetData(&rendererData, sizeof(rendererData));
			});

		Renderer::SetSceneEnvironment(this, mSceneData.SceneEnvironment, mShadowPassPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetDepthImage());
	}

	void SceneRenderer::EndScene()
	{
		NR_CORE_ASSERT(mActive);

#if MULTI_THREAD
		Ref<SceneRenderer> instance = this;
		sThreadPool.emplace_back(([instance]() mutable
			{
				instance->FlushDrawList();
			}));
#else 
		FlushDrawList();
#endif

		mActive = false;
	}

	void SceneRenderer::WaitForThreads()
	{
		for (uint32_t i = 0; i < sThreadPool.size(); ++i)
		{
			sThreadPool[i].join();
		}
		sThreadPool.clear();
	}

	void SceneRenderer::SubmitParticles(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		mParticlesDrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		mDrawList.push_back({ mesh, overrideMaterial, transform });
		mShadowPassDrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform)
	{
		mSelectedMeshDrawList.push_back({ mesh, nullptr, transform });
		mShadowPassDrawList.push_back({ mesh, nullptr, transform });
	}

	void SceneRenderer::SubmitColliderMesh(const BoxColliderComponent& component, const glm::mat4& parentTransform)
	{
		mColliderDrawList.push_back({ component.DebugMesh, nullptr, glm::translate(parentTransform, component.Offset) });
	}

	void SceneRenderer::SubmitColliderMesh(const SphereColliderComponent& component, const glm::mat4& parentTransform)
	{
		mColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::SubmitColliderMesh(const CapsuleColliderComponent& component, const glm::mat4& parentTransform)
	{
		mColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::SubmitColliderMesh(const MeshColliderComponent& component, const glm::mat4& parentTransform)
	{
		for (auto debugMesh : component.ProcessedMeshes)
			mColliderDrawList.push_back({ debugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::ShadowMapPass()
	{
		auto& directionalLights = mSceneData.SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Multiplier == 0.0f || !directionalLights[0].CastShadows)
		{
			for (int i = 0; i < 4; ++i)
			{
				// Clear shadow maps
				Renderer::BeginRenderPass(mCommandBuffer, ShadowMapRenderPass[i]);
				Renderer::EndRenderPass(mCommandBuffer);
			}
			return;
		}
		
		for (int i = 0; i < 4; ++i)
		{
			Renderer::BeginRenderPass(mCommandBuffer, ShadowMapRenderPass[i]);

			// Render entities
			Buffer cascade(&i, sizeof(uint32_t));
			for (auto& dc : mShadowPassDrawList)
			{
				Renderer::RenderMesh(mCommandBuffer, mShadowPassPipeline, mUniformBufferSet, dc.Mesh, dc.Transform, mShadowPassMaterial, cascade);
			}

			Renderer::EndRenderPass(mCommandBuffer);
		}
	}

	void SceneRenderer::GeometryPass()
	{
		Renderer::BeginRenderPass(mCommandBuffer, mGeometryPipeline->GetSpecification().RenderPass);
		// Skybox
		mSkyboxMaterial->Set("uUniforms.TextureLod", mSceneData.SkyboxLod);

		Ref<TextureCube> radianceMap = mSceneData.SceneEnvironment ? mSceneData.SceneEnvironment->RadianceMap : Renderer::GetBlackCubeTexture();
		mSkyboxMaterial->Set("uTexture", radianceMap);
		Renderer::SubmitFullscreenQuad(mCommandBuffer, mSkyboxPipeline, mUniformBufferSet, mSkyboxMaterial);

		// Render entities
		for (auto& dc : mDrawList)
		{
			Renderer::RenderMesh(mCommandBuffer, mGeometryPipeline, mUniformBufferSet, dc.Mesh, dc.Transform);
		}

		for (auto& dc : mSelectedMeshDrawList)
		{
			Renderer::RenderMesh(mCommandBuffer, mGeometryPipeline, mUniformBufferSet, dc.Mesh, dc.Transform);
		}

		for (auto& dc : mParticlesDrawList)
		{
			//TODO:
			//Renderer::RenderParticles(mCommandBuffer, mGeometryPipeline, mUniformBufferSet, dc.Mesh, dc.Transform);
		}

		// Grid
		if (GetOptions().ShowGrid)
		{
			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(8.0f));
			Renderer::RenderQuad(mCommandBuffer, mGridPipeline, mUniformBufferSet, mGridMaterial, transform);
		}

		if (GetOptions().ShowBoundingBoxes)
		{
#if 0
			Renderer2D::BeginScene(viewProjection);
			for (auto& dc : DrawList)
			{
				Renderer::DrawAABB(dc.Mesh, dc.Transform);
			}
			Renderer2D::EndScene();
#endif
		}

		Renderer::EndRenderPass(mCommandBuffer);
	}

	void SceneRenderer::CompositePass()
	{
		Renderer::BeginRenderPass(mCommandBuffer, mCompositePipeline->GetSpecification().RenderPass);

		auto framebuffer = mGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer;
		float exposure = mSceneData.SceneCamera.CameraObj.GetExposure();
		int textureSamples = framebuffer->GetSpecification().Samples;

		CompositeMaterial->Set("uUniforms.Exposure", exposure);

		CompositeMaterial->Set("uTexture", framebuffer->GetImage());

		Renderer::SubmitFullscreenQuad(mCommandBuffer, mCompositePipeline, nullptr, CompositeMaterial);
		Renderer::EndRenderPass(mCommandBuffer);
	}

	void SceneRenderer::BloomBlurPass()
	{
#if 0
		int amount = 10;
		int index = 0;

		int horizontalCounter = 0, verticalCounter = 0;
		for (int i = 0; i < amount; ++i)
		{
			index = i % 2;
			Renderer::BeginRenderPass(BloomBlurPass[index]);
			BloomBlurShader->Bind();
			BloomBlurShader->SetBool("uHorizontal", index);
			if (index)
			{
				horizontalCounter++;
			}
			else
			{
				verticalCounter++;
			}
			if (i > 0)
			{
				auto fb = BloomBlurPass[1 - index]->GetSpecification().TargetFramebuffer;
				fb->BindTexture();
			}
			else
			{
				auto fb = CompositePass->GetSpecification().TargetFramebuffer;
				auto id = fb->GetColorAttachmentRendererID(1);
				Renderer::Submit([id]()
					{
						glBindTextureUnit(0, id);
					});
			}
			Renderer::SubmitFullscreenQuad(nullptr);
			Renderer::EndRenderPass();
		}

		// Composite bloom
		{
			Renderer::BeginRenderPass(BloomBlendPass);
			BloomBlendShader->Bind();
			BloomBlendShader->SetFloat("uExposure", SceneData.SceneCamera.Camera.GetExposure());
			BloomBlendShader->SetBool("uEnableBloom", EnableBloom);

			CompositePass->GetSpecification().TargetFramebuffer->BindTexture(0);
			BloomBlurPass[index]->GetSpecification().TargetFramebuffer->BindTexture(1);

			Renderer::SubmitFullscreenQuad(nullptr);
			Renderer::EndRenderPass();
		}
#endif
	}

	void SceneRenderer::FlushDrawList()
	{
		if (mResourcesCreated)
		{
			mCommandBuffer->Begin();
			ShadowMapPass();
			GeometryPass();
			CompositePass();
			mCommandBuffer->End();
			mCommandBuffer->Submit();
			// BloomBlurPass();
		}

		mDrawList.clear();
		mParticlesDrawList.clear();
		mSelectedMeshDrawList.clear();
		mShadowPassDrawList.clear();
		mColliderDrawList.clear();
		mSceneData = {};
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return mCompositePipeline->GetSpecification().RenderPass;
	}

	Ref<Image2D> SceneRenderer::GetFinalPassImage()
	{
		if (!mResourcesCreated)
		{
			return nullptr;
		}

		return mCompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage();
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return mOptions;
	}

	void SceneRenderer::ImGuiRender()
	{
		ImGui::Begin("Scene Renderer");

		if (ImGui::TreeNode("Shaders"))
		{
			auto& shaders = Shader::sAllShaders;
			for (auto& shader : shaders)
			{
				if (ImGui::TreeNode(shader->GetName().c_str()))
				{
					std::string buttonName = "Reload##" + shader->GetName();
					if (ImGui::Button(buttonName.c_str()))
					{
						shader->Reload(true);
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		if (UI::BeginTreeNode("Shadows"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Soft Shadows", RendererDataUB.SoftShadows);
			UI::Property("Light Size", RendererDataUB.LightSize, 0.01f);
			UI::Property("Max Shadow Distance", RendererDataUB.MaxShadowDistance, 1.0f);
			UI::Property("Shadow Fade", RendererDataUB.ShadowFade, 5.0f);
			UI::EndPropertyGrid();
			if (UI::BeginTreeNode("Cascade Settings"))
			{
				UI::BeginPropertyGrid();
				UI::Property("Show Cascades", RendererDataUB.ShowCascades);
				UI::Property("Cascade Fading", RendererDataUB.CascadeFading);
				UI::Property("Cascade Transition Fade", RendererDataUB.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
				UI::Property("Cascade Split", CascadeSplitLambda, 0.01f);
				UI::Property("CascadeNearPlaneOffset", CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
				UI::Property("CascadeFarPlaneOffset", CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			if (UI::BeginTreeNode("Shadow Map", false))
			{
				static int cascadeIndex = 0;
				auto fb = ShadowMapRenderPass[cascadeIndex]->GetSpecification().TargetFrameBuffer;
				auto image = fb->GetDepthImage();

				float size = ImGui::GetContentRegionAvail().x;
				UI::BeginPropertyGrid();
				UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
				UI::EndPropertyGrid();
				UI::Image(image, (uint32_t)cascadeIndex, { size, size }, { 0, 1 }, { 1, 0 });
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}

#if 0
		if (UI::BeginTreeNode("Bloom"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Bloom", EnableBloom);
			UI::Property("Bloom threshold", BloomThreshold, 0.05f);
			UI::EndPropertyGrid();

			auto fb = BloomBlurPass[0]->GetSpecification().TargetFramebuffer;
			auto id = fb->GetColorAttachmentRendererID();

			float size = ImGui::GetContentRegionAvail().x;
			float w = size;
			float h = w / ((float)fb->GetWidth() / (float)fb->GetHeight());
			ImGui::Image((ImTextureID)id, { w, h }, { 0, 1 }, { 1, 0 });
			UI::EndTreeNode();
		}
#endif

		ImGui::End();
	}

}