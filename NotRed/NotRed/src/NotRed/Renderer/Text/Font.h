#pragma once

#include <filesystem>

#include "NotRed/Core/Core.h"
#include "NotRed/Renderer/Texture.h"

namespace NR
{
	struct MSDFData;

	class Font
	{
	public:
		Font(const std::filesystem::path& filepath);
		~Font();

		Ref<Texture2D> GetTextureAtlas() const { return mAtlas; }
		const MSDFData* GetMSDFData() const { return mData; }

		static Ref<Font> GetDefault();

	private:
		MSDFData* mData;
		Ref<Texture2D> mAtlas;
	};
}