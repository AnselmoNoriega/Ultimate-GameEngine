#pragma once

#include "NotRed/Renderer/Texture.h"

#include <filesystem>

namespace NR
{
	struct MSDFData;

	class Font : public RefCounted
	{
	public:
		Font(const std::filesystem::path& filepath);
		virtual ~Font();

		Ref<Texture2D> GetFontAtlas() const { return mTextureAtlas; }
		const MSDFData* GetMSDFData() const { return mMSDFData; }

	private:
		std::filesystem::path mFilePath;
		Ref<Texture2D> mTextureAtlas;
		MSDFData* mMSDFData = nullptr;
	};
}