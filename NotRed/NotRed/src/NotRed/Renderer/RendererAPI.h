#pragma once

namespace NR
{
	class RendererAPI
	{
	private:

	public:
		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);
	};
}