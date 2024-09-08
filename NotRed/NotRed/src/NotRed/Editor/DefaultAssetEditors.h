#pragma once

#include "AssetEditorPanel.h"
#include "NotRed/Renderer/Mesh.h"

namespace NR
{
	class PhysicsMaterialEditor : public AssetEditor
	{
	public:
		PhysicsMaterialEditor();

		void SetAsset(const Ref<Asset>& asset) override { mAsset = (Ref<PhysicsMaterial>)asset; }

	private:
		void Render() override;

	private:
		Ref<PhysicsMaterial> mAsset;
	};

	class TextureEditor : public AssetEditor
	{
	public:
		TextureEditor();

		void SetAsset(const Ref<Asset>& asset) override { mAsset = (Ref<Texture>)asset; }

	private:
		void Render() override;

	private:
		Ref<Texture> mAsset;
	};

}