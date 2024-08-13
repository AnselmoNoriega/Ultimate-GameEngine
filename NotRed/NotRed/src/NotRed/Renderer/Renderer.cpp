#include "nrpch.h"
#include "Renderer.h"
#include "RenderCommand.h"

namespace NR
{
	Renderer* Renderer::sInstance = new Renderer();

	void Renderer::Init()
	{

	}

	void Renderer::Clear()
	{
		// HZ_RENDER(Clear());
	}

	void Renderer::Clear(float r, float g, float b, float a)
	{
		float params[4] = { r, g, b, a };
		sInstance->mCommandQueue.SubmitCommand(RenderCommand::Clear, params, sizeof(float) * 4);
	}

	void Renderer::ClearMagenta()
	{
		Clear(1, 0, 1);
	}

	void Renderer::SetClearColor(float r, float g, float b, float a)
	{

	}

	void Renderer::WaitAndRender()
	{
		sInstance->mCommandQueue.Execute();
	}
}