#include "nrpch.h"
#include "Renderer.h"

namespace NR
{
	Renderer* Renderer::sInstance = new Renderer();
	RendererAPIType RendererAPI::sCurrentRendererAPI = RendererAPIType::OpenGL;

	void Renderer::Init()
	{
		NR_RENDER({ RendererAPI::Init(); });
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
}