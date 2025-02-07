#pragma once

#include "NotRed/Renderer/Texture.h"

#include <filesystem>

namespace NR
{
	struct MSDFData;

	class Font : public Asset
	{
	public:
		Font(const std::filesystem::path& filepath);
		virtual ~Font();

		Ref<Texture2D> GetFontAtlas() const { return mTextureAtlas; }
		const MSDFData* GetMSDFData() const { return mMSDFData; }
		
		static void StaticInit();
		
		static Ref<Font> GetDefaultFont();
		
		static AssetType GetStaticType() { return AssetType::Font; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		std::filesystem::path mFilePath;
		Ref<Texture2D> mTextureAtlas;
		MSDFData* mMSDFData = nullptr;
	};
}