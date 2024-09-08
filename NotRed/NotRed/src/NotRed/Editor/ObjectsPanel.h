#pragma once

#include "NotRed/Renderer/Texture.h"
#include "NotRed/Asset/AssetManager.h"

namespace NR
{
	class ObjectsPanel
	{
	public:
		ObjectsPanel();

		void ImGuiRender();

	private:
		void DrawObject(const char* label, AssetHandle handle);

	private:
		Ref<Texture2D> mCubeImage;
	};
}