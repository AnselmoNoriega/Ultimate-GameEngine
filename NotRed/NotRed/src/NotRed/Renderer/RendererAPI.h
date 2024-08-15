#pragma once

namespace NR
{
	using RendererID = uint32_t;

	enum class NOT_RED_API RendererAPIType
	{
		None,
		OpenGL
	}; 
	
	struct RenderAPICapabilities
	{
		std::string Vendor;
		std::string Renderer;
		std::string Version;

		int MaxSamples;
		float MaxAnisotropy;
	};

	class NOT_RED_API RendererAPI
	{
	public:
		static void Init();
		static void Shutdown();

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(unsigned int count, bool depthTest = true);

		static RenderAPICapabilities& GetCapabilities()
		{
			static RenderAPICapabilities capabilities;
			return capabilities;
		}

		static RendererAPIType Current() { return sCurrentRendererAPI; }

	private:
		static RendererAPIType sCurrentRendererAPI;
	};
}