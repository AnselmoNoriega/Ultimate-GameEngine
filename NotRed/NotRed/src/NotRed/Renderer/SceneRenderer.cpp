#include "nrpch.h"
#include "SceneRenderer.h"

#include "Renderer.h"
#include "SceneEnvironment.h"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer2D.h"
#include "UniformBuffer.h"
#include "NotRed/Math/Noise.h"

#include "NotRed/ImGui/ImGui.h"
#include "NotRed/Debug/Profiler.h"

#include "NotRed/Platform/Vulkan/VKComputePipeline.h"
#include "NotRed/Platform/Vulkan/VKMaterial.h"
#include "NotRed/Platform/Vulkan/VKRenderer.h"
#include "NotRed/Platform/Vulkan/VKImage.h"


enum Binding : uint32_t
{
	Camera = 0,
	ShadowData = 1,

	RendererData = 3,
	SceneData = 4,
	PointLightData = 5,
	AlbedoTexture = 6,
	NormalTexture = 7,
	MetalnessTexture = 8,
	RoughnessTexture = 9,
	EnvRadianceTex = 10,
	EnvIrradianceTex = 11,
	BRDFLUTTexture = 12,
	ShadowMapTexture = 13,
	VisibleLightIndicesBuffer = 14,
	LinearDepthTex = 15,
	ScreenData = 16,
	HBAOData = 17
};

namespace NR
{
	static std::vector<std::thread> sThreadPool;

	SceneRenderer::SceneRenderer(Ref<Scene> scene, SceneRendererSpecification specification)
		: mScene(scene), mSpecification(specification)
	{
		Init();
	}


	void SceneRenderer::Init()
	{
		NR_SCOPE_TIMER("SceneRenderer::Init");

		if (mSpecification.SwapChainTarget)
		{
			mCommandBuffer = RenderCommandBuffer::CreateFromSwapChain("SceneRenderer");
		}
		else
		{
			mCommandBuffer = RenderCommandBuffer::Create(0, "SceneRenderer");
		}

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		mUniformBufferSet = UniformBufferSet::Create(framesInFlight);
		mUniformBufferSet->Create(sizeof(UBCamera), Binding::Camera);
		mUniformBufferSet->Create(sizeof(UBShadow), Binding::ShadowData);
		mUniformBufferSet->Create(sizeof(UBRendererData), Binding::RendererData);
		mUniformBufferSet->Create(sizeof(UBScene), Binding::SceneData);
		mUniformBufferSet->Create(sizeof(UBPointLights), Binding::PointLightData);
		mUniformBufferSet->Create(sizeof(UBScreenData), Binding::ScreenData);
		mUniformBufferSet->Create(sizeof(UBHBAOData), Binding::HBAOData);

		mCompositeShader = Renderer::GetShaderLibrary()->Get("SceneComposite");
		CompositeMaterial = Material::Create(mCompositeShader);

		//Light culling compute pipeline
		{
			mLightCullingWorkGroups = { (mViewportWidth + mViewportWidth % 16) / 16,(mViewportHeight + mViewportHeight % 16) / 16, 1 };

			mStorageBufferSet = StorageBufferSet::Create(framesInFlight);
			mStorageBufferSet->Create(1, 14); //Can't allocate 0 bytes.. Resized later

			mLightCullingMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("LightCulling"), "LightCulling");
			Ref<Shader> lightCullingShader = Renderer::GetShaderLibrary()->Get("LightCulling");
			mLightCullingPipeline = PipelineCompute::Create(lightCullingShader);
		}

		VertexBufferLayout vertexLayout = {
			{ ShaderDataType::Float3, "aPosition" },
			{ ShaderDataType::Float3, "aNormal" },
			{ ShaderDataType::Float3, "aTangent" },
			{ ShaderDataType::Float3, "aBinormal" },
			{ ShaderDataType::Float2, "aTexCoord" }
		};

		VertexBufferLayout instanceLayout = {
			{ ShaderDataType::Float4, "aMRow0" },
			{ ShaderDataType::Float4, "aMRow1" },
			{ ShaderDataType::Float4, "aMRow2" },
		};

		VertexBufferLayout boneInfluenceLayout = {
			{ ShaderDataType::Int4,   "aBoneIDs" },
			{ ShaderDataType::Float4, "aBoneWeights" },
		};

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

			FrameBufferSpecification shadowMapFrameBufferSpec;
			shadowMapFrameBufferSpec.DebugName = "Shadow Map";
			shadowMapFrameBufferSpec.Width = 4096;
			shadowMapFrameBufferSpec.Height = 4096;
			shadowMapFrameBufferSpec.Attachments = { ImageFormat::DEPTH32F };
			shadowMapFrameBufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			shadowMapFrameBufferSpec.Resizable = false;
			shadowMapFrameBufferSpec.ExistingImage = cascadedDepthImage;

			// 4 cascades
			auto shadowPassShader = Renderer::GetShaderLibrary()->Get("ShadowMap");
			auto shadowPassShaderAnim = Renderer::GetShaderLibrary()->Get("ShadowMap_Anim");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "ShadowPass";
			pipelineSpec.Shader = shadowPassShader;
			pipelineSpec.Layout = vertexLayout;
			pipelineSpec.InstanceLayout = instanceLayout;
			PipelineSpecification pipelineSpecAnim;
			pipelineSpecAnim.DebugName = "ShadowPass-Anim";
			pipelineSpecAnim.Shader = shadowPassShaderAnim;
			pipelineSpecAnim.Layout = vertexLayout;
			pipelineSpecAnim.InstanceLayout = instanceLayout;
			pipelineSpecAnim.BoneInfluencesLayout = boneInfluenceLayout;

