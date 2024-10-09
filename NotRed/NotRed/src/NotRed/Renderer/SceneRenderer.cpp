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
	struct SceneRendererData
	{
		const Scene* ActiveScene = nullptr;

		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;

			Ref<Environment> SceneEnvironment;
			float SkyboxLod = 0.0f;
			float SceneEnvironmentIntensity;
			LightEnvironment SceneLightEnvironment;
			Light ActiveLight;

		} SceneData;

		Ref<Texture2D> BRDFLUT;
		Ref<Shader> CompositeShader;
		Ref<Shader> BloomBlurShader;
		Ref<Shader> BloomBlendShader;

		Ref<RenderPass> PreDepthRenderPass;
		Ref<RenderPass> GeoPass;
		Ref<RenderPass> CompositePass;
		Ref<RenderPass> BloomBlurPass[2];
		Ref<RenderPass> BloomBlendPass; 

		struct UBCamera
		{
			glm::mat4 ViewProjection;
			glm::mat4 InverseViewProjection;
			glm::mat4 View;
		} 
		CameraData;
		std::vector<Ref<UniformBuffer>> CameraUniformBuffer;

		struct UBShadow
		{
			glm::mat4 ViewProjection[4];
		} 
		ShadowData;
		std::vector<Ref<UniformBuffer>> ShadowUniformBuffer;

		struct Light
		{
			glm::vec3 Direction;
			float Padding = 0.0f;
			glm::vec3 Radiance;
			float Multiplier;
		};
		struct UBScene
		{
			Light lights;
			glm::vec3 uCameraPosition;
		}
		SceneDataUB;
		std::vector<Ref<UniformBuffer>> SceneUniformBuffer;

		struct UBRendererData
		{
			glm::vec4 uCascadeSplits;
			bool ShowCascades = false;
			char Padding0[3];
			bool SoftShadows = true;
			char Padding1[3];
			float LightSize = 0.5f;
			float MaxShadowDistance = 200.0f;
			float ShadowFade;
			bool CascadeFading = true;
			char Padding2[3];
			float CascadeTransitionFade = 1.0f;
		} 
		RendererDataUB;
		std::vector<Ref<UniformBuffer>> RendererDataUniformBuffer;

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

		glm::ivec3 LightCullingWorkGroups;

		Ref<Pipeline> GeometryPipeline;
		Ref<Pipeline> ParticlePipeline;
		Ref<Pipeline> CompositePipeline;
		Ref<Pipeline> ShadowPassPipeline;
		Ref<Material> ShadowPassMaterial;
		Ref<Pipeline> SkyboxPipeline;
		Ref<Pipeline> PreDepthPipeline;

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			Ref<Material> Material;
			glm::mat4 Transform;
		};
		std::vector<DrawCommand> DrawList;
		std::vector<DrawCommand> DrawListParticles;
		std::vector<DrawCommand> SelectedMeshDrawList;
		std::vector<DrawCommand> ColliderDrawList;
		std::vector<DrawCommand> ShadowPassDrawList;

		Ref<Pipeline> GridPipeline;
		Ref<Shader> GridShader;
		Ref<Material> GridMaterial;
		Ref<Material> OutlineMaterial, OutlineAnimMaterial;
		Ref<Material> ColliderMaterial;

		Ref<Material> CompositeMaterial;
		Ref<Material> SkyboxMaterial;
		Ref<Material> LightCullingMaterial;

		SceneRendererOptions Options;

		uint32_t ViewportWidth = 0, ViewportHeight = 0;
		bool NeedsResize = false;
	};

	static SceneRendererData* sData = nullptr;

	void SceneRenderer::Init()
	{
		sData = new SceneRendererData();

		sData->BRDFLUT = Texture2D::Create("assets/textures/BRDF_LUT.tga");

		// Create uniform buffers
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		sData->CameraUniformBuffer.resize(framesInFlight);
		sData->ShadowUniformBuffer.resize(framesInFlight);
		sData->SceneUniformBuffer.resize(framesInFlight);
		sData->RendererDataUniformBuffer.resize(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			sData->CameraUniformBuffer[i] = UniformBuffer::Create(sizeof(SceneRendererData::UBCamera), 0);
			sData->ShadowUniformBuffer[i] = UniformBuffer::Create(sizeof(SceneRendererData::UBShadow), 1);
			sData->SceneUniformBuffer[i] = UniformBuffer::Create(sizeof(SceneRendererData::UBScene), 2);
			sData->RendererDataUniformBuffer[i] = UniformBuffer::Create(sizeof(SceneRendererData::UBRendererData), 3);
			Renderer::SetUniformBuffer(sData->CameraUniformBuffer[i], i, 0);
			Renderer::SetUniformBuffer(sData->ShadowUniformBuffer[i], i, 0);
			Renderer::SetUniformBuffer(sData->SceneUniformBuffer[i], i, 0);
			Renderer::SetUniformBuffer(sData->RendererDataUniformBuffer[i], i, 0);
		}

		sData->LightCullingMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("LightCulling"), "LightCulling");
		sData->CompositeShader = Renderer::GetShaderLibrary()->Get("HDR");
		sData->CompositeMaterial = Material::Create(sData->CompositeShader);
		sData->BRDFLUT = Texture2D::Create("Assets/Textures/BRDF_LUT.tga");

		// Geometry Pass
		{
			FrameBufferSpecification geoFrameBufferSpec;
			geoFrameBufferSpec.Width = 1280;
			geoFrameBufferSpec.Height = 720;
			geoFrameBufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA32F, ImageFormat::Depth };
			geoFrameBufferSpec.Samples = 1;
			geoFrameBufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

			Ref<FrameBuffer> FrameBuffer = FrameBuffer::Create(geoFrameBufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float3, "aNormal" },
				{ ShaderDataType::Float3, "aTangent" },
				{ ShaderDataType::Float3, "aBinormal" },
				{ ShaderDataType::Float2, "aTexCoord" },
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("PBR_Static");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFrameBuffer = FrameBuffer;
			renderPassSpec.DebugName = "Geometry";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "PBR-Static";
			sData->GeometryPipeline = Pipeline::Create(pipelineSpecification);
		}
		{
			FrameBufferSpecification geoFrameBufferSpec;
			geoFrameBufferSpec.Width = 1280;
			geoFrameBufferSpec.Height = 720;
			geoFrameBufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA32F, ImageFormat::Depth };
			geoFrameBufferSpec.Samples = 1;
			geoFrameBufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

			Ref<FrameBuffer> FrameBuffer = FrameBuffer::Create(geoFrameBufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" }
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Particle");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFrameBuffer = FrameBuffer;
			renderPassSpec.DebugName = "Particles";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "Particles";
			sData->ParticlePipeline = Pipeline::Create(pipelineSpecification);
		}

		// Composite Pass
		{
			FrameBufferSpecification compFrameBufferSpec;
			compFrameBufferSpec.Width = 1280;
			compFrameBufferSpec.Height = 720;
			compFrameBufferSpec.Attachments = { ImageFormat::RGBA };
			compFrameBufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

			Ref<FrameBuffer> FrameBuffer = FrameBuffer::Create(compFrameBufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("HDR");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFrameBuffer = FrameBuffer;
			renderPassSpec.DebugName = "Composite";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "HDR";
			sData->CompositePipeline = Pipeline::Create(pipelineSpecification);
		}

		// Shadow Pass
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

			FrameBufferSpecification shadowMapFrameBufferSpec;
			shadowMapFrameBufferSpec.Width = 4096;
			shadowMapFrameBufferSpec.Height = 4096;
			shadowMapFrameBufferSpec.Attachments = { ImageFormat::DEPTH32F };
			shadowMapFrameBufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			shadowMapFrameBufferSpec.Resizable = false;
			shadowMapFrameBufferSpec.ExistingImage = cascadedDepthImage;

			// 4 cascades
			for (int i = 0; i < 4; ++i)
			{
				shadowMapFrameBufferSpec.ExistingImageLayer = i;

				RenderPassSpecification shadowMapRenderPassSpec;
				shadowMapRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(shadowMapFrameBufferSpec);
				shadowMapRenderPassSpec.DebugName = "ShadowMap";
				sData->ShadowMapRenderPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
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
			pipelineSpec.RenderPass = sData->ShadowMapRenderPass[0];
			sData->ShadowPassPipeline = Pipeline::Create(pipelineSpec);

			sData->ShadowPassMaterial = Material::Create(shadowPassShader, "ShadowPass");
		}

		// PreDepth
		{
			FrameBufferSpecification preDepthFramebufferSpec;
			preDepthFramebufferSpec.Width = 1280;
			preDepthFramebufferSpec.Height = 720;
			preDepthFramebufferSpec.Attachments = { ImageFormat::DEPTH32F };
			preDepthFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

			RenderPassSpecification preDepthRenderPassSpec;
			preDepthRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(preDepthFramebufferSpec);
			preDepthRenderPassSpec.DebugName = "PreDepth";

			sData->PreDepthRenderPass = RenderPass::Create(preDepthRenderPassSpec);
			auto PreDepthShader = Renderer::GetShaderLibrary()->Get("PreDepth");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "PreDepthPass";
			pipelineSpec.Shader = PreDepthShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float3, "aNormal" },
				{ ShaderDataType::Float3, "aTangent" },
				{ ShaderDataType::Float3, "aBinormal" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpec.RenderPass = sData->PreDepthRenderPass;
			sData->PreDepthPipeline = Pipeline::Create(pipelineSpec);
		}

		// Grid
		{
			sData->GridShader = Renderer::GetShaderLibrary()->Get("Grid");
			const float gridScale = 16.025f;
			const float gridSize = 0.025f;
			sData->GridMaterial = Material::Create(sData->GridShader);
			sData->GridMaterial->Set("uSettings.Scale", gridScale);
			sData->GridMaterial->Set("uSettings.Size", gridSize);

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Grid";
			pipelineSpec.Shader = sData->GridShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpec.RenderPass = sData->GeometryPipeline->GetSpecification().RenderPass;
			sData->GridPipeline = Pipeline::Create(pipelineSpec);
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
			pipelineSpec.RenderPass = sData->GeometryPipeline->GetSpecification().RenderPass;
			sData->SkyboxPipeline = Pipeline::Create(pipelineSpec);

			sData->SkyboxMaterial = Material::Create(skyboxShader);
			sData->SkyboxMaterial->ModifyFlags(MaterialFlag::DepthTest, false);
		}
	}

	void SceneRenderer::Shutdown()
	{
		delete sData;
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (sData->ViewportWidth != width || sData->ViewportHeight != height)
		{
			sData->ViewportWidth = width;
			sData->ViewportHeight = height;
			sData->NeedsResize = true;
		}
	}

	struct FrustumBounds
	{
		float r, l, b, t, f, n;
	};

	struct CascadeData
	{
		glm::mat4 ViewProj;
		glm::mat4 View;
		float SplitDepth;
	};

	static void CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection)
	{
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
			float d = sData->CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		cascadeSplits[3] = 0.3f;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] =
			{
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3( 1.0f,  1.0f, -1.0f),
				glm::vec3( 1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3( 1.0f,  1.0f,  1.0f),
				glm::vec3( 1.0f, -1.0f,  1.0f),
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

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -lightDirection;
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + sData->CascadeNearPlaneOffset, maxExtents.z - minExtents.z + sData->CascadeFarPlaneOffset);

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

	void SceneRenderer::BeginScene(const Scene* scene, const SceneRendererCamera& camera)
	{
		NR_CORE_ASSERT(!sData->ActiveScene);

		sData->ActiveScene = scene;

		sData->SceneData.SceneCamera = camera;
		sData->SceneData.SceneEnvironment = scene->mEnvironment;
		sData->SceneData.SceneEnvironmentIntensity = scene->mEnvironmentIntensity;
		sData->SceneData.ActiveLight = scene->mLight;
		sData->SceneData.SceneLightEnvironment = scene->mLightEnvironment;
		sData->SceneData.SkyboxLod = scene->mSkyboxLod;

		if (sData->NeedsResize)
		{
			sData->PreDepthPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(sData->ViewportWidth, sData->ViewportHeight);
			sData->GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(sData->ViewportWidth, sData->ViewportHeight);
			sData->ParticlePipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(sData->ViewportWidth, sData->ViewportHeight);
			sData->CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(sData->ViewportWidth, sData->ViewportHeight);
			
			sData->LightCullingWorkGroups = { (sData->ViewportWidth + sData->ViewportWidth % 16) / 16, (sData->ViewportHeight + sData->ViewportHeight % 16) / 16, 1 };

			Renderer::Submit([=]
				{
					auto shader = sData->LightCullingMaterial->GetShader();

					if (RendererAPI::Current() == RendererAPIType::Vulkan)
					{
					}
					else
					{
						shader.As<GLShader>()->ResizeStorageBuffer(14, sData->LightCullingWorkGroups.x * sData->LightCullingWorkGroups.y * 4 * 1024);
					}
				});
		}
		Renderer::SetSceneEnvironment(sData->SceneData.SceneEnvironment, sData->ShadowPassPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetDepthImage());

		auto& sceneCamera = sData->SceneData.SceneCamera;
		auto& viewMatrix = sceneCamera.ViewMatrix;
		auto& projectionMatrix = sceneCamera.CameraObj.GetProjectionMatrix();
		auto viewProjection = sceneCamera.CameraObj.GetProjectionMatrix() * sData->SceneData.SceneCamera.ViewMatrix;
		glm::vec3 cameraPosition = glm::inverse(sceneCamera.ViewMatrix)[3];

		// TODO: handle uniform buffers better
		const DirectionalLight& directionalLight = sData->SceneData.SceneLightEnvironment.DirectionalLights[0];
		Renderer::Submit([viewMatrix, projectionMatrix, viewProjection, sceneCamera, cameraPosition, directionalLight, pointLights = scene->m_LightEnvironment.PointLights]
			{
				{
					auto inverseVP = glm::inverse(viewProjection);
					struct CameraMatrices
					{
						glm::mat4 ViewProjection;
						glm::mat4 InverseViewProjection;
						glm::mat4 ProjectionMatrix;
						glm::mat4 ViewMatrix;
					} matrices;
					matrices = { viewProjection, inverseVP, projectionMatrix, viewMatrix };
					//viewProj.LightMatrixCascade = sData->LightMatrices[0];
					struct DirLight
					{
						glm::vec3 Direction;
						float Padding = 0.0f;
						glm::vec3 Radiance;
						float Multiplier;
					};

					struct SceneData
					{
						DirLight lights;
						glm::vec3 uCameraPosition{};
					};

					struct RendererDataBuffer
					{
						bool ShowCascades;
						bool SoftShadows;
						float LightSize;
						float MaxShadowDistance;
						float ShadowFade;
						bool CascadeFading;
						float CascadeTransitionFade;
						int TilesCountX;
					} rendererDataBuffer;
					rendererDataBuffer = {
						sData->ShowCascades,
						sData->SoftShadows,
						sData->LightSize,
						sData->MaxShadowDistance,
						sData->ShadowFade,
						sData->CascadeFading,
						sData->CascadeTransitionFade,
						sData->LightCullingWorkGroups.x
					};


					const uint32_t pointLightCount = pointLights.size();
					SceneData ub;
					ub.lights.Direction = directionalLight.Direction;
					ub.lights.Radiance = directionalLight.Radiance;
					ub.lights.Multiplier = directionalLight.Multiplier;
					ub.uCameraPosition = cameraPosition;
					if (RendererAPI::Current() == RendererAPIType::Vulkan)
					{
						void* ubPtr = VKShader::MapUniformBuffer(0);
						memcpy(ubPtr, &matrices, sizeof(CameraMatrices));
						VKShader::UnmapUniformBuffer(0);

						ubPtr = VKShader::MapUniformBuffer(2);
						memcpy(ubPtr, &ub, sizeof(SceneData));
						VKShader::UnmapUniformBuffer(2);

						ubPtr = VKShader::MapUniformBuffer(3);
						memcpy(ubPtr, &ub, sizeof(RendererDataBuffer));
						VKShader::UnmapUniformBuffer(3);

						ubPtr = VKShader::MapUniformBuffer(4);
						memcpy(ubPtr, &pointLightCount, 4);
						memcpy(static_cast<byte*>(ubPtr) + 16, pointLights.data(), sizeof(PointLight) * pointLightCount);
						VKShader::UnmapUniformBuffer(4);

					}
					else
					{
						auto shader = sData->GeometryPipeline->GetSpecification().Shader.As<GLShader>();
						shader->SetUniformBuffer("Camera", &matrices, sizeof(CameraMatrices));
						shader->SetUniformBuffer("SceneData", &ub, sizeof(SceneData));
						shader->SetUniformBuffer("RendererData", &rendererDataBuffer, sizeof(RendererDataBuffer));
						const auto& buffer = GLShader::FindUniformBuffer("PointLightData");
						shader->SetUniformBuffer(buffer, &pointLightCount, 16);
						shader->SetUniformBuffer(buffer, pointLights.data(), sizeof(PointLight) * pointLightCount, 16);
					}
				}

				{
					CascadeData cascades[4];
					CalculateCascades(cascades, sceneCamera, directionalLight.Direction);
					sData->LightViewMatrix = cascades[0].View;

					for (int i = 0; i < 1; i++)
					{
						sData->CascadeSplits[i] = cascades[i].SplitDepth;
					}

					struct ShadowData
					{
						glm::mat4 ViewProjection;
					};
					ShadowData shadowData;
					shadowData.ViewProjection = cascades[0].ViewProj;
					if (RendererAPI::Current() == RendererAPIType::Vulkan)
					{
						void* ubPtr = VKShader::MapUniformBuffer(1);
						memcpy(ubPtr, &shadowData, sizeof(ShadowData));
						VKShader::UnmapUniformBuffer(1);
					}
					else
					{
						auto shadowPassShader = sData->ShadowPassPipeline->GetSpecification().Shader;
						auto shader = shadowPassShader.As<GLShader>();
						shader->SetUniformBuffer("ShadowData", &shadowData, sizeof(ShadowData));
					}
				}
			});
	}

	void SceneRenderer::EndScene()
	{
		NR_CORE_ASSERT(sData->ActiveScene);

		sData->ActiveScene = nullptr;

		FlushDrawList();
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		sData->DrawList.push_back({ mesh, overrideMaterial, transform });
		sData->ShadowPassDrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRenderer::SubmitParticles(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		sData->DrawListParticles.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform)
	{
		sData->SelectedMeshDrawList.push_back({ mesh, nullptr, transform });
		sData->ShadowPassDrawList.push_back({ mesh, nullptr, transform });
	}

	void SceneRenderer::SubmitColliderMesh(const BoxColliderComponent& component, const glm::mat4& parentTransform)
	{
		sData->ColliderDrawList.push_back({ component.DebugMesh, nullptr, glm::translate(parentTransform, component.Offset) });
	}

	void SceneRenderer::SubmitColliderMesh(const SphereColliderComponent& component, const glm::mat4& parentTransform)
	{
		sData->ColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::SubmitColliderMesh(const CapsuleColliderComponent& component, const glm::mat4& parentTransform)
	{
		sData->ColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::SubmitColliderMesh(const MeshColliderComponent& component, const glm::mat4& parentTransform)
	{
		for (auto debugMesh : component.ProcessedMeshes)
		{
			sData->ColliderDrawList.push_back({ debugMesh, nullptr, parentTransform });
		}
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> SceneRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return Renderer::CreateEnvironmentMap(filepath);
	}

	void SceneRenderer::DepthPass()
	{
		{
			auto& directionalLights = sData->SceneData.SceneLightEnvironment.DirectionalLights;
			if (directionalLights[0].Multiplier == 0.0f || !directionalLights[0].CastShadows)
			{
				for (int i = 0; i < 4; ++i)
				{
					// Clear shadow maps
					Renderer::BeginRenderPass(sData->ShadowMapRenderPass[i]);
					Renderer::EndRenderPass();
				}
				return;
			}

			for (int i = 0; i < 1; ++i)
			{
				Renderer::BeginRenderPass(sData->ShadowMapRenderPass[i]);
				// static glm::mat4 scaleBiasMatrix = glm::scale(glm::mat4(1.0f), { 0.5f, 0.5f, 0.5f }) * glm::translate(glm::mat4(1.0f), { 1, 1, 1 });
				// Render entities
				for (auto& dc : sData->ShadowPassDrawList)
				{
					Renderer::RenderMesh(sData->ShadowPassPipeline, dc.Mesh, dc.Transform);
				}
				Renderer::EndRenderPass();
			}
		}

		// PreDepth Pass, used for light culling
		{
			Renderer::BeginRenderPass(sData->PreDepthRenderPass);
			for (auto& dc : sData->DrawList)
			{
				Renderer::RenderMesh(sData->PreDepthPipeline, dc.Mesh, dc.Transform);
			}
			for (auto& dc : sData->SelectedMeshDrawList)
			{
				Renderer::RenderMesh(sData->PreDepthPipeline, dc.Mesh, dc.Transform);
			}
		}
	}

	void SceneRenderer::LightCullingPass()
	{
		sData->LightCullingMaterial->Set("uPreDepthMap", sData->PreDepthRenderPass->GetSpecification().TargetFrameBuffer->GetDepthImage());
		sData->LightCullingMaterial->Set("uScreenData.uScreenSize", glm::ivec2{ sData->ViewportWidth, sData->ViewportHeight });


		Renderer::DispatchComputeShader(sData->LightCullingWorkGroups, sData->LightCullingMaterial);
	}

	void SceneRenderer::GeometryPass()
	{
		Renderer::BeginRenderPass(sData->GeometryPipeline->GetSpecification().RenderPass);

		// Skybox
		sData->SkyboxMaterial->Set("uUniforms.TextureLod", sData->SceneData.SkyboxLod);
		const Ref<TextureCube> radianceMap = sData->SceneData.SceneEnvironment ? sData->SceneData.SceneEnvironment->RadianceMap : Renderer::GetBlackCubeTexture();
		sData->SkyboxMaterial->Set("uTexture", radianceMap);
		Renderer::SubmitFullscreenQuad(sData->SkyboxPipeline, sData->SkyboxMaterial);

		// Render entities
		for (auto& dc : sData->DrawList)
		{
			Renderer::RenderMesh(sData->GeometryPipeline, dc.Mesh, dc.Transform);
		}

		for (auto& dc : sData->SelectedMeshDrawList)
		{
			Renderer::RenderMesh(sData->GeometryPipeline, dc.Mesh, dc.Transform);
		}

		for (auto& dc : sData->DrawListParticles)
		{
			Renderer::RenderParticles(sData->ParticlePipeline, dc.Mesh, dc.Transform);
		}

		// Grid
		if (GetOptions().ShowGrid)
		{
			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(8.0f));
			Renderer::RenderQuad(sData->GridPipeline, sData->GridMaterial, transform);
		}

		if (GetOptions().ShowBoundingBoxes)
		{
		}

		Renderer::EndRenderPass();
	}

	void SceneRenderer::CompositePass()
	{
		Renderer::BeginRenderPass(sData->CompositePipeline->GetSpecification().RenderPass);

		auto frameBuffer = sData->GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer;
		float exposure = sData->SceneData.SceneCamera.CameraObj.GetExposure();
		int textureSamples = frameBuffer->GetSpecification().Samples;

		sData->CompositeMaterial->Set("uUniforms.Exposure", exposure);
		sData->CompositeMaterial->Set("uTexture", frameBuffer->GetImage());

		Renderer::SubmitFullscreenQuad(sData->CompositePipeline, sData->CompositeMaterial);
		Renderer::EndRenderPass();
	}

	void SceneRenderer::BloomBlurPass()
	{
	}

	void SceneRenderer::FlushDrawList()
	{
		NR_CORE_ASSERT(!sData->ActiveScene);

		DepthPass();
		LightCullingPass();
		GeometryPass();
		CompositePass();
		BloomBlurPass();

		sData->DrawList.clear();
		sData->DrawListParticles.clear();
		sData->SelectedMeshDrawList.clear();
		sData->ShadowPassDrawList.clear();
		sData->ColliderDrawList.clear();
		sData->SceneData = {};
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return sData->CompositePipeline->GetSpecification().RenderPass;
	}

	Ref<Image2D> SceneRenderer::GetFinalPassImage()
	{
		return sData->CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage();
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return sData->Options;
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
			UI::Property("Soft Shadows", sData->RendererDataUB.SoftShadows); 
			UI::Property("Light Size", sData->RendererDataUB.LightSize, 0.01f); 
			UI::Property("Max Shadow Distance", sData->RendererDataUB.MaxShadowDistance, 1.0f); 
			UI::Property("Shadow Fade", sData->RendererDataUB.ShadowFade, 5.0f);
			UI::EndPropertyGrid();

			if (UI::BeginTreeNode("Cascade Settings"))
			{
				UI::BeginPropertyGrid(); 
				UI::Property("Show Cascades", sData->RendererDataUB.ShowCascades); 
				UI::Property("Cascade Fading", sData->RendererDataUB.CascadeFading); 
				UI::Property("Cascade Transition Fade", sData->RendererDataUB.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
				UI::Property("Cascade Split", sData->CascadeSplitLambda, 0.01f);
				UI::Property("CascadeNearPlaneOffset", sData->CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
				UI::Property("CascadeFarPlaneOffset", sData->CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			if (UI::BeginTreeNode("Shadow Map", false))
			{
				static int cascadeIndex = 0;
				auto fb = sData->ShadowMapRenderPass[cascadeIndex]->GetSpecification().TargetFrameBuffer;
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

		ImGui::End();
	}
}