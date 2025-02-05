#pragma once

#include <filesystem>

#include "NotRed/Core/Input.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Asset/AssetMetadata.h"

namespace NR
{
#define MAX_INPUT_BUFFER_LENGTH 128

	enum class ContentBrowserAction
	{
		None = 0,
		Refresh = 1 << 0,
		ClearSelections = 1 << 1,
		Selected = 1 << 2,
		Hovered = 1 << 3,
		Renamed = 1 << 4,
		NavigateToThis = 1 << 5,
		OpenDeleteDialogue = 1 << 6,
		SelectToHere = 1 << 7,
		Moved = 1 << 8,
		ShowInExplorer = 1 << 9,
		OpenExternal = 1 << 10,
		Reload = 1 << 11,
		Copy = 1 << 12,
		Duplicate = 1 << 13,
		StartRenaming = 1 << 14
	};

	struct CBItemActionResult
	{
		uint16_t Field = 0;

		void Modify(ContentBrowserAction flag, bool add)
		{
			if (add)
			{
				Field |= (uint16_t)flag;
			}
			else
			{
				Field &= ~(uint16_t)flag;
			}
		}

		bool IsSet(ContentBrowserAction flag) const { return (uint16_t)flag & Field; }
	};

	class ContentBrowserItem : public RefCounted
	{
	public:
		enum class ItemType : uint16_t
		{
			Directory, Asset
		};

	public:
		ContentBrowserItem(ItemType type, AssetHandle id, const std::string& name, const Ref<Texture2D>& icon);
		virtual ~ContentBrowserItem() = default;

		void RenderBegin();
		CBItemActionResult Render(float thumbnailSize, bool displayAssetType);
		void RenderEnd();

		virtual void Delete() {}
		virtual bool Move(const std::filesystem::path& destination) { return false; }

		bool IsSelected() const { return mIsSelected; }

		AssetHandle GetID() const { return mID; }
		ItemType GetType() const { return mType; }
		const std::string& GetName() const { return mName; }

		const Ref<Texture2D>& GetIcon() const { return mIcon; }

		virtual void Activate(CBItemActionResult& actionResult) {}
		void StartRenaming();
		void StopRenaming();
		bool IsRenaming() const { return mIsRenaming; }

		void SetSelected(bool value);

		void Rename(const std::string& newName, bool fromCallback = false);

	private:
		virtual void Renamed(const std::string& newName, bool fromCallback = false) { mName = newName; }
		virtual void RenderCustomContextItems() {}
		virtual void UpdateDrop(CBItemActionResult& actionResult) {}

		void ContextMenuOpen(CBItemActionResult& actionResult);

	protected:
		ItemType mType;
		AssetHandle mID;
		std::string mName;
		Ref<Texture2D> mIcon;

		bool mIsSelected = false;
		bool mIsRenaming = false;
		bool mIsDragging = false;

	private:
		friend class ContentBrowserPanel;
	};

	struct DirectoryInfo : public RefCounted
	{
		AssetHandle Handle;
		Ref<DirectoryInfo> Parent;

		std::filesystem::path FilePath;

		std::vector<AssetHandle> Assets;
		std::unordered_map<AssetHandle, Ref<DirectoryInfo>> SubDirectories;
	};

	class ContentBrowserDirectory : public ContentBrowserItem
	{
	public:
		ContentBrowserDirectory(const Ref<DirectoryInfo>& directoryInfo, const Ref<Texture2D>& icon);
		virtual ~ContentBrowserDirectory() = default;

		Ref<DirectoryInfo>& GetDirectoryInfo() { return mDirectoryInfo; }

		void Delete() override;
		bool Move(const std::filesystem::path& destination) override;

	private:
		void Activate(CBItemActionResult& actionResult) override;
		void Renamed(const std::string& newName, bool fromCallback) override;
		void UpdateDrop(CBItemActionResult& actionResult) override;

		void UpdateDirectoryPath(Ref<DirectoryInfo> directoryInfo, const std::filesystem::path& newParentPath, const std::filesystem::path& newName);

	private:
		Ref<DirectoryInfo> mDirectoryInfo;
	};

	class ContentBrowserAsset : public ContentBrowserItem
	{
	public:
		ContentBrowserAsset(const AssetMetadata& assetInfo, const Ref<Texture2D>& icon);
		~ContentBrowserAsset() override = default;

		const AssetMetadata& GetAssetInfo() const { return mAssetInfo; }

		void Delete() override;
		bool Move(const std::filesystem::path& destination) override;

	private:
		void Activate(CBItemActionResult& actionResult) override;
		void Renamed(const std::string& newName, bool fromCallback) override;

	private:
		AssetMetadata mAssetInfo;
	};

	namespace Utils
	{
		static std::string ContentBrowserItemTypeToString(ContentBrowserItem::ItemType type)
		{
			switch (type)
			{
			case ContentBrowserItem::ItemType::Asset: return "Asset";
			case ContentBrowserItem::ItemType::Directory: return "Directory";
			}

			return "Unknown";
		}
	}

}