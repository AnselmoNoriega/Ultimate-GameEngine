#include "nrpch.h"
#include "Renderer.h"

#include "Shader.h"

namespace NR
{
	Renderer* Renderer::sInstance = new Renderer();
	RendererAPIType RendererAPI::sCurrentRendererAPI = RendererAPIType::OpenGL;

	void Renderer::Init()
	{
		sInstance->mShaderLibrary = std::make_unique<ShaderLibrary>();

		NR_RENDER({ RendererAPI::Init(); });

		Renderer::GetShaderLibrary()->Load("Assets/Shaders/PBR_Static");
		Renderer::GetShaderLibrary()->Load("Assets/Shaders/PBR_Anim");
	}

	void Renderer::Clear()
	{
		NR_RENDER({
			RendererAPI::Clear(0.0f, 0.0f, 0.0f, 1.0f);
			});
	}

	void Renderer::Clear(float r, float g, float b, float a)
	{
		NR_RENDER_4(r, g, b, a, {
			RendererAPI::Clear(r, g, b, a);
			});
	}

	void Renderer::ClearMagenta()
	{
		Clear(1, 0, 1);
	}

	void Renderer::SetClearColor(float r, float g, float b, float a)
	{

	}

	void Renderer::DrawIndexed(uint32_t count, bool depthTestActive)
	{
		NR_RENDER_2(count, depthTestActive, {
			RendererAPI::DrawIndexed(count, depthTestActive);
			});
	}

	void Renderer::WaitAndRender()
	{
		sInstance->mCommandQueue.Execute();
	}

	void Renderer::IBeginRenderPass(const Ref<RenderPass>& renderPass)
	{
		mActiveRenderPass = renderPass;

		renderPass->GetSpecification().TargetFrameBuffer->Bind();
	}

	void Renderer::IEndRenderPass()
	{
		NR_CORE_ASSERT(mActiveRenderPass, "No active render pass! Have you called Renderer::EndRenderPass twice?");
		mActiveRenderPass->GetSpecification().TargetFrameBuffer->Unbind();
		mActiveRenderPass = nullptr;
	}

	void Renderer::ISubmitMesh(const Ref<Mesh>& mesh)
	{

	}
}