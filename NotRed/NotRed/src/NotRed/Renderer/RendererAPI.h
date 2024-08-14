#pragma once

namespace NR
{
	using RendererID = uint32_t;

	enum class NOT_RED_API RendererAPIType
	{
		None,
		OpenGL
	};

	class NOT_RED_API RendererAPI
	{
	public:
		static void Init();
		static void Shutdown();

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count);

		static RendererAPIType Current() { return sCurrentRendererAPI; }

	private:
		static RendererAPIType sCurrentRendererAPI;
	};
}