#include "nrpch.h"
#include "AssetEditorPanel.h"

#include "DefaultAssetEditors.h"
#include "NotRed/Asset/AssetManager.h"

#include "MeshViewerPanel.h"

namespace NR
{
	AssetEditor::AssetEditor(const char* title)
		: mTitle(title), mMinSize(200, 400), mMaxSize(2000, 2000)
	{
	}

	void AssetEditor::ImGuiRender()
	{
		if (!mIsOpen)
		{
			return;
		}

		bool wasOpen = mIsOpen;
		ImGui::SetNextWindowSizeConstraints(mMinSize, mMaxSize);
		ImGui::Begin(mTitle.c_str(), &mIsOpen, mFlags);
		Render();
		ImGui::End();
		if (wasOpen && !mIsOpen)
		{
			Close();
		}
	}

	void AssetEditor::SetOpen(bool isOpen)
	{
		mIsOpen = isOpen;
		if (!mIsOpen)
		{
			Close();
		}
	}

	void AssetEditor::SetMinSize(uint32_t width, uint32_t height)
	{
		if (width <= 0) width = 200;
		if (height <= 0) height = 400;

		mMinSize = ImVec2((float)width, (float)height);
	}

	void AssetEditor::SetMaxSize(uint32_t width, uint32_t height)
	{
		if (width <= 0) width = 2000;
		if (height <= 0) height = 2000;
		if (float(width) <= mMinSize.x) width = uint32_t(mMinSize.x * 2.f);
		if (float(height) <= mMinSize.y) height = uint32_t(mMinSize.y * 2.f);

		mMaxSize = ImVec2((float)width, (float)height);
	}

	void AssetEditor::SetTitle(const std::string& newTitle)
	{
		mTitle = newTitle;
	}

	const std::string& AssetEditor::GetTitle() const
	{
		return mTitle;
	}

	void AssetEditorPanel::RegisterDefaultEditors()
	{
		RegisterEditor<TextureViewer>(AssetType::Texture);
		RegisterEditor<PhysicsMaterialEditor>(AssetType::PhysicsMat);
		RegisterEditor<AudioFileViewer>(AssetType::Audio);
		RegisterEditor<MeshViewerPanel>(AssetType::MeshAsset);
		RegisterEditor<SoundConfigEditor>(AssetType::SoundConfig);
	}

	void AssetEditorPanel::UnregisterAllEditors()
	{
		sEditors.clear();
	}

	void AssetEditorPanel::Update(float dt)
	{
		for (auto& kv : sEditors)
		{
			kv.second->Update(dt);
		}
	}
	void AssetEditorPanel::OnEvent(Event& e)
	{
		for (auto& kv : sEditors)
		{
			kv.second->OnEvent(e);
		}
	}

	void AssetEditorPanel::ImGuiRender()
	{
		for (auto& kv : sEditors)
		{
			kv.second->ImGuiRender();
		}
	}

	void AssetEditorPanel::OpenEditor(const Ref<Asset>& asset)
	{
		if (sEditors.find(asset->GetAssetType()) == sEditors.end())
		{
			return;
		}

		sEditors[asset->GetAssetType()]->SetOpen(true);
		sEditors[asset->GetAssetType()]->SetAsset(AssetManager::GetAsset<Asset>(asset->Handle));
	}

	std::unordered_map<AssetType, Scope<AssetEditor>> AssetEditorPanel::sEditors;
}