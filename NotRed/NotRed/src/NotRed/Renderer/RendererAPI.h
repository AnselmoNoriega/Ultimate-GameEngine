#pragma once

namespace NR
{
	using RendererID = uint32_t;

	enum class PrimitiveType
	{
		None, 
		Lines,
		Triangles
	};

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

		int MaxSamples = 0;
		float MaxAnisotropy = 0.0f;
		int MaxTextureUnits = 0;
	};

	class RendererAPI
	{
	public:
		static void Init();
		static void Shutdown();

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest = true, bool faceCulling = true);
		static void SetLineThickness(float thickness);

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