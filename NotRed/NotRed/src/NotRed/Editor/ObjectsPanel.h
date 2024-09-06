#pragma once

#include "NotRed/Renderer/Texture.h"

namespace NR
{
	class ObjectsPanel
	{
	public:
		ObjectsPanel();

		void ImGuiRender();

	private:
		Ref<Texture2D> mCubeImage;
	};
}