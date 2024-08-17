#pragma once

namespace NR
{
	using RendererID = uint32_t;

	enum class RendererAPIType
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

	class RendererAPI
	{
	public:
		static void Init();
		static void Shutdown();

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count, bool depthTestActive = true);

		static RenderAPICapabilities& GetCapabilities()
		{
			static RenderAPICapabilities capabilities;
			return capabilities;
		}

		static RendererAPIType Current() { return sCurrentRendererAPI; }

	private:
		static void LoadRequiredAssets();

	private:
		static RendererAPIType sCurrentRendererAPI;
	};
}