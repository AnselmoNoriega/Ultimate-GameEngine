#include "nrpch.h"
#include "AssetEditorPanel.h"

#include "DefaultAssetEditors.h"
#include "NotRed/Asset/AssetManager.h"

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
		ImGui::Begin(mTitle, &mIsOpen, mFlags);
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

		mMinSize = ImVec2(width, height);
	}

	void AssetEditor::SetMaxSize(uint32_t width, uint32_t height)
	{
		if (width <= 0) width = 2000;
		if (height <= 0) height = 2000;
		if (width <= mMinSize.x) width = mMinSize.x * 2;
		if (height <= mMinSize.y) height = mMinSize.y * 2;

		mMaxSize = ImVec2(width, height);
	}

	void AssetEditorPanel::RegisterDefaultEditors()
	{
		RegisterEditor<TextureViewer>(AssetType::Texture);
		RegisterEditor<PhysicsMaterialEditor>(AssetType::PhysicsMat);
	}

	void AssetEditorPanel::UnregisterAllEditors()
	{
		sEditors.clear();
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
		if (sEditors.find(asset->Type) == sEditors.end())
		{
			NR_CORE_WARN("No editor registered for {0} assets", asset->Extension);
			return;
		}

		sEditors[asset->Type]->SetOpen(true);
		sEditors[asset->Type]->SetAsset(AssetManager::GetAsset<Asset>(asset->Handle));
	}

	std::unordered_map<AssetType, Scope<AssetEditor>> AssetEditorPanel::sEditors;

}