			// 4 cascades
			for (int i = 0; i < 4; i++)
			{
				shadowMapFrameBufferSpec.ExistingImageLayers.clear();
				shadowMapFrameBufferSpec.ExistingImageLayers.emplace_back(i);

				RenderPassSpecification shadowMapRenderPassSpec;
				shadowMapRenderPassSpec.DebugName = shadowMapFrameBufferSpec.DebugName;
				shadowMapRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(shadowMapFrameBufferSpec);
				auto renderpass = RenderPass::Create(shadowMapRenderPassSpec);

				pipelineSpec.RenderPass = renderpass;
				mShadowPassPipelines[i] = Pipeline::Create(pipelineSpec);

				pipelineSpecAnim.RenderPass = renderpass;
				mShadowPassPipelinesAnim[i] = Pipeline::Create(pipelineSpecAnim);
			}
			mShadowPassMaterial = Material::Create(pipelineSpec.Shader, pipelineSpec.DebugName);
		}

		// PreDepth
		{
			FrameBufferSpecification preDepthFrameBufferSpec;
			preDepthFrameBufferSpec.DebugName = "PreDepth";
			preDepthFrameBufferSpec.Attachments = { ImageFormat::RED32F, ImageFormat::DEPTH32F };
			preDepthFrameBufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

			RenderPassSpecification preDepthRenderPassSpec;
			preDepthRenderPassSpec.DebugName = preDepthFrameBufferSpec.DebugName;
			preDepthRenderPassSpec.TargetFrameBuffer = FrameBuffer::Create(preDepthFrameBufferSpec);

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "PreDepth";

			pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("PreDepth");
			pipelineSpec.Layout = vertexLayout;
			pipelineSpec.InstanceLayout = instanceLayout;
			pipelineSpec.RenderPass = RenderPass::Create(preDepthRenderPassSpec);
			mPreDepthPipeline = Pipeline::Create(pipelineSpec);
			mPreDepthMaterial = Material::Create(pipelineSpec.Shader, pipelineSpec.DebugName);

			pipelineSpec.DebugName = "PreDepth_Anim";
			pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("PreDepth_Anim");
			pipelineSpec.BoneInfluencesLayout = boneInfluenceLayout;
			mPreDepthPipelineAnim = Pipeline::Create(pipelineSpec);
		}

		// Geometry
		{
			FrameBufferSpecification geoFrameBufferSpec;
			geoFrameBufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA16F, ImageFormat::RGBA16F, ImageFormat::Depth };
			geoFrameBufferSpec.Samples = 1;
			geoFrameBufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			geoFrameBufferSpec.DebugName = "Geometry";
			Ref<FrameBuffer> frameBuffer = FrameBuffer::Create(geoFrameBufferSpec);

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = geoFrameBufferSpec.DebugName;
			renderPassSpec.TargetFrameBuffer = frameBuffer;
			Ref<RenderPass> renderPass = RenderPass::Create(renderPassSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "PBR-Static";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("PBR_Static");
			pipelineSpecification.Layout = vertexLayout;
			pipelineSpecification.InstanceLayout = instanceLayout;
			pipelineSpecification.LineWidth = mLineWidth;
			pipelineSpecification.RenderPass = renderPass;
			mGeometryPipeline = Pipeline::Create(pipelineSpecification);

			pipelineSpecification.DebugName = "PBR-Anim";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("PBR_Anim");
			pipelineSpecification.BoneInfluencesLayout = boneInfluenceLayout;
			mGeometryPipelineAnim = Pipeline::Create(pipelineSpecification); // Note: same frameBuffer and renderpass as mGeometryPipeline
		}

		// Selected Geometry isolation (for outline pass)
		{
			FrameBufferSpecification frameBufferSpec;
			frameBufferSpec.DebugName = "SelectedGeometry";
			frameBufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
			frameBufferSpec.Samples = 1;
			frameBufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			Ref<FrameBuffer> frameBuffer = FrameBuffer::Create(frameBufferSpec);

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = frameBufferSpec.DebugName;
			renderPassSpec.TargetFrameBuffer = FrameBuffer::Create(frameBufferSpec);
			Ref<RenderPass> renderPass = RenderPass::Create(renderPassSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "SelectedGeometry";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SelectedGeometry");
			pipelineSpecification.Layout = vertexLayout;
			pipelineSpecification.InstanceLayout = instanceLayout;
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			mSelectedGeometryPipeline = Pipeline::Create(pipelineSpecification);
			mSelectedGeometryMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);

			pipelineSpecification.DebugName = "SelectedGeometry-Anim";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SelectedGeometry_Anim");
			pipelineSpecification.BoneInfluencesLayout = boneInfluenceLayout;
			mSelectedGeometryPipelineAnim = Pipeline::Create(pipelineSpecification); // Note: same frameBuffer and renderpass as mSelectedGeometryPipeline
		}

		// Bloom Compute
		{
			auto shader = Renderer::GetShaderLibrary()->Get("Bloom");
			mBloomComputePipeline = PipelineCompute::Create(shader);
			TextureProperties props;
			props.SamplerWrap = TextureWrap::Clamp;
			props.Storage = true;
			props.DebugName = "BloomCompute-0";
			mBloomComputeTextures[0] = Texture2D::Create(ImageFormat::RGBA32F, 1, 1, nullptr, props);
			props.DebugName = "BloomCompute-1";
			mBloomComputeTextures[1] = Texture2D::Create(ImageFormat::RGBA32F, 1, 1, nullptr, props);
			props.DebugName = "BloomCompute-2";
			mBloomComputeTextures[2] = Texture2D::Create(ImageFormat::RGBA32F, 1, 1, nullptr, props);
			mBloomComputeMaterial = Material::Create(shader);

			mBloomDirtTexture = Renderer::GetBlackTexture();
		}

		// Deinterleaving
		{
			ImageSpecification imageSpec;
			imageSpec.Format = ImageFormat::RED32F;
			imageSpec.Layers = 16;
			imageSpec.Usage = ImageUsage::Attachment;
			imageSpec.Deinterleaved = true;
			Ref<Image2D> image = Image2D::Create(imageSpec);
			image->Invalidate();
			image->CreatePerLayerImageViews();

			FrameBufferSpecification deinterleavingFrameBufferSpec;
			deinterleavingFrameBufferSpec.Attachments = { ImageFormat::RED32F, ImageFormat::RED32F, ImageFormat::RED32F, ImageFormat::RED32F, ImageFormat::RED32F, ImageFormat::RED32F, ImageFormat::RED32F, ImageFormat::RED32F, };
			deinterleavingFrameBufferSpec.ClearColor = { 1.0f, 0.0f, 0.0f, 1.0f };
			deinterleavingFrameBufferSpec.DebugName = "Deinterleaving";
			deinterleavingFrameBufferSpec.ExistingImage = image;
			//deinterleavingFrameBufferSpec.ClearOnLoad = false;

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("Deinterleaving");
			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Deinterleaving";

			pipelineSpec.Shader = shader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};

			// 2 frame buffers, 2 render passes .. 8 attachments each
			for (int rp = 0; rp < 2; rp++)
			{
				deinterleavingFrameBufferSpec.ExistingImageLayers.clear();
				for (int layer = 0; layer < 8; layer++)
					deinterleavingFrameBufferSpec.ExistingImageLayers.emplace_back(rp * 8 + layer);

				Ref<FrameBuffer> frameBuffer = FrameBuffer::Create(deinterleavingFrameBufferSpec);

				RenderPassSpecification deinterleavingRenderPassSpec;
				deinterleavingRenderPassSpec.TargetFrameBuffer = frameBuffer;
				deinterleavingRenderPassSpec.DebugName = "Deinterleaving";
				pipelineSpec.RenderPass = RenderPass::Create(deinterleavingRenderPassSpec);

				mDeinterleavingPipelines[rp] = Pipeline::Create(pipelineSpec);
			}
			mDeinterleavingMaterial = Material::Create(pipelineSpec.Shader, pipelineSpec.DebugName);

		}

		// HBAO
		{
			ImageSpecification imageSpec;
			imageSpec.Format = ImageFormat::RG16F;
			imageSpec.Usage = ImageUsage::Storage;
			imageSpec.Layers = 16;
			imageSpec.DebugName = "HBAO-Output";
			Ref<Image2D> image = Image2D::Create(imageSpec);
			image->Invalidate();
			image->CreatePerLayerImageViews();

			mHBAOOutputImage = image;

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("HBAO");

			mHBAOPipeline = Ref<VKComputePipeline>::Create(shader);
			mHBAOMaterial = Material::Create(shader, "HBAO");

			for (int i = 0; i < 16; i++)
				HBAODataUB.Float2Offsets[i] = glm::vec4((float)(i % 4) + 0.5f, (float)(i / 4) + 0.5f, 0.0f, 1.f);
			std::memcpy(HBAODataUB.Jitters, Noise::HBAOJitter().data(), sizeof glm::vec4 * 16);
		}

		// Reinterleaving
		{
			FrameBufferSpecification reinterleavingFrameBufferSpec;
			reinterleavingFrameBufferSpec.Attachments = { ImageFormat::RG16F, };
			reinterleavingFrameBufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			reinterleavingFrameBufferSpec.DebugName = "Reinterleaving";

			Ref<FrameBuffer> frameBuffer = FrameBuffer::Create(reinterleavingFrameBufferSpec);
			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("Reinterleaving");
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" },
			};
			pipelineSpecification.BackfaceCulling = false;


			pipelineSpecification.Shader = shader;

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFrameBuffer = frameBuffer;
			renderPassSpec.DebugName = "Reinterleaving";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "Reinterleaving";
			pipelineSpecification.DepthWrite = false;
			mReinterleavingPipeline = Pipeline::Create(pipelineSpecification);

			mReinterleavingMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
		}

		//HBAO Blur
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DepthWrite = false;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" },
			};
			pipelineSpecification.BackfaceCulling = false;

			//HBAO first blur pass
			{
				FrameBufferSpecification hbaoBlurFrameBufferSpec;
				hbaoBlurFrameBufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
				hbaoBlurFrameBufferSpec.Attachments.Attachments.emplace_back(ImageFormat::RG16F);
				hbaoBlurFrameBufferSpec.DebugName = "HBAOBlur";

				RenderPassSpecification renderPassSpec;
				renderPassSpec.TargetFrameBuffer = FrameBuffer::Create(hbaoBlurFrameBufferSpec);
				renderPassSpec.DebugName = hbaoBlurFrameBufferSpec.DebugName;
				pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);

				auto shader = Renderer::GetShaderLibrary()->Get("HBAOBlur");
				pipelineSpecification.Shader = shader;
				pipelineSpecification.DebugName = "HBAOBlur";
				mHBAOBlurPipelines[0] = Pipeline::Create(pipelineSpecification);
				mHBAOBlurMaterials[0] = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
			}

			//HBAO second blur pass
			{
				FrameBufferSpecification hbaoBlurFrameBufferSpec;
				hbaoBlurFrameBufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
				hbaoBlurFrameBufferSpec.Attachments.Attachments.emplace_back(ImageFormat::RGBA32F);
				hbaoBlurFrameBufferSpec.DebugName = "HBAOBlur";
				hbaoBlurFrameBufferSpec.ClearOnLoad = false;
				hbaoBlurFrameBufferSpec.ExistingImages[0] = mGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage(0);
				hbaoBlurFrameBufferSpec.BlendMode = FrameBufferBlendMode::Zero_SrcColor;

				RenderPassSpecification renderPassSpec;
				renderPassSpec.TargetFrameBuffer = FrameBuffer::Create(hbaoBlurFrameBufferSpec);
				renderPassSpec.DebugName = hbaoBlurFrameBufferSpec.DebugName;
				pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);

				auto shader = Renderer::GetShaderLibrary()->Get("HBAOBlur2");
				pipelineSpecification.Shader = shader;
				pipelineSpecification.DebugName = "HBAOBlur2";
				mHBAOBlurPipelines[1] = Pipeline::Create(pipelineSpecification);
				mHBAOBlurMaterials[1] = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
			}
		}

		// Composite
		{
			FrameBufferSpecification compFrameBufferSpec;
			compFrameBufferSpec.DebugName = "SceneComposite";
			compFrameBufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			compFrameBufferSpec.SwapChainTarget = mSpecification.SwapChainTarget;
			// No depth for swapchain
			if (mSpecification.SwapChainTarget)
				compFrameBufferSpec.Attachments = { ImageFormat::RGBA };
			else
				compFrameBufferSpec.Attachments = { ImageFormat::RGBA, ImageFormat::Depth };

			Ref<FrameBuffer> frameBuffer = FrameBuffer::Create(compFrameBufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SceneComposite");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFrameBuffer = frameBuffer;
			renderPassSpec.DebugName = "SceneComposite";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "SceneComposite";
			pipelineSpecification.DepthWrite = false;
			mCompositePipeline = Pipeline::Create(pipelineSpecification);
		}

		// DOF
		{
			FrameBufferSpecification compFrameBufferSpec;
			compFrameBufferSpec.DebugName = "POST-DepthOfField";
			compFrameBufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			// No depth for swapchain
			if (mSpecification.SwapChainTarget)
				compFrameBufferSpec.Attachments = { ImageFormat::RGBA };
			else
				compFrameBufferSpec.Attachments = { ImageFormat::RGBA, ImageFormat::Depth };

			Ref<FrameBuffer> frameBuffer = FrameBuffer::Create(compFrameBufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("DOF");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFrameBuffer = frameBuffer;
			renderPassSpec.DebugName = compFrameBufferSpec.DebugName;
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = compFrameBufferSpec.DebugName;
			pipelineSpecification.DepthWrite = false;
			mDOFPipeline = Pipeline::Create(pipelineSpecification);
			mDOFMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
		}

		// External compositing
		if (!mSpecification.SwapChainTarget)
		{
			FrameBufferSpecification extCompFrameBufferSpec;
			extCompFrameBufferSpec.DebugName = "External Composite";
			extCompFrameBufferSpec.Attachments = { ImageFormat::RGBA, ImageFormat::Depth };
			extCompFrameBufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			extCompFrameBufferSpec.ClearOnLoad = false;
			// Use the color buffer from the final compositing pass, but the depth buffer from
			// the actual 3D geometry pass, in case we want to composite elements behind meshes
			// in the scene
			extCompFrameBufferSpec.ExistingImages[0] = mCompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage();
			extCompFrameBufferSpec.ExistingImages[1] = mGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetDepthImage();
			Ref<FrameBuffer> frameBuffer = FrameBuffer::Create(extCompFrameBufferSpec);

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = extCompFrameBufferSpec.DebugName;
			renderPassSpec.TargetFrameBuffer = frameBuffer;
			mExternalCompositeRenderPass = RenderPass::Create(renderPassSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Wireframe";
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Wireframe = true;
			pipelineSpecification.DepthTest = true;
			pipelineSpecification.LineWidth = 2.0f;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Wireframe");
			pipelineSpecification.Layout = vertexLayout;
			pipelineSpecification.InstanceLayout = instanceLayout;
			pipelineSpecification.RenderPass = mExternalCompositeRenderPass;
			mGeometryWireframePipeline = Pipeline::Create(pipelineSpecification);

			pipelineSpecification.DepthTest = false;
			pipelineSpecification.DebugName = "Wireframe-OnTop";
			mGeometryWireframeOnTopPipeline = Pipeline::Create(pipelineSpecification);

			pipelineSpecification.DepthTest = true;
			pipelineSpecification.DebugName = "Wireframe-Anim";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Wireframe_Anim");
			pipelineSpecification.BoneInfluencesLayout = boneInfluenceLayout;
			mGeometryWireframePipelineAnim = Pipeline::Create(pipelineSpecification); // Note: same frameBuffer and renderpass as mGeometryWireframePipeline

			pipelineSpecification.DepthTest = false;
			pipelineSpecification.DebugName = "Wireframe-Anim-OnTop";
			mGeometryWireframeOnTopPipelineAnim = Pipeline::Create(pipelineSpecification);

		}

		// Temporary frameBuffers for re-use
		{
			FrameBufferSpecification frameBufferSpec;
			frameBufferSpec.Attachments = { ImageFormat::RGBA32F };
			frameBufferSpec.Samples = 1;
			frameBufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			frameBufferSpec.BlendMode = FrameBufferBlendMode::OneZero;

			for (uint32_t i = 0; i < 2; i++)
				mTempFrameBuffers.emplace_back(FrameBuffer::Create(frameBufferSpec));
		}

		// Jump Flood (outline)
		{
			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = "JumpFlood-Init";
			renderPassSpec.TargetFrameBuffer = mTempFrameBuffers[0];

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = renderPassSpec.DebugName;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Init");
			mJumpFloodInitPipeline = Pipeline::Create(pipelineSpecification);
			mJumpFloodInitMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);

			const char* passName[2] = { "EvenPass", "OddPass" };
			for (uint32_t i = 0; i < 2; i++)
			{
				renderPassSpec.TargetFrameBuffer = mTempFrameBuffers[(i + 1) % 2];
				renderPassSpec.DebugName = fmt::format("JumpFlood-{0}", passName[i]);

				pipelineSpecification.DebugName = renderPassSpec.DebugName;
				pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Pass");
				mJumpFloodPassPipeline[i] = Pipeline::Create(pipelineSpecification);
				mJumpFloodPassMaterial[i] = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
			}

			// Outline compositing
			{
				pipelineSpecification.RenderPass = mCompositePipeline->GetSpecification().RenderPass;
				pipelineSpecification.DebugName = "JumpFlood-Composite";
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Composite");
				pipelineSpecification.DepthTest = false;
				mJumpFloodCompositePipeline = Pipeline::Create(pipelineSpecification);
				mJumpFloodCompositeMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
			}
		}

		// Grid
		{
			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Grid";
			pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("Grid");
			pipelineSpec.BackfaceCulling = false;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float2, "aTexCoord" }
			};
			pipelineSpec.RenderPass = mGeometryPipeline->GetSpecification().RenderPass;
			mGridPipeline = Pipeline::Create(pipelineSpec);

			const float gridScale = 16.025f;
			const float gridSize = 0.025f;
			mGridMaterial = Material::Create(pipelineSpec.Shader, pipelineSpec.DebugName);
			mGridMaterial->Set("uSettings.Scale", gridScale);
			mGridMaterial->Set("uSettings.Size", gridSize);
		}

		// Collider
		//auto colliderShader = Shader::Create("assets/shaders/Collider.glsl");
		//ColliderMaterial = Material::Create(Material::Create(colliderShader));
		//ColliderMaterial->ModifyFlags(MaterialFlag::DepthTest, false);

		mWireframeMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Wireframe"), "Wireframe");
		mWireframeMaterial->Set("uMaterialUniforms.Color", glm::vec4{ 1.0f, 0.5f, 0.0f, 1.0f });

		mColliderMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Wireframe"), "Collider");
		mColliderMaterial->Set("uMaterialUniforms.Color", mOptions.PhysicsCollidersColor);

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
			mSkyboxMaterial = Material::Create(pipelineSpec.Shader, pipelineSpec.DebugName);
			mSkyboxMaterial->ModifyFlags(MaterialFlag::DepthTest, false);
		}

		// TODO(Yan): resizeable/flushable
		const size_t TransformBufferCount = 10 * 1024; // 10240 transforms
		mSubmeshTransformBuffer = VertexBuffer::Create(sizeof(TransformVertexData) * TransformBufferCount);
		mTransformVertexData = new TransformVertexData[TransformBufferCount];

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
			mInvViewportWidth = 1.f / (float)width;
			mInvViewportHeight = 1.f / (float)height;
			mNeedsResize = true;
		}
	}

	void SceneRenderer::updateHBAOData()
	{
		const auto& opts = mOptions;
		UBHBAOData& hbaoData = HBAODataUB;

		// radius
		const float meters2viewSpace = 1.0f;
		const float R = opts.HBAORadius * meters2viewSpace;
		const float R2 = R * R;
		hbaoData.NegInvR2 = -1.0f / R2;
		hbaoData.InvQuarterResolution = 1.f / glm::vec2{ (float)mViewportWidth / 4, (float)mViewportHeight / 4 };
		hbaoData.RadiusToScreen = R * 0.5f * (float)mViewportHeight / (tanf(glm::radians(mSceneData.SceneCamera.FOV) * 0.5f) * 2.0f); //FOV is hard coded

		const float* P = glm::value_ptr(mSceneData.SceneCamera.Camera.GetProjectionMatrix());
		const glm::vec4 projInfoPerspective = {
				 2.0f / (P[4 * 0 + 0]),                  // (x) * (R - L)/N
				 2.0f / (P[4 * 1 + 1]),                  // (y) * (T - B)/N
				-(1.0f - P[4 * 2 + 0]) / P[4 * 0 + 0],  // L/N
				-(1.0f + P[4 * 2 + 1]) / P[4 * 1 + 1],  // B/N
		};
		hbaoData.PerspectiveInfo = projInfoPerspective;
		hbaoData.IsOrtho = false; //TODO: change depending on camera
		hbaoData.PowExponent = glm::max(opts.HBAOIntensity, 0.f);
		hbaoData.NDotVBias = glm::min(std::max(0.f, opts.HBAOBias), 1.f);
		hbaoData.AOMultiplier = 1.f / (1.f - hbaoData.NDotVBias);
	}

	void SceneRenderer::BeginScene(const SceneRendererCamera& camera)
	{
		NR_PROFILE_FUNC();

		NR_CORE_ASSERT(mScene);
		NR_CORE_ASSERT(!mActive);
		mActive = true;

		if (!mResourcesCreated)
			return;

		mSceneData.SceneCamera = camera;
		mSceneData.SceneEnvironment = mScene->mEnvironment;
		mSceneData.SceneEnvironmentIntensity = mScene->mEnvironmentIntensity;
		mSceneData.ActiveLight = mScene->mLight;
		mSceneData.SceneLightEnvironment = mScene->mLightEnvironment;
		mSceneData.SkyboxLod = mScene->mSkyboxLod;
		mSceneData.ActiveLight = mScene->mLight;

		if (mNeedsResize)
		{
			mNeedsResize = false;

			mGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(mViewportWidth, mViewportHeight);

			uint32_t quarterWidth = (uint32_t)glm::ceil((float)mViewportWidth / 4.0f);
			uint32_t quarterHeight = (uint32_t)glm::ceil((float)mViewportHeight / 4.0f);
			mDeinterleavingPipelines[0]->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(quarterWidth, quarterHeight);
			mDeinterleavingPipelines[1]->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(quarterWidth, quarterHeight);
			mHBAOWorkGroups.x = (uint32_t)glm::ceil((float)quarterWidth / 16.0f);
			mHBAOWorkGroups.y = (uint32_t)glm::ceil((float)quarterHeight / 16.0f);
			mHBAOWorkGroups.z = 16;
			auto& spec = mHBAOOutputImage->GetSpecification();
			spec.Width = quarterWidth;
			spec.Height = quarterHeight;
			mHBAOOutputImage->Invalidate();
			mHBAOOutputImage->CreatePerLayerImageViews();
			mReinterleavingPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(mViewportWidth, mViewportHeight);
			mHBAOBlurPipelines[0]->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(mViewportWidth, mViewportHeight);
			mHBAOBlurPipelines[1]->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(mViewportWidth, mViewportHeight); //Only resize after geometry frameBuffer

			mPreDepthPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(mViewportWidth, mViewportHeight);
			mCompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->Resize(mViewportWidth, mViewportHeight);

			// Half-size bloom texture
			{
				uint32_t viewportWidth = mViewportWidth / 2;
				uint32_t viewportHeight = mViewportHeight / 2;
				viewportWidth += (mBloomComputeWorkgroupSize - (viewportWidth % mBloomComputeWorkgroupSize));
				viewportHeight += (mBloomComputeWorkgroupSize - (viewportHeight % mBloomComputeWorkgroupSize));
				mBloomComputeTextures[0]->Resize(viewportWidth, viewportHeight);
				mBloomComputeTextures[1]->Resize(viewportWidth, viewportHeight);
				mBloomComputeTextures[2]->Resize(viewportWidth, viewportHeight);
			}

			for (auto& tempFB : mTempFrameBuffers)
				tempFB->Resize(mViewportWidth, mViewportHeight);

			if (mExternalCompositeRenderPass)
				mExternalCompositeRenderPass->GetSpecification().TargetFrameBuffer->Resize(mViewportWidth, mViewportHeight);

			if (mSpecification.SwapChainTarget)
				mCommandBuffer = RenderCommandBuffer::CreateFromSwapChain("SceneRenderer");

			mLightCullingWorkGroups = { (mViewportWidth + mViewportWidth % 16) / 16,(mViewportHeight + mViewportHeight % 16) / 16, 1 };
			RendererDataUB.TilesCountX = mLightCullingWorkGroups.x;

			mStorageBufferSet->Resize(14, 0, mLightCullingWorkGroups.x * mLightCullingWorkGroups.y * 4096);
		}

		// Update uniform buffers
		UBCamera& cameraData = CameraDataUB;
		UBScene& sceneData = SceneDataUB;
		UBShadow& shadowData = ShadowData;
		UBRendererData& rendererData = RendererDataUB;
		UBPointLights& pointLightData = PointLightsUB;
		UBHBAOData& hbaoData = HBAODataUB;
		UBScreenData& screenData = ScreenDataUB;

		auto& sceneCamera = mSceneData.SceneCamera;
		const auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;
		const glm::vec3 cameraPosition = glm::inverse(sceneCamera.ViewMatrix)[3];

		const auto inverseVP = glm::inverse(viewProjection);
		cameraData.ViewProjection = viewProjection;
		cameraData.InverseViewProjection = inverseVP;
		cameraData.Projection = sceneCamera.Camera.GetProjectionMatrix();
		cameraData.View = sceneCamera.ViewMatrix;
		Ref<SceneRenderer> instance = this;
		Renderer::Submit([instance, cameraData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->mUniformBufferSet->Get(Binding::Camera, 0, bufferIndex)->RT_SetData(&cameraData, sizeof(cameraData));
			});

		const std::vector<PointLight>& pointLightsVec = mSceneData.SceneLightEnvironment.PointLights;
		pointLightData.Count = uint32_t(pointLightsVec.size());
		std::memcpy(pointLightData.PointLights, pointLightsVec.data(), sizeof PointLight * pointLightsVec.size()); //(Karim) Do we really have to copy that?
		Renderer::Submit([instance, &pointLightData]() mutable
			{
				const uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				Ref<UniformBuffer> bufferSet = instance->mUniformBufferSet->Get(Binding::PointLightData, 0, bufferIndex);
				bufferSet->RT_SetData(&pointLightData, 16ull + sizeof PointLight * pointLightData.Count);
			});

		const auto& directionalLight = mSceneData.SceneLightEnvironment.DirectionalLights[0];
		sceneData.lights.Direction = directionalLight.Direction;
		sceneData.lights.Radiance = directionalLight.Radiance;
		sceneData.lights.Multiplier = directionalLight.Multiplier;
		sceneData.uCameraPosition = cameraPosition;
		sceneData.EnvironmentMapIntensity = mSceneData.SceneEnvironmentIntensity;
		Renderer::Submit([instance, sceneData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->mUniformBufferSet->Get(Binding::SceneData, 0, bufferIndex)->RT_SetData(&sceneData, sizeof(sceneData));
			});

		updateHBAOData();

		Renderer::Submit([instance, hbaoData]() mutable
			{
				const uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->mUniformBufferSet->Get(Binding::HBAOData, 0, bufferIndex)->RT_SetData(&hbaoData, sizeof(hbaoData));
			});

		screenData.FullResolution = { mViewportWidth, mViewportHeight };
		screenData.InvFullResolution = { mInvViewportWidth, mInvViewportHeight };
		Renderer::Submit([instance, screenData]() mutable
			{
				const uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->mUniformBufferSet->Get(Binding::ScreenData, 0, bufferIndex)->RT_SetData(&screenData, sizeof(screenData));
			});

		CascadeData cascades[4];
		CalculateCascades(cascades, sceneCamera, directionalLight.Direction);

		// TODO: four cascades for now
		for (int i = 0; i < 4; i++)
		{
			CascadeSplits[i] = cascades[i].SplitDepth;
			shadowData.ViewProjection[i] = cascades[i].ViewProj;
		}
		Renderer::Submit([instance, shadowData]() mutable
			{
				const uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->mUniformBufferSet->Get(Binding::ShadowData, 0, bufferIndex)->RT_SetData(&shadowData, sizeof(shadowData));
			});

		rendererData.CascadeSplits = CascadeSplits;
		Renderer::Submit([instance, rendererData]() mutable
			{
				const uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->mUniformBufferSet->Get(Binding::RendererData, 0, bufferIndex)->RT_SetData(&rendererData, sizeof(rendererData));
			});

		Renderer::SetSceneEnvironment(this, mSceneData.SceneEnvironment, mShadowPassPipelines[0]->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetDepthImage(),
			mPreDepthPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage());
	}

	void SceneRenderer::EndScene()
	{
		NR_PROFILE_FUNC();

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
		for (uint32_t i = 0; i < sThreadPool.size(); i++)
			sThreadPool[i].join();

		sThreadPool.clear();
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		NR_PROFILE_FUNC();

		// TODO: Culling, sorting, etc.

		const auto& submeshes = mesh->GetMeshSource()->GetSubmeshes();
		uint32_t materialIndex = submeshes[submeshIndex].MaterialIndex;
		AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex)->Handle : mesh->GetMaterials()->GetMaterial(materialIndex)->Handle;

		MeshKey meshKey = { mesh->Handle, materialHandle, submeshIndex };
		auto& transformStorage = mMeshTransformMap[meshKey].Transforms.emplace_back();

		transformStorage.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		transformStorage.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		transformStorage.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };


		// Main geo
		{
			auto& dc = mDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
		}

		// Shadow pass
		{
			auto& dc = mShadowPassDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::SubmitStaticMesh(Ref<StaticMesh> staticMesh, Ref<MaterialTable> materialTable, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		NR_PROFILE_FUNC();

		Ref<MeshSource> meshSource = staticMesh->GetMeshSource();
		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex : staticMesh->GetSubmeshes())
		{
			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

			const auto& submeshes = staticMesh->GetMeshSource()->GetSubmeshes();
			uint32_t materialIndex = submeshes[submeshIndex].MaterialIndex;
			AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex)->Handle : staticMesh->GetMaterials()->GetMaterial(materialIndex)->Handle;

			MeshKey meshKey = { staticMesh->Handle, materialHandle, submeshIndex };
			auto& transformStorage = mMeshTransformMap[meshKey].Transforms.emplace_back();

			transformStorage.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformStorage.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformStorage.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			// Main geo
			{
				auto& dc = mStaticMeshDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
			}

			// Shadow pass
			{
				auto& dc = mStaticMeshShadowPassDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
			}
		}

	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		NR_PROFILE_FUNC();

		// TODO: Culling, sorting, etc.

		const auto& submeshes = mesh->GetMeshSource()->GetSubmeshes();
		uint32_t materialIndex = submeshes[submeshIndex].MaterialIndex;
		AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex)->Handle : mesh->GetMaterials()->GetMaterial(materialIndex)->Handle;

		MeshKey meshKey = { mesh->Handle, materialHandle, submeshIndex };
		auto& transformStorage = mMeshTransformMap[meshKey].Transforms.emplace_back();

		transformStorage.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		transformStorage.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		transformStorage.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

		uint32_t instanceIndex = 0;

		// Main geo
		{
			auto& dc = mDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;

			instanceIndex = dc.InstanceCount;
			dc.InstanceCount++;
		}

		// Selected mesh list
		{
			auto& dc = mSelectedMeshDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
			dc.InstanceOffset = instanceIndex;
		}

		// Shadow pass
		{
			auto& dc = mShadowPassDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::SubmitSelectedStaticMesh(Ref<StaticMesh> staticMesh, Ref<MaterialTable> materialTable, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		NR_PROFILE_FUNC();

		Ref<MeshSource> meshSource = staticMesh->GetMeshSource();
		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex : staticMesh->GetSubmeshes())
		{
			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

			const auto& submeshes = staticMesh->GetMeshSource()->GetSubmeshes();
			uint32_t materialIndex = submeshes[submeshIndex].MaterialIndex;
			AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex)->Handle : staticMesh->GetMaterials()->GetMaterial(materialIndex)->Handle;

			MeshKey meshKey = { staticMesh->Handle, materialHandle, submeshIndex };
			auto& transformStorage = mMeshTransformMap[meshKey].Transforms.emplace_back();

			transformStorage.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformStorage.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformStorage.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			uint32_t instanceIndex = 0;

			// Main geo
			{
				auto& dc = mStaticMeshDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;

				instanceIndex = dc.InstanceCount;
				dc.InstanceCount++;
			}

			// Selected mesh list
			{
				auto& dc = mSelectedStaticMeshDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
				dc.InstanceOffset = instanceIndex;
			}

			// Shadow pass
			{
				auto& dc = mStaticMeshShadowPassDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
			}
		}
	}

	void SceneRenderer::SubmitPhysicsDebugMesh(Ref<Mesh> mesh, uint32_t submeshIndex, const glm::mat4& transform)
	{
		MeshKey meshKey = { mesh->Handle, 5, submeshIndex };
		auto& transformStorage = mMeshTransformMap[meshKey].Transforms.emplace_back();

		transformStorage.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		transformStorage.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		transformStorage.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

		{
			auto& dc = mColliderDrawList[meshKey];
			dc.Mesh = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::SubmitPhysicsStaticDebugMesh(Ref<StaticMesh> staticMesh, const glm::mat4& transform)
	{
		Ref<MeshSource> meshSource = staticMesh->GetMeshSource();
		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex : staticMesh->GetSubmeshes())
		{
			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

			MeshKey meshKey = { staticMesh->Handle, 5, submeshIndex };
			auto& transformStorage = mMeshTransformMap[meshKey].Transforms.emplace_back();

			transformStorage.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformStorage.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformStorage.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			{
				auto& dc = mStaticColliderDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.SubmeshIndex = submeshIndex;
				dc.InstanceCount++;
			}

		}
	}

	void SceneRenderer::ClearPass(Ref<RenderPass> renderPass, bool explicitClear)
	{
		NR_PROFILE_FUNC();

		Renderer::BeginRenderPass(mCommandBuffer, renderPass, explicitClear);
		Renderer::EndRenderPass(mCommandBuffer);
	}

	void SceneRenderer::ShadowMapPass()
	{
		NR_PROFILE_FUNC();

		mGPUTimeQueries.ShadowMapPassQuery = mCommandBuffer->BeginTimestampQuery();

		auto& directionalLights = mSceneData.SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Multiplier == 0.0f || !directionalLights[0].CastShadows)
		{
			// Clear shadow maps
			for (int i = 0; i < 4; i++)
				ClearPass(mShadowPassPipelines[i]->GetSpecification().RenderPass);

			return;
		}

		// TODO: change to four cascades (or set number)
		for (int i = 0; i < 4; i++)
		{
			Renderer::BeginRenderPass(mCommandBuffer, mShadowPassPipelines[i]->GetSpecification().RenderPass);

			// static glm::mat4 scaleBiasMatrix = glm::scale(glm::mat4(1.0f), { 0.5f, 0.5f, 0.5f }) * glm::translate(glm::mat4(1.0f), { 1, 1, 1 });

			// Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : mStaticMeshShadowPassDrawList)
			{
				NR_CORE_VERIFY(mMeshTransformMap.find(mk) != mMeshTransformMap.end());
				const auto& transformData = mMeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(mCommandBuffer, mShadowPassPipelines[i], mUniformBufferSet, nullptr, dc.StaticMesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mShadowPassMaterial, cascade);
			}
			for (auto& [mk, dc] : mShadowPassDrawList)
			{
				NR_CORE_VERIFY(mMeshTransformMap.find(mk) != mMeshTransformMap.end());
				const auto& transformData = mMeshTransformMap.at(mk);
				if (dc.Mesh->IsRigged())
				{
					Renderer::RenderMeshWithMaterial(mCommandBuffer, mShadowPassPipelinesAnim[i], mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mShadowPassMaterial, cascade);
				}
				else
				{
					Renderer::RenderMeshWithMaterial(mCommandBuffer, mShadowPassPipelines[i], mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mShadowPassMaterial, cascade);
				}
			}

			Renderer::EndRenderPass(mCommandBuffer);
		}

		mCommandBuffer->EndTimestampQuery(mGPUTimeQueries.ShadowMapPassQuery);
	}

	void SceneRenderer::PreDepthPass()
	{
		// PreDepth Pass, only used for light culling for now
		mGPUTimeQueries.DepthPrePassQuery = mCommandBuffer->BeginTimestampQuery();
		Renderer::BeginRenderPass(mCommandBuffer, mPreDepthPipeline->GetSpecification().RenderPass);
		for (auto& [mk, dc] : mStaticMeshDrawList)
		{
			const auto& transformData = mMeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(mCommandBuffer, mPreDepthPipeline, mUniformBufferSet, nullptr, dc.StaticMesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mPreDepthMaterial);
		}
		for (auto& [mk, dc] : mDrawList)
		{
			const auto& transformData = mMeshTransformMap.at(mk);
			if (dc.Mesh->IsRigged())
			{
				Renderer::RenderMeshWithMaterial(mCommandBuffer, mPreDepthPipelineAnim, mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mPreDepthMaterial);
			}
			else
			{
				Renderer::RenderMeshWithMaterial(mCommandBuffer, mPreDepthPipeline, mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mPreDepthMaterial);
			}
		}
		for (auto& [mk, dc] : mSelectedStaticMeshDrawList)
		{
			const auto& transformData = mMeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(mCommandBuffer, mPreDepthPipeline, mUniformBufferSet, nullptr, dc.StaticMesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mPreDepthMaterial);
		}
		for (auto& [mk, dc] : mSelectedMeshDrawList)
		{
			const auto& transformData = mMeshTransformMap.at(mk);
			if (dc.Mesh->IsRigged())
			{
				Renderer::RenderMeshWithMaterial(mCommandBuffer, mPreDepthPipelineAnim, mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mPreDepthMaterial);
			}
			else
			{
				Renderer::RenderMeshWithMaterial(mCommandBuffer, mPreDepthPipeline, mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mPreDepthMaterial);
			}
		}
		Renderer::EndRenderPass(mCommandBuffer);

		Renderer::Submit([cb = mCommandBuffer, image = mPreDepthPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetDepthImage().As<VKImage2D>()]()
			{
				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				imageMemoryBarrier.image = image->GetImageInfo().Image;
				imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, image->GetSpecification().Mips, 0, 1 };
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(
					cb.As<VKRenderCommandBuffer>()->GetCommandBuffer(Renderer::GetCurrentFrameIndex()),
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &imageMemoryBarrier);
			});

		mCommandBuffer->EndTimestampQuery(mGPUTimeQueries.DepthPrePassQuery);
	}

	void SceneRenderer::LightCullingPass()
	{
		mLightCullingMaterial->Set("uPreDepthMap", mPreDepthPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetDepthImage());
		mLightCullingMaterial->Set("uScreenData.uScreenSize", glm::ivec2{ mViewportWidth, mViewportHeight });

		mGPUTimeQueries.LightCullingPassQuery = mCommandBuffer->BeginTimestampQuery();
		Renderer::LightCulling(mCommandBuffer, mLightCullingPipeline, mUniformBufferSet, mStorageBufferSet, mLightCullingMaterial, glm::ivec2{ mViewportWidth, mViewportHeight }, mLightCullingWorkGroups);
		mCommandBuffer->EndTimestampQuery(mGPUTimeQueries.LightCullingPassQuery);
	}

	void SceneRenderer::GeometryPass()
	{
		NR_PROFILE_FUNC();

		mGPUTimeQueries.GeometryPassQuery = mCommandBuffer->BeginTimestampQuery();

		Renderer::BeginRenderPass(mCommandBuffer, mSelectedGeometryPipeline->GetSpecification().RenderPass);
		for (auto& [mk, dc] : mSelectedStaticMeshDrawList)
		{
			const auto& transformData = mMeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(mCommandBuffer, mSelectedGeometryPipeline, mUniformBufferSet, nullptr, dc.StaticMesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), dc.InstanceCount, mSelectedGeometryMaterial);
		}
		for (auto& [mk, dc] : mSelectedMeshDrawList)
		{
			const auto& transformData = mMeshTransformMap.at(mk);
			if (dc.Mesh->IsRigged())
			{
				Renderer::RenderMeshWithMaterial(mCommandBuffer, mSelectedGeometryPipelineAnim, mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), dc.InstanceCount, mSelectedGeometryMaterial);
			}
			else
			{
				Renderer::RenderMeshWithMaterial(mCommandBuffer, mSelectedGeometryPipeline, mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), dc.InstanceCount, mSelectedGeometryMaterial);
			}
		}
		Renderer::EndRenderPass(mCommandBuffer);

		Renderer::BeginRenderPass(mCommandBuffer, mGeometryPipeline->GetSpecification().RenderPass);
		// Skybox
		mSkyboxMaterial->Set("uUniforms.TextureLod", mSceneData.SkyboxLod);
		mSkyboxMaterial->Set("uUniforms.Intensity", mSceneData.SceneEnvironmentIntensity);

		const Ref<TextureCube> radianceMap = mSceneData.SceneEnvironment ? mSceneData.SceneEnvironment->RadianceMap : Renderer::GetBlackCubeTexture();
		mSkyboxMaterial->Set("uTexture", radianceMap);
		Renderer::SubmitFullscreenQuad(mCommandBuffer, mSkyboxPipeline, mUniformBufferSet, nullptr, mSkyboxMaterial);

		// Render static meshes
		for (auto& [mk, dc] : mStaticMeshDrawList)
		{
			const auto& transformData = mMeshTransformMap.at(mk);
			Renderer::RenderStaticMesh(mCommandBuffer, mGeometryPipeline, mUniformBufferSet, mStorageBufferSet, dc.StaticMesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.StaticMesh->GetMaterials(), mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount);
		}

		// Render dynamic meshes
		for (auto& [mk, dc] : mDrawList)
		{
			const auto& transformData = mMeshTransformMap.at(mk);
			Renderer::RenderSubmeshInstanced(mCommandBuffer, (dc.Mesh->IsRigged() ? mGeometryPipelineAnim : mGeometryPipeline), mUniformBufferSet, mStorageBufferSet, dc.Mesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.Mesh->GetMaterials(), mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount);

		}

		// Grid
		if (GetOptions().ShowGrid)
		{
			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(8.0f));
			Renderer::RenderQuad(mCommandBuffer, mGridPipeline, mUniformBufferSet, nullptr, mGridMaterial, transform);
		}

		Renderer::EndRenderPass(mCommandBuffer);

		mCommandBuffer->EndTimestampQuery(mGPUTimeQueries.GeometryPassQuery);
	}

	void SceneRenderer::DeinterleavingPass()
	{
		if (!mOptions.EnableHBAO)
		{
			for (int i = 0; i < 2; i++)
				ClearPass(mDeinterleavingPipelines[i]->GetSpecification().RenderPass);

			return;
		}

		mDeinterleavingMaterial->Set("uLinearDepthTex", mPreDepthPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage());

		constexpr static glm::vec2 offsets[2]{ { 0.5f, 0.5f }, { 0.5f, 2.5f } };
		for (int i = 0; i < 2; i++)
		{
			mDeinterleavingMaterial->Set("uInfo.UVOffset", offsets[i]);
			Renderer::BeginRenderPass(mCommandBuffer, mDeinterleavingPipelines[i]->GetSpecification().RenderPass);
			Renderer::SubmitFullscreenQuad(mCommandBuffer, mDeinterleavingPipelines[i], mUniformBufferSet, nullptr, mDeinterleavingMaterial);
			Renderer::EndRenderPass(mCommandBuffer);
		}
	}

	void SceneRenderer::HBAOPass()
	{
		if (!mOptions.EnableHBAO)
		{
			Renderer::ClearImage(mCommandBuffer, mHBAOOutputImage);
			return;
		}
		mHBAOMaterial->Set("uLinearDepthTexArray", mDeinterleavingPipelines[0]->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage());
		mHBAOMaterial->Set("uViewNormalsTex", mGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage(1));
		mHBAOMaterial->Set("uViewPositionTex", mGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage(2));
		mHBAOMaterial->Set("oColor", mHBAOOutputImage);

		Renderer::DispatchComputeShader(mCommandBuffer, mHBAOPipeline, mUniformBufferSet, nullptr, mHBAOMaterial, mHBAOWorkGroups);
	}

	void SceneRenderer::ReinterleavingPass()
	{
		if (!mOptions.EnableHBAO)
		{
			ClearPass(mReinterleavingPipeline->GetSpecification().RenderPass);
			return;
		}
		Renderer::BeginRenderPass(mCommandBuffer, mReinterleavingPipeline->GetSpecification().RenderPass);
		mReinterleavingMaterial->Set("uTexResultsArray", mHBAOOutputImage);
		Renderer::SubmitFullscreenQuad(mCommandBuffer, mReinterleavingPipeline, nullptr, nullptr, mReinterleavingMaterial);
		Renderer::EndRenderPass(mCommandBuffer);
	}

	void SceneRenderer::HBAOBlurPass()
	{
		if (!mOptions.EnableHBAO)
		{
			ClearPass(mHBAOBlurPipelines[0]->GetSpecification().RenderPass);
			return;
		}
		{
			Renderer::BeginRenderPass(mCommandBuffer, mHBAOBlurPipelines[0]->GetSpecification().RenderPass);
			mHBAOBlurMaterials[0]->Set("uInfo.InvResDirection", glm::vec2{ mInvViewportWidth, 0.0f });
			mHBAOBlurMaterials[0]->Set("uInfo.Sharpness", mOptions.HBAOBlurSharpness);
			mHBAOBlurMaterials[0]->Set("uInputTex", mReinterleavingPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage());
			Renderer::SubmitFullscreenQuad(mCommandBuffer, mHBAOBlurPipelines[0], nullptr, nullptr, mHBAOBlurMaterials[0]);
			Renderer::EndRenderPass(mCommandBuffer);
		}

		{
			Renderer::BeginRenderPass(mCommandBuffer, mHBAOBlurPipelines[1]->GetSpecification().RenderPass);
			mHBAOBlurMaterials[1]->Set("uInfo.InvResDirection", glm::vec2{ 0.0f, mInvViewportHeight });
			mHBAOBlurMaterials[1]->Set("uInfo.Sharpness", mOptions.HBAOBlurSharpness);
			mHBAOBlurMaterials[1]->Set("uInputTex", mHBAOBlurPipelines[0]->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage());
			Renderer::SubmitFullscreenQuad(mCommandBuffer, mHBAOBlurPipelines[1], nullptr, nullptr, mHBAOBlurMaterials[1]);
			Renderer::EndRenderPass(mCommandBuffer);
		}
	}

	void SceneRenderer::JumpFloodPass()
	{
		NR_PROFILE_FUNC();

		mGPUTimeQueries.JumpFloodPassQuery = mCommandBuffer->BeginTimestampQuery();
		Renderer::BeginRenderPass(mCommandBuffer, mJumpFloodInitPipeline->GetSpecification().RenderPass);

		auto frameBuffer = mSelectedGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer;
		mJumpFloodInitMaterial->Set("uTexture", frameBuffer->GetImage());

		Renderer::SubmitFullscreenQuad(mCommandBuffer, mJumpFloodInitPipeline, nullptr, mJumpFloodInitMaterial);
		Renderer::EndRenderPass(mCommandBuffer);

		mJumpFloodPassMaterial[0]->Set("uTexture", mTempFrameBuffers[0]->GetImage());
		mJumpFloodPassMaterial[1]->Set("uTexture", mTempFrameBuffers[1]->GetImage());

		int steps = 2;
		int step = std::round(std::pow(steps - 1, 2));
		int index = 0;
		Buffer vertexOverrides;
		Ref<FrameBuffer> passFB = mJumpFloodPassPipeline[0]->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer;
		glm::vec2 texelSize = { 1.0f / (float)passFB->GetWidth(), 1.0f / (float)passFB->GetHeight() };
		vertexOverrides.Allocate(sizeof(glm::vec2) + sizeof(int));
		vertexOverrides.Write(glm::value_ptr(texelSize), sizeof(glm::vec2));
		while (step != 0)
		{
			vertexOverrides.Write(&step, sizeof(int), sizeof(glm::vec2));

			Renderer::BeginRenderPass(mCommandBuffer, mJumpFloodPassPipeline[index]->GetSpecification().RenderPass);
			Renderer::SubmitFullscreenQuadWithOverrides(mCommandBuffer, mJumpFloodPassPipeline[index], nullptr, mJumpFloodPassMaterial[index], vertexOverrides, Buffer());
			Renderer::EndRenderPass(mCommandBuffer);

			index = (index + 1) % 2;
			step /= 2;
		}

		mJumpFloodCompositeMaterial->Set("uTexture", mTempFrameBuffers[1]->GetImage());
		mCommandBuffer->EndTimestampQuery(mGPUTimeQueries.JumpFloodPassQuery);
	}

	void SceneRenderer::BloomCompute()
	{
		Ref<VKComputePipeline> pipeline = mBloomComputePipeline.As<VKComputePipeline>();

		//mBloomComputeMaterial->Set("oImage", mBloomComputeTexture);

		struct BloomComputePushConstants
		{
			glm::vec4 Params;
			float LOD = 0.0f;
			int Mode = 0; // 0 = prefilter, 1 = downsample, 2 = firstUpsample, 3 = upsample
		} bloomComputePushConstants;
		bloomComputePushConstants.Params = { mBloomSettings.Threshold, mBloomSettings.Threshold - mBloomSettings.Knee, mBloomSettings.Knee * 2.0f, 0.25f / mBloomSettings.Knee };
		bloomComputePushConstants.Mode = 0;

		auto inputTexture = mGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage().As<VKImage2D>();

		mGPUTimeQueries.BloomComputePassQuery = mCommandBuffer->BeginTimestampQuery();

		Renderer::Submit([bloomComputePushConstants, inputTexture, workGroupSize = mBloomComputeWorkgroupSize, commandBuffer = mCommandBuffer, bloomTextures = mBloomComputeTextures, pipeline]() mutable
			{
				constexpr bool useComputeQueue = false;

				VkDevice device = VKContext::GetCurrentDevice()->GetVulkanDevice();

				Ref<VKImage2D> images[3] =
				{
					bloomTextures[0]->GetImage().As<VKImage2D>(),
					bloomTextures[1]->GetImage().As<VKImage2D>(),
					bloomTextures[2]->GetImage().As<VKImage2D>()
				};

				auto shader = pipeline->GetShader().As<VKShader>();

				auto descriptorImageInfo = images[0]->GetDescriptor();
				descriptorImageInfo.imageView = images[0]->RT_GetMipImageView(0);

				std::array<VkWriteDescriptorSet, 3> writeDescriptors;

				VkDescriptorSetLayout descriptorSetLayout = shader->GetDescriptorSetLayout(0);

				VkDescriptorSetAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = &descriptorSetLayout;

				pipeline->Begin(useComputeQueue ? nullptr : commandBuffer);

				if (false)
				{
					VkImageMemoryBarrier imageMemoryBarrier = {};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageMemoryBarrier.image = inputTexture->GetImageInfo().Image;
					imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, inputTexture->GetSpecification().Mips, 0, 1 };
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					vkCmdPipelineBarrier(
						pipeline->GetActiveCommandBuffer(),
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						0,
						0, nullptr,
						0, nullptr,
						1, &imageMemoryBarrier);
				}

				// Output image
				VkDescriptorSet descriptorSet = VKRenderer::RT_AllocateDescriptorSet(allocInfo);
				writeDescriptors[0] = *shader->GetDescriptorSet("oImage");
				writeDescriptors[0].dstSet = descriptorSet;
				writeDescriptors[0].pImageInfo = &descriptorImageInfo;

				// Input image
				writeDescriptors[1] = *shader->GetDescriptorSet("uTexture");
				writeDescriptors[1].dstSet = descriptorSet;
				writeDescriptors[1].pImageInfo = &inputTexture->GetDescriptor();

				writeDescriptors[2] = *shader->GetDescriptorSet("uBloomTexture");
				writeDescriptors[2].dstSet = descriptorSet;
				writeDescriptors[2].pImageInfo = &inputTexture->GetDescriptor();

				vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

				uint32_t workGroupsX = bloomTextures[0]->GetWidth() / workGroupSize;
				uint32_t workGroupsY = bloomTextures[0]->GetHeight() / workGroupSize;

				pipeline->SetPushConstants(&bloomComputePushConstants, sizeof(bloomComputePushConstants));
				pipeline->Dispatch(descriptorSet, workGroupsX, workGroupsY, 1);

				{
					VkImageMemoryBarrier imageMemoryBarrier = {};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
					imageMemoryBarrier.image = images[0]->GetImageInfo().Image;
					imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, images[0]->GetSpecification().Mips, 0, 1 };
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					vkCmdPipelineBarrier(
						pipeline->GetActiveCommandBuffer(),
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						0,
						0, nullptr,
						0, nullptr,
						1, &imageMemoryBarrier);
				}

				bloomComputePushConstants.Mode = 1;

				VkSampler samplerClamp = VKRenderer::GetClampSampler();

				uint32_t mips = bloomTextures[0]->GetMipLevelCount() - 2;
				for (uint32_t i = 1; i < mips; i++)
				{
					auto [mipWidth, mipHeight] = bloomTextures[0]->GetMipSize(i);
					workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)workGroupSize);
					workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)workGroupSize);

					{
						// Output image
						descriptorImageInfo.imageView = images[1]->RT_GetMipImageView(i);

						descriptorSet = VKRenderer::RT_AllocateDescriptorSet(allocInfo);
						writeDescriptors[0] = *shader->GetDescriptorSet("oImage");
						writeDescriptors[0].dstSet = descriptorSet;
						writeDescriptors[0].pImageInfo = &descriptorImageInfo;

						// Input image
						writeDescriptors[1] = *shader->GetDescriptorSet("uTexture");
						writeDescriptors[1].dstSet = descriptorSet;
						auto descriptor = bloomTextures[0]->GetImage().As<VKImage2D>()->GetDescriptor();
						//descriptor.sampler = samplerClamp;
						writeDescriptors[1].pImageInfo = &descriptor;

						writeDescriptors[2] = *shader->GetDescriptorSet("uBloomTexture");
						writeDescriptors[2].dstSet = descriptorSet;
						writeDescriptors[2].pImageInfo = &inputTexture->GetDescriptor();

						vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

						bloomComputePushConstants.LOD = i - 1.0f;
						pipeline->SetPushConstants(&bloomComputePushConstants, sizeof(bloomComputePushConstants));
						pipeline->Dispatch(descriptorSet, workGroupsX, workGroupsY, 1);
					}

					{
						VkImageMemoryBarrier imageMemoryBarrier = {};
						imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
						imageMemoryBarrier.image = images[1]->GetImageInfo().Image;
						imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, images[1]->GetSpecification().Mips, 0, 1 };
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
						vkCmdPipelineBarrier(
							pipeline->GetActiveCommandBuffer(),
							VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
							VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
							0,
							0, nullptr,
							0, nullptr,
							1, &imageMemoryBarrier);
					}

					{
						descriptorImageInfo.imageView = images[0]->RT_GetMipImageView(i);

						// Output image
						descriptorSet = VKRenderer::RT_AllocateDescriptorSet(allocInfo);
						writeDescriptors[0] = *shader->GetDescriptorSet("oImage");
						writeDescriptors[0].dstSet = descriptorSet;
						writeDescriptors[0].pImageInfo = &descriptorImageInfo;

						// Input image
						writeDescriptors[1] = *shader->GetDescriptorSet("uTexture");
						writeDescriptors[1].dstSet = descriptorSet;
						auto descriptor = bloomTextures[1]->GetImage().As<VKImage2D>()->GetDescriptor();
						//descriptor.sampler = samplerClamp;
						writeDescriptors[1].pImageInfo = &descriptor;

						writeDescriptors[2] = *shader->GetDescriptorSet("uBloomTexture");
						writeDescriptors[2].dstSet = descriptorSet;
						writeDescriptors[2].pImageInfo = &inputTexture->GetDescriptor();

						vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

						bloomComputePushConstants.LOD = i;
						pipeline->SetPushConstants(&bloomComputePushConstants, sizeof(bloomComputePushConstants));
						pipeline->Dispatch(descriptorSet, workGroupsX, workGroupsY, 1);
					}

					{
						VkImageMemoryBarrier imageMemoryBarrier = {};
						imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
						imageMemoryBarrier.image = images[0]->GetImageInfo().Image;
						imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, images[0]->GetSpecification().Mips, 0, 1 };
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
						vkCmdPipelineBarrier(
							pipeline->GetActiveCommandBuffer(),
							VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
							VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
							0,
							0, nullptr,
							0, nullptr,
							1, &imageMemoryBarrier);
					}
				}

				bloomComputePushConstants.Mode = 2;
				workGroupsX *= 2;
				workGroupsY *= 2;

				// Output image
				descriptorSet = VKRenderer::RT_AllocateDescriptorSet(allocInfo);
				descriptorImageInfo.imageView = images[2]->RT_GetMipImageView(mips - 2);

				writeDescriptors[0] = *shader->GetDescriptorSet("oImage");
				writeDescriptors[0].dstSet = descriptorSet;
				writeDescriptors[0].pImageInfo = &descriptorImageInfo;

				// Input image
				writeDescriptors[1] = *shader->GetDescriptorSet("uTexture");
				writeDescriptors[1].dstSet = descriptorSet;
				writeDescriptors[1].pImageInfo = &bloomTextures[0]->GetImage().As<VKImage2D>()->GetDescriptor();

				writeDescriptors[2] = *shader->GetDescriptorSet("uBloomTexture");
				writeDescriptors[2].dstSet = descriptorSet;
				writeDescriptors[2].pImageInfo = &inputTexture->GetDescriptor();

				vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

				bloomComputePushConstants.LOD--;
				pipeline->SetPushConstants(&bloomComputePushConstants, sizeof(bloomComputePushConstants));

				auto [mipWidth, mipHeight] = bloomTextures[2]->GetMipSize(mips - 2);
				workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)workGroupSize);
				workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)workGroupSize);
				pipeline->Dispatch(descriptorSet, workGroupsX, workGroupsY, 1);

				{
					VkImageMemoryBarrier imageMemoryBarrier = {};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
					imageMemoryBarrier.image = images[2]->GetImageInfo().Image;
					imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, images[2]->GetSpecification().Mips, 0, 1 };
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					vkCmdPipelineBarrier(
						pipeline->GetActiveCommandBuffer(),
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						0,
						0, nullptr,
						0, nullptr,
						1, &imageMemoryBarrier);
				}

				bloomComputePushConstants.Mode = 3;

				// Upsample
				for (int32_t mip = mips - 3; mip >= 0; mip--)
				{
					auto [mipWidth, mipHeight] = bloomTextures[2]->GetMipSize(mip);
					workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)workGroupSize);
					workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)workGroupSize);

					// Output image
					descriptorImageInfo.imageView = images[2]->RT_GetMipImageView(mip);
					auto descriptorSet = VKRenderer::RT_AllocateDescriptorSet(allocInfo);
					writeDescriptors[0] = *shader->GetDescriptorSet("oImage");
					writeDescriptors[0].dstSet = descriptorSet;
					writeDescriptors[0].pImageInfo = &descriptorImageInfo;

					// Input image
					writeDescriptors[1] = *shader->GetDescriptorSet("uTexture");
					writeDescriptors[1].dstSet = descriptorSet;
					writeDescriptors[1].pImageInfo = &bloomTextures[0]->GetImage().As<VKImage2D>()->GetDescriptor();

					writeDescriptors[2] = *shader->GetDescriptorSet("uBloomTexture");
					writeDescriptors[2].dstSet = descriptorSet;
					writeDescriptors[2].pImageInfo = &images[2]->GetDescriptor();

					vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

					bloomComputePushConstants.LOD = mip;
					pipeline->SetPushConstants(&bloomComputePushConstants, sizeof(bloomComputePushConstants));
					pipeline->Dispatch(descriptorSet, workGroupsX, workGroupsY, 1);

					{
						VkImageMemoryBarrier imageMemoryBarrier = {};
						imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
						imageMemoryBarrier.image = images[2]->GetImageInfo().Image;
						imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, images[2]->GetSpecification().Mips, 0, 1 };
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
						vkCmdPipelineBarrier(
							pipeline->GetActiveCommandBuffer(),
							VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
							VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
							0,
							0, nullptr,
							0, nullptr,
							1, &imageMemoryBarrier);
					}
				}

				pipeline->End();
			});
		mCommandBuffer->EndTimestampQuery(mGPUTimeQueries.BloomComputePassQuery);
	}

	void SceneRenderer::CompositePass()
	{
		NR_PROFILE_FUNC();

		mGPUTimeQueries.CompositePassQuery = mCommandBuffer->BeginTimestampQuery();

		Renderer::BeginRenderPass(mCommandBuffer, mCompositePipeline->GetSpecification().RenderPass, true);

		auto frameBuffer = mGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer;
		float exposure = mSceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = frameBuffer->GetSpecification().Samples;

		CompositeMaterial->Set("uUniforms.Exposure", exposure);
		if (mBloomSettings.Enabled)
		{
			CompositeMaterial->Set("uUniforms.BloomIntensity", mBloomSettings.Intensity);
			CompositeMaterial->Set("uUniforms.BloomDirtIntensity", mBloomSettings.DirtIntensity);
		}
		else
		{
			CompositeMaterial->Set("uUniforms.BloomIntensity", 0.0f);
			CompositeMaterial->Set("uUniforms.BloomDirtIntensity", 0.0f);
		}

		// CompositeMaterial->Set("uUniforms.TextureSamples", textureSamples);

		CompositeMaterial->Set("uTexture", frameBuffer->GetImage());
		CompositeMaterial->Set("uBloomTexture", mBloomComputeTextures[2]);
		CompositeMaterial->Set("uBloomDirtTexture", mBloomDirtTexture);

		Renderer::SubmitFullscreenQuad(mCommandBuffer, mCompositePipeline, nullptr, CompositeMaterial);
		Renderer::SubmitFullscreenQuad(mCommandBuffer, mJumpFloodCompositePipeline, nullptr, mJumpFloodCompositeMaterial);
		Renderer::EndRenderPass(mCommandBuffer);

		mCommandBuffer->EndTimestampQuery(mGPUTimeQueries.CompositePassQuery);

#if 0 // WIP
		// DOF
		Renderer::BeginRenderPass(mCommandBuffer, mDOFPipeline->GetSpecification().RenderPass);
		mDOFMaterial->Set("uTexture", mCompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage());
		mDOFMaterial->Set("uDepthTexture", mPreDepthPipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetDepthImage());
		Renderer::SubmitFullscreenQuad(mCommandBuffer, mDOFPipeline, nullptr, mDOFMaterial);
		Renderer::EndRenderPass(mCommandBuffer);
#endif

		//Renderer::BeginRenderPass(mCommandBuffer, mJumpFloodCompositePipeline->GetSpecification().RenderPass);
		//Renderer::EndRenderPass(mCommandBuffer);


		if (mOptions.ShowSelectedInWireframe)
		{
			Renderer::BeginRenderPass(mCommandBuffer, mExternalCompositeRenderPass);

			for (auto& [mk, dc] : mSelectedStaticMeshDrawList)
			{
				const auto& transformData = mMeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(mCommandBuffer, mGeometryWireframePipeline, mUniformBufferSet, nullptr, dc.StaticMesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), dc.InstanceCount, mWireframeMaterial);
			}

			for (auto& [mk, dc] : mSelectedMeshDrawList)
			{
				const auto& transformData = mMeshTransformMap.at(mk);
				Renderer::RenderMeshWithMaterial(mCommandBuffer, mGeometryWireframePipeline, mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), dc.InstanceCount, mWireframeMaterial);
			}

			Renderer::EndRenderPass(mCommandBuffer);
		}

		if (mOptions.ShowPhysicsColliders != SceneRendererOptions::PhysicsColliderView::None)
		{
			Renderer::BeginRenderPass(mCommandBuffer, mExternalCompositeRenderPass);
			auto pipeline = mOptions.ShowPhysicsColliders == SceneRendererOptions::PhysicsColliderView::Normal ? mGeometryWireframePipeline : mGeometryWireframeOnTopPipeline;
			auto pipelineAnim = mOptions.ShowPhysicsColliders == SceneRendererOptions::PhysicsColliderView::Normal ? mGeometryWireframePipelineAnim : mGeometryWireframeOnTopPipelineAnim;
			mColliderMaterial->Set("uMaterialUniforms.Color", mOptions.PhysicsCollidersColor);

			for (auto& [mk, dc] : mStaticColliderDrawList)
			{
				NR_CORE_VERIFY(mMeshTransformMap.find(mk) != mMeshTransformMap.end());
				const auto& transformData = mMeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(mCommandBuffer, pipeline, mUniformBufferSet, nullptr, dc.StaticMesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mColliderMaterial);
			}

			for (auto& [mk, dc] : mColliderDrawList)
			{
				NR_CORE_VERIFY(mMeshTransformMap.find(mk) != mMeshTransformMap.end());
				const auto& transformData = mMeshTransformMap.at(mk);
				if (dc.Mesh->IsRigged())
				{
					Renderer::RenderMeshWithMaterial(mCommandBuffer, pipelineAnim, mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mColliderMaterial);
				}
				else
				{
					Renderer::RenderMeshWithMaterial(mCommandBuffer, pipeline, mUniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, mSubmeshTransformBuffer, transformData.TransformOffset, dc.InstanceCount, mColliderMaterial);
				}
			}

			Renderer::EndRenderPass(mCommandBuffer);
		}
	}

	void SceneRenderer::FlushDrawList()
	{
		if (mResourcesCreated && mViewportWidth > 0 && mViewportHeight > 0)
		{
			PreRender();

			mCommandBuffer->Begin();

			// Main render passes
			ShadowMapPass();
			PreDepthPass();
			LightCullingPass();
			GeometryPass();

			// HBAO
			mGPUTimeQueries.HBAOPassQuery = mCommandBuffer->BeginTimestampQuery();
			DeinterleavingPass();
			HBAOPass();
			ReinterleavingPass();
			HBAOBlurPass();
			mCommandBuffer->EndTimestampQuery(mGPUTimeQueries.HBAOPassQuery);

			// Post-processing
			JumpFloodPass();
			BloomCompute();
			CompositePass();

			mCommandBuffer->End();
			mCommandBuffer->Submit();
		}
		else
		{
			// Empty pass
			mCommandBuffer->Begin();

			ClearPass();

			mCommandBuffer->End();
			mCommandBuffer->Submit();
		}

		UpdateStatistics();

		mDrawList.clear();
		mSelectedMeshDrawList.clear();
		mShadowPassDrawList.clear();

		mStaticMeshDrawList.clear();
		mSelectedStaticMeshDrawList.clear();
		mStaticMeshShadowPassDrawList.clear();

		mColliderDrawList.clear();
		mStaticColliderDrawList.clear();
		mSceneData = {};

		mMeshTransformMap.clear();
	}

	void SceneRenderer::PreRender()
	{
		NR_PROFILE_FUNC();

#if 0
		uint32_t offset = 0;

		for (size_t i = 0; i < mStaticMeshShadowPassDrawList.size(); i++)
		{
			auto& dc = mStaticMeshShadowPassDrawList[i];

			// Copy transform data
			mTransformVertexData[offset].MRow[0] = { dc.Transform[0][0], dc.Transform[1][0], dc.Transform[2][0], dc.Transform[3][0] };
			mTransformVertexData[offset].MRow[1] = { dc.Transform[0][1], dc.Transform[1][1], dc.Transform[2][1], dc.Transform[3][1] };
			mTransformVertexData[offset].MRow[2] = { dc.Transform[0][2], dc.Transform[1][2], dc.Transform[2][2], dc.Transform[3][2] };

			dc.TransformOffset = offset * sizeof(TransformVertexData);
			offset++;
		}

		for (size_t i = 0; i < mShadowPassDrawList.size(); i++)
		{
			auto& dc = mShadowPassDrawList[i];

			// Copy transform data
			mTransformVertexData[offset].MRow[0] = { dc.Transform[0][0], dc.Transform[1][0], dc.Transform[2][0], dc.Transform[3][0] };
			mTransformVertexData[offset].MRow[1] = { dc.Transform[0][1], dc.Transform[1][1], dc.Transform[2][1], dc.Transform[3][1] };
			mTransformVertexData[offset].MRow[2] = { dc.Transform[0][2], dc.Transform[1][2], dc.Transform[2][2], dc.Transform[3][2] };

			dc.TransformOffset = offset * sizeof(TransformVertexData);
			offset++;
		}

		for (size_t i = 0; i < mStaticMeshDrawList.size(); i++)
		{
			auto& dc = mStaticMeshDrawList[i];

			// Copy transform data
			mTransformVertexData[offset].MRow[0] = { dc.Transform[0][0], dc.Transform[1][0], dc.Transform[2][0], dc.Transform[3][0] };
			mTransformVertexData[offset].MRow[1] = { dc.Transform[0][1], dc.Transform[1][1], dc.Transform[2][1], dc.Transform[3][1] };
			mTransformVertexData[offset].MRow[2] = { dc.Transform[0][2], dc.Transform[1][2], dc.Transform[2][2], dc.Transform[3][2] };

			dc.TransformOffset = offset * sizeof(TransformVertexData);
			offset++;
		}

		/*for (size_t i = 0; i < mDrawList.size(); i++)
		{
			auto& dc = mDrawList[i];

			// Copy transform data
			mTransformVertexData[offset].MRow[0] = { dc.Transform[0][0], dc.Transform[1][0], dc.Transform[2][0], dc.Transform[3][0] };
			mTransformVertexData[offset].MRow[1] = { dc.Transform[0][1], dc.Transform[1][1], dc.Transform[2][1], dc.Transform[3][1] };
			mTransformVertexData[offset].MRow[2] = { dc.Transform[0][2], dc.Transform[1][2], dc.Transform[2][2], dc.Transform[3][2] };

			dc.TransformOffset = offset * sizeof(TransformVertexData);
			offset++;
		}*/
#endif

		uint32_t offset = 0;
		for (auto& [key, transformData] : mMeshTransformMap)
		{
			transformData.TransformOffset = offset * sizeof(TransformVertexData);
			for (const auto& transform : transformData.Transforms)
			{
				mTransformVertexData[offset] = transform;
				offset++;
			}

		}

		mSubmeshTransformBuffer->SetData(mTransformVertexData, offset * sizeof(TransformVertexData));
	}

	void SceneRenderer::ClearPass()
	{
		NR_PROFILE_FUNC();

		Renderer::BeginRenderPass(mCommandBuffer, mCompositePipeline->GetSpecification().RenderPass, true);
		Renderer::EndRenderPass(mCommandBuffer);
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return mCompositePipeline->GetSpecification().RenderPass;
	}

	Ref<Image2D> SceneRenderer::GetFinalPassImage()
	{
		if (!mResourcesCreated)
			return nullptr;

		return mCompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetImage();
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return mOptions;
	}

	void SceneRenderer::CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection) const
	{
		auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;

		const int SHADOW_MAP_CASCADE_COUNT = 4;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

		float nearClip = sceneCamera.Near;
		float farClip = sceneCamera.Far;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
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
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
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
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

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
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + CascadeNearPlaneOffset, maxExtents.z - minExtents.z + CascadeFarPlaneOffset);

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			float ShadowMapResolution = mShadowPassPipelines[0]->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer->GetWidth();
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

	void SceneRenderer::UpdateStatistics()
	{
		mStatistics.DrawCalls = 0;
		mStatistics.Instances = 0;
		mStatistics.Meshes = 0;

		for (auto& [mk, dc] : mSelectedStaticMeshDrawList)
		{
			mStatistics.Instances += dc.InstanceCount;
			mStatistics.DrawCalls++;
			mStatistics.Meshes++;
		}

		for (auto& [mk, dc] : mStaticMeshDrawList)
		{
			mStatistics.Instances += dc.InstanceCount;
			mStatistics.DrawCalls++;
			mStatistics.Meshes++;
		}

		for (auto& [mk, dc] : mSelectedMeshDrawList)
		{
			mStatistics.Instances += dc.InstanceCount;
			mStatistics.DrawCalls++;
			mStatistics.Meshes++;
		}

		for (auto& [mk, dc] : mDrawList)
		{
			mStatistics.Instances += dc.InstanceCount;
			mStatistics.DrawCalls++;
			mStatistics.Meshes++;
		}

		mStatistics.SavedDraws = mStatistics.Instances - mStatistics.DrawCalls;
	}

	void SceneRenderer::SetLineWidth(float width)
	{
		mLineWidth = width;
		if (mGeometryWireframePipeline)
			mGeometryWireframePipeline->GetSpecification().LineWidth = width;
	}

	void SceneRenderer::ImGuiRender()
	{
		NR_PROFILE_FUNC();

		ImGui::Begin("Scene Renderer");

		ImGui::Text("Viewport Size: %d, %d", mViewportWidth, mViewportHeight);

		const float headerSpacingOffset = -(ImGui::GetStyle().ItemSpacing.y + 1.0f);

		if (UI::PropertyGridHeader("Shaders", false))
		{
			ImGui::Indent();
			auto& shaders = Shader::sAllShaders;
			for (auto& shader : shaders)
			{
				ImGui::Columns(2);
				ImGui::Text(shader->GetName().c_str());
				ImGui::NextColumn();
				std::string buttonName = "Reload##" + shader->GetName();
				if (ImGui::Button(buttonName.c_str()))
					shader->Reload(true);
				ImGui::Columns(1);
			}
			ImGui::Unindent();
			UI::EndTreeNode();
			UI::ShiftCursorY(18.0f);
		}
		else
			UI::ShiftCursorY(headerSpacingOffset);

		if (UI::PropertyGridHeader("Visualization"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Show Light Complexity", RendererDataUB.ShowLightComplexity);
			UI::Property("Show Shadow Cascades", RendererDataUB.ShowCascades);
			static int maxDrawCall = 1000;
			UI::PropertySlider("Selected Draw", VKRenderer::GetSelectedDrawCall(), -1, maxDrawCall);
			UI::Property("Max Draw Call", maxDrawCall);
			UI::EndPropertyGrid();
			UI::EndTreeNode();
		}
		else
			UI::ShiftCursorY(headerSpacingOffset);


		if (UI::PropertyGridHeader("Bloom Settings"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Bloom Enabled", mBloomSettings.Enabled);
			UI::Property("Threshold", mBloomSettings.Threshold);
			UI::Property("Knee", mBloomSettings.Knee);
			UI::Property("Upsample Scale", mBloomSettings.UpsampleScale);
			UI::Property("Intensity", mBloomSettings.Intensity, 0.05f, 0.0f, 20.0f);
			UI::Property("Dirt Intensity", mBloomSettings.DirtIntensity, 0.05f, 0.0f, 20.0f);

			// TODO(Yan): move this to somewhere else
			UI::Image(mBloomDirtTexture, ImVec2(64, 64));
			if (ImGui::IsItemHovered())
			{
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get().OpenFile("");
					if (!filename.empty())
						mBloomDirtTexture = Texture2D::Create(filename);
				}
			}

			UI::EndPropertyGrid();
			UI::EndTreeNode();
		}
		else
			UI::ShiftCursorY(headerSpacingOffset);


		if (UI::PropertyGridHeader("Horizon-Based Ambient Occlusion"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Enable", mOptions.EnableHBAO);
			UI::Property("Intensity", mOptions.HBAOIntensity, 0.05f, 0.1f, 4.0f);
			UI::Property("Radius", mOptions.HBAORadius, 0.05f, 0.0f, 4.0f);
			UI::Property("Bias", mOptions.HBAOBias, 0.02f, 0.0f, 0.95f);
			UI::Property("Blur Sharpness", mOptions.HBAOBlurSharpness, 0.5f, 0.0f, 100.f);
			UI::EndPropertyGrid();

			UI::EndTreeNode();
		}
		else
			UI::ShiftCursorY(headerSpacingOffset);

		if (UI::PropertyGridHeader("Shadows"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Soft Shadows", RendererDataUB.SoftShadows);
			UI::Property("DirLight Size", RendererDataUB.LightSize, 0.01f);
			UI::Property("Max Shadow Distance", RendererDataUB.MaxShadowDistance, 1.0f);
			UI::Property("Shadow Fade", RendererDataUB.ShadowFade, 5.0f);
			UI::EndPropertyGrid();
			if (UI::BeginTreeNode("Cascade Settings"))
			{
				UI::BeginPropertyGrid();
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
				auto fb = mShadowPassPipelines[cascadeIndex]->GetSpecification().RenderPass->GetSpecification().TargetFrameBuffer;
				auto image = fb->GetDepthImage();

				float size = ImGui::GetContentRegionAvailWidth(); // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
				UI::BeginPropertyGrid();
				UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
				UI::EndPropertyGrid();
				if (mResourcesCreated)
					UI::Image(image, (uint32_t)cascadeIndex, { size, size }, { 0, 1 }, { 1, 0 });
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}
		else
			UI::ShiftCursorY(headerSpacingOffset);

		if (UI::PropertyGridHeader("Render Statistics", false))
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			ImGui::Text("GPU time: %.3fms", mCommandBuffer->GetExecutionGPUTime(frameIndex));

			ImGui::Text("Shadow Map Pass: %.3fms", mCommandBuffer->GetExecutionGPUTime(frameIndex, mGPUTimeQueries.ShadowMapPassQuery));
			ImGui::Text("Depth Pre-Pass: %.3fms", mCommandBuffer->GetExecutionGPUTime(frameIndex, mGPUTimeQueries.DepthPrePassQuery));
			ImGui::Text("Light Culling Pass: %.3fms", mCommandBuffer->GetExecutionGPUTime(frameIndex, mGPUTimeQueries.LightCullingPassQuery));
			ImGui::Text("Geometry Pass: %.3fms", mCommandBuffer->GetExecutionGPUTime(frameIndex, mGPUTimeQueries.GeometryPassQuery));
			ImGui::Text("HBAO Pass: %.3fms", mCommandBuffer->GetExecutionGPUTime(frameIndex, mGPUTimeQueries.HBAOPassQuery));
			ImGui::Text("Bloom Pass: %.3fms", mCommandBuffer->GetExecutionGPUTime(frameIndex, mGPUTimeQueries.BloomComputePassQuery));
			ImGui::Text("Jump Flood Pass: %.3fms", mCommandBuffer->GetExecutionGPUTime(frameIndex, mGPUTimeQueries.JumpFloodPassQuery));
			ImGui::Text("Composite Pass: %.3fms", mCommandBuffer->GetExecutionGPUTime(frameIndex, mGPUTimeQueries.CompositePassQuery));

			if (UI::BeginTreeNode("Pipeline Statistics"))
			{
				const PipelineStatistics& pipelineStats = mCommandBuffer->GetPipelineStatistics(frameIndex);
				ImGui::Text("Input Assembly Vertices: %llu", pipelineStats.InputAssemblyVertices);
				ImGui::Text("Input Assembly Primitives: %llu", pipelineStats.InputAssemblyPrimitives);
				ImGui::Text("Vertex Shader Invocations: %llu", pipelineStats.VertexShaderInvocations);
				ImGui::Text("Clipping Invocations: %llu", pipelineStats.ClippingInvocations);
				ImGui::Text("Clipping Primitives: %llu", pipelineStats.ClippingPrimitives);
				ImGui::Text("Fragment Shader Invocations: %llu", pipelineStats.FragmentShaderInvocations);
				ImGui::Text("Compute Shader Invocations: %llu", pipelineStats.ComputeShaderInvocations);
				UI::EndTreeNode();
			}

			if (UI::BeginTreeNode("Draw Statistics"))
			{
				ImGui::Text("Draw calls: %d", mStatistics.DrawCalls);
				ImGui::Text("Meshes: %d", mStatistics.Meshes);
				ImGui::Text("Instances: %d", mStatistics.Instances);
				ImGui::Text("Actual instances: %d", mStatistics.Instances - mStatistics.DrawCalls);
				ImGui::Text("Draws saved (by instancing): %d", mStatistics.SavedDraws);
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}
		else
			UI::ShiftCursorY(headerSpacingOffset);

#if 0
		if (UI::BeginTreeNode("Compute Bloom"))
		{
			float size = ImGui::GetContentRegionAvailWidth();
			if (mResourcesCreated)
			{
				static int tex = 0;
				UI::PropertySlider("Texture", tex, 0, 2);
				static int mip = 0;
				auto [mipWidth, mipHeight] = mBloomComputeTextures[tex]->GetMipSize(mip);
				std::string label = fmt::format("Mip ({0}x{1})", mipWidth, mipHeight);
				UI::PropertySlider(label.c_str(), mip, 0, mBloomComputeTextures[tex]->GetMipLevelCount() - 1);
				UI::ImageMip(mBloomComputeTextures[tex]->GetImage(), mip, { size, size * (1.0f / mBloomComputeTextures[tex]->GetImage()->GetAspectRatio()) }, { 0, 1 }, { 1, 0 });
			}
			UI::EndTreeNode();
		}
#endif

		ImGui::End();
	}

}