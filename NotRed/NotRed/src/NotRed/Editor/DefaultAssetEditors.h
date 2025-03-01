#pragma once

#include "AssetEditorPanel.h"
#include "NotRed/Renderer/Mesh.h"

#include "NotRed/Scene/Prefab.h"
#include "NotRed/Editor/SceneHierarchyPanel.h"

namespace NR
{
	struct SoundConfig;

	class PhysicsMaterialEditor : public AssetEditor
	{
	public:
		PhysicsMaterialEditor();

		void SetAsset(const Ref<Asset>& asset) override { mAsset = (Ref<PhysicsMaterial>)asset; }

	private:		
		void Close() override;
		void Render() override;

	private:
		Ref<PhysicsMaterial> mAsset;
	};

	class MaterialEditor : public AssetEditor
	{
	public:
		MaterialEditor();
		void SetAsset(const Ref<Asset>& asset) override { mMaterialAsset = (Ref<MaterialAsset>)asset; }

	private:
		void Close() override;
		void Render() override;

	private:
		Ref<MaterialAsset> mMaterialAsset;
		Ref<Texture2D> mCheckerboardTex;
	};

	class PrefabEditor : public AssetEditor
	{
	public:
		PrefabEditor();
		void SetAsset(const Ref<Asset>& asset) override { mPrefab = (Ref<Prefab>)asset; mSceneHierarchyPanel.SetSceneContext(mPrefab->mScene); }

	private:
		void Close() override;
		void Render() override;

	private:
		Ref<Prefab> mPrefab;
		SceneHierarchyPanel mSceneHierarchyPanel;
	};

	class TextureViewer : public AssetEditor
	{
	public:
		TextureViewer();

		void SetAsset(const Ref<Asset>& asset) override { mAsset = (Ref<Texture>)asset; }

	private:
		void Render() override;

	private:		
		void Close() override;
		Ref<Texture> mAsset;
	};

	class AudioFileViewer : public AssetEditor
	{
	public:
		AudioFileViewer();
		void SetAsset(const Ref<Asset>& asset) override;

	private:
		void Close() override;
		void Render() override;

	private:
		Ref<AudioFile> mAsset;
	};

	class SoundConfigEditor : public AssetEditor
	{
	public:
		SoundConfigEditor();
		void SetAsset(const Ref<Asset>& asset) override;

	private:
		void Close() override;
		void Render() override;

	private:
		Ref<SoundConfig> mAsset;
	};
}