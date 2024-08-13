#pragma once

#include "RenderCommandQueue.h"
#include "RendererAPI.h"

namespace NR
{
	class NOT_RED_API Renderer
	{
	private:

	public:
		// Commands
		static void Clear();
		static void Clear(float r, float g, float b, float a = 1.0f);
		static void SetClearColor(float r, float g, float b, float a);

		static void ClearMagenta();

		void Init();

		static void Submit(const std::function<void()>& command)
		{
		}

		void WaitAndRender();

		inline static Renderer& Get() { return *sInstance; }
	private:
		static Renderer* sInstance;

		RenderCommandQueue mCommandQueue;
	};

#define NR_RENDER(x) ::NR::Renderer::Submit([=](){ RendererAPI::x; })
}