#include "nrpch.h"
#include "Renderer.h"

#include "Shader.h"

#include <glad/glad.h>
#include <map>

#include "RendererAPI.h"
#include "SceneRenderer.h"
#include "Renderer2D.h"

#include "NotRed/Core/Timer.h"
#include "NotRed/Debug/Profiler.h"

#include "NotRed/Platform/Vulkan/VKComputePipeline.h"
#include "NotRed/Platform/Vulkan/VKRenderer.h"

#include "NotRed/Platform/Vulkan/VKContext.h"

#include "NotRed/Project/Project.h"

#include <filesystem>

namespace NR
{
	static std::unordered_map<size_t, Ref<Pipeline>> sPipelineCache;

	static RendererAPI* sRendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<Ref<PipelineCompute>> ComputePipelines;
		std::vector<Ref<Pipeline>> Pipelines;
		std::vector<Ref<Material>> Materials;
	};
	static std::unordered_map<size_t, ShaderDependencies> sShaderDependencies;

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<PipelineCompute> computePipeline)
	{
		sShaderDependencies[shader->GetHash()].ComputePipelines.push_back(computePipeline);
	}

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline)
	{
		sShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
	}

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material)
	{
		sShaderDependencies[shader->GetHash()].Materials.push_back(material);
	}

	void Renderer::ShaderReloaded(size_t hash)
	{
		if (sShaderDependencies.find(hash) != sShaderDependencies.end())
		{
			auto& dependencies = sShaderDependencies.at(hash);
			for (auto& pipeline : dependencies.Pipelines)
			{
				pipeline->Invalidate();
			}

			for (auto& computePipeline : dependencies.ComputePipelines)
			{
				computePipeline.As<VKComputePipeline>()->CreatePipeline();
			}

			for (auto& material : dependencies.Materials)
			{
				material->Invalidate();
			}
		}
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return Application::Get().GetWindow().GetSwapChain().GetCurrentBufferIndex();
	}

	void RendererAPI::SetAPI(RendererAPIType api)
	{
		NR_CORE_VERIFY(api == RendererAPIType::Vulkan, "Vulkan is currently the only supported Renderer API");
		sCurrentRendererAPI = api;
	}

	struct RendererData
	{
		Ref<ShaderLibrary> mShaderLibrary;

		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<Texture2D> BRDFLutTexture;
		Ref<TextureCube> BlackCubeTexture;
		Ref<Environment> EmptyEnvironment;
	};

	static RendererData* sData = nullptr;
	static RenderCommandQueue* sCommandQueue = nullptr;
	static RenderCommandQueue sResourceFreeQueue[3];

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::Vulkan: return new VKRenderer();
		}
		NR_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void Renderer::Init()
	{
		sData = new RendererData();
		sCommandQueue = new RenderCommandQueue();

		// Make sure we don't have more frames in flight than swapchain images
		Renderer::GetConfig().FramesInFlight = glm::min<uint32_t>(Renderer::GetConfig().FramesInFlight, Application::Get().GetWindow().GetSwapChain().GetImageCount());

		sRendererAPI = InitRendererAPI();

		sData->mShaderLibrary = Ref<ShaderLibrary>::Create();

		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PBR_Static");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PBR_Anim");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Grid");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Wireframe");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Wireframe_Anim");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Skybox");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/ShadowMap");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/ShadowMap_Anim");

		// HBAO
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Deinterleaving");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Reinterleaving");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/HBAO");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/HBAOBlur");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/HBAOBlur2");

		// Environment compute shaders
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentMipFilter");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EquirectangularToCubeMap");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentIrradiance");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PreethamSky");

		// Post-processing
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/Bloom");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PostProcessing/DOF");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SceneComposite");

		// Light-culling
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PreDepth");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PreDepth_Anim");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/LightCulling");

		// Renderer2D Shaders
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Line");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Circle");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Text");

		// Jump Flood Shaders
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Init");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Pass");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Composite");

		// Misc
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SelectedGeometry");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SelectedGeometry_Anim");

		// Compile shaders
		Renderer::WaitAndRender();

		uint32_t whiteTextureData = 0xffffffff;
		sData->WhiteTexture = Texture2D::Create(ImageFormat::RGBA, 1, 1, &whiteTextureData);

		uint32_t blackTextureData = 0xff000000;
		sData->BlackTexture = Texture2D::Create(ImageFormat::RGBA, 1, 1, &blackTextureData);
		{
			TextureProperties props;
			props.SamplerWrap = TextureWrap::Clamp;
			sData->BRDFLutTexture = Texture2D::Create("Resources/Renderer/BRDF_LUT.tga", props);
		}

		uint32_t blackCubeTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		sData->BlackCubeTexture = TextureCube::Create(ImageFormat::RGBA, 1, 1, &blackCubeTextureData);

		sData->EmptyEnvironment = Ref<Environment>::Create(sData->BlackCubeTexture, sData->BlackCubeTexture);

		sRendererAPI->Init();
	}

	void Renderer::Shutdown()
	{
		sShaderDependencies.clear();
		sRendererAPI->Shutdown();

		delete sData;
		delete sCommandQueue;
	}

	RendererCapabilities& Renderer::GetCapabilities()
	{
		return sRendererAPI->GetCapabilities();
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return sData->mShaderLibrary;
	}

	void Renderer::WaitAndRender()
	{
		NR_PROFILE_FUNC();
		NR_SCOPE_PERF("Renderer::WaitAndRender");
		sCommandQueue->Execute();
	}

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		NR_CORE_ASSERT(renderPass, "Render pass cannot be null!");

		sRendererAPI->BeginRenderPass(renderCommandBuffer, renderPass, explicitClear);
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		sRendererAPI->EndRenderPass(renderCommandBuffer);
	}

	void Renderer::BeginFrame()
	{
		sRendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		sRendererAPI->EndFrame();
	}

	void Renderer::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow, Ref<Image2D> linearDepth)
	{
		sRendererAPI->SetSceneEnvironment(sceneRenderer, environment, shadow, linearDepth);
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return sRendererAPI->CreateEnvironmentMap(filepath);
	}

	void Renderer::LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> computePipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups)
	{
		sRendererAPI->LightCulling(renderCommandBuffer, computePipeline, uniformBufferSet, storageBufferSet, material, screenSize, workGroups);
	}

	void Renderer::DispatchComputeShader(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> computePipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups)
	{
		sRendererAPI->DispatchComputeShader(renderCommandBuffer, computePipeline, uniformBufferSet, storageBufferSet, material, workGroups);
	}

	Ref<TextureCube> Renderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
	{
		return sRendererAPI->CreatePreethamSky(turbidity, azimuth, inclination);
	}

	void Renderer::RenderStaticMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<StaticMesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount)
	{
		sRendererAPI->RenderStaticMesh(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, materialTable, transformBuffer, transformOffset, instanceCount);
	}

	void Renderer::RenderSubmeshInstanced(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<MaterialTable> materialTable, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, const std::vector<Ref<StorageBuffer>>& boneTransformUBs, uint32_t boneTransformsOffset, uint32_t instanceCount)
	{
		sRendererAPI->RenderSubmeshInstanced(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, materialTable, transformBuffer, transformOffset, boneTransformUBs, boneTransformsOffset, instanceCount);
	}

	void Renderer::RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, const std::vector<Ref<StorageBuffer>>& boneTransformUBs, uint32_t boneTransformsOffset, uint32_t instanceCount, Ref<Material> material, Buffer additionalUniforms)
	{
		sRendererAPI->RenderMeshWithMaterial(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, material, transformBuffer, transformOffset, boneTransformUBs, boneTransformsOffset, instanceCount, additionalUniforms);
	}

	void Renderer::RenderStaticMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<StaticMesh> mesh, uint32_t submeshIndex, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Ref<Material> material, Buffer additionalUniforms)
	{
		sRendererAPI->RenderStaticMeshWithMaterial(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, submeshIndex, material, transformBuffer, transformOffset, instanceCount, additionalUniforms);
	}

	void Renderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::mat4& transform)
	{
		sRendererAPI->RenderQuad(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, material, transform);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount /*= 0*/)
	{
		sRendererAPI->RenderGeometry(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, material, vertexBuffer, indexBuffer, transform, indexCount);
	}

	void Renderer::SubmitQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, const glm::mat4& transform)
	{
		NR_CORE_ASSERT(false, "Not Implemented");
		/*bool depthTest = true;
		if (material)
		{
			material->Bind();
			depthTest = material->GetFlag(MaterialFlag::DepthTest);
			cullFace = !material->GetFlag(MaterialFlag::TwoSided);

			auto shader = material->GetShader();
			shader->SetUniformBuffer("Transform", &transform, sizeof(glm::mat4));
		}

		sData->mFullscreenQuadVertexBuffer->Bind();
		sData->mFullscreenQuadPipeline->Bind();
		sData->mFullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);*/
	}

	void Renderer::ClearImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Image2D> image)
	{
		sRendererAPI->ClearImage(renderCommandBuffer, image);
	}

	void Renderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material)
	{
		sRendererAPI->SubmitFullscreenQuad(renderCommandBuffer, pipeline, uniformBufferSet, material);
	}

	void Renderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material)
	{
		sRendererAPI->SubmitFullscreenQuad(renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, material);
	}

	void Renderer::SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides)
	{
		sRendererAPI->SubmitFullscreenQuadWithOverrides(renderCommandBuffer, pipeline, uniformBufferSet, material, vertexShaderOverrides, fragmentShaderOverrides);
	}

#if 0
	void Renderer::SubmitFullscreenQuad(Ref<Material> material)
	{
		// Retrieve pipeline from cache
		auto& shader = material->GetShader();
		auto hash = shader->GetHash();
		if (sPipelineCache.find(hash) == sPipelineCache.end())
		{
			// Create pipeline
			PipelineSpecification spec = sData->mFullscreenQuadPipelineSpec;
			spec.Shader = shader;
			spec.DebugName = "Renderer-FullscreenQuad-" + shader->GetName();
			sPipelineCache[hash] = Pipeline::Create(spec);
		}

		auto& pipeline = sPipelineCache[hash];

		bool depthTest = true;
		bool cullFace = true;
		if (material)
		{
			// material->Bind();
			depthTest = material->GetFlag(MaterialFlag::DepthTest);
			cullFace = !material->GetFlag(MaterialFlag::TwoSided);
		}

		sData->FullscreenQuadVertexBuffer->Bind();
		pipeline->Bind();
		sData->FullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}
#endif

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return sData->WhiteTexture;
	}

	Ref<Texture2D> Renderer::GetBlackTexture()
	{
		return sData->BlackTexture;
	}

	Ref<Texture2D> Renderer::GetBRDFLutTexture()
	{
		return sData->BRDFLutTexture;
	}

	Ref<TextureCube> Renderer::GetBlackCubeTexture()
	{
		return sData->BlackCubeTexture;
	}


	Ref<Environment> Renderer::GetEmptyEnvironment()
	{
		return sData->EmptyEnvironment;
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *sCommandQueue;
	}

	RenderCommandQueue& Renderer::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return sResourceFreeQueue[index];
	}

	RendererConfig& Renderer::GetConfig()
	{
		static RendererConfig config;
		return config;
	}

}