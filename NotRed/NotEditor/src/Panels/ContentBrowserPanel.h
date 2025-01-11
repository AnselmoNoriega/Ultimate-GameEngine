#pragma once

#include <map>

#include "NotRed/Project/Project.h"
#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/ImGui/ImGui.h"

#include "ContentBrowser/ContentBrowserItem.h"
#include "NotRed/Core/Events/KeyEvent.h"

#define MAX_INPUT_BUFFER_LENGTH 128

namespace NR
{
    class SelectionStack
    {
    public:
        void Select(AssetHandle handle)
        {
            if (IsSelected(handle))
            {
                return;
            }

            mSelections.push_back(handle);
        }

        void Deselect(AssetHandle handle)
        {
            if (!IsSelected(handle))
            {
                return;
            }

            for (auto it = mSelections.begin(); it != mSelections.end(); it++)
            {
                if (handle == *it)
                {
                    mSelections.erase(it);
                    break;
                }
            }
        }

        bool IsSelected(AssetHandle handle) const
        {
            for (const auto& selectedHandle : mSelections)
            {
                if (selectedHandle == handle)
                {
                    return true;
                }
            }

            return false;
        }

        void CopyFrom(const SelectionStack& other)
        {
            mSelections.assign(other.begin(), other.end());
        }

        void Clear()
        {
            mSelections.clear();
        }

        size_t SelectionCount() const { return mSelections.size(); }
        const AssetHandle* SelectionData() const { return mSelections.data(); }

        AssetHandle operator[](size_t index) const
        {
            NR_CORE_ASSERT(index >= 0 && index < mSelections.size());
            return mSelections[index];
        }

        std::vector<AssetHandle>::iterator begin() { return mSelections.begin(); }
        std::vector<AssetHandle>::const_iterator begin() const { return mSelections.begin(); }
        std::vector<AssetHandle>::iterator end() { return mSelections.end(); }
        std::vector<AssetHandle>::const_iterator end() const { return mSelections.end(); }

    private:
        std::vector<AssetHandle> mSelections;
    };


    struct ContentBrowserItemList
    {
        static constexpr size_t InvalidItem = std::numeric_limits<size_t>::max();

        std::vector<Ref<ContentBrowserItem>> Items;

        std::vector<Ref<ContentBrowserItem>>::iterator begin() { return Items.begin(); }
        std::vector<Ref<ContentBrowserItem>>::iterator end() { return Items.end(); }
        std::vector<Ref<ContentBrowserItem>>::const_iterator begin() const { return Items.begin(); }
        std::vector<Ref<ContentBrowserItem>>::const_iterator end() const { return Items.end(); }

        Ref<ContentBrowserItem>& operator[](size_t index) { return Items[index]; }
        const Ref<ContentBrowserItem>& operator[](size_t index) const { return Items[index]; }

        void erase(AssetHandle handle)
        {
            size_t index = FindItem(handle);
            if (index == InvalidItem)
            {
                return;
            }

            auto it = Items.begin() + index;
            Items.erase(it);
        }

        size_t FindItem(AssetHandle handle) const
        {
            for (size_t i = 0; i < Items.size(); i++)
            {
                if (Items[i]->GetID() == handle)
                {
                    return i;
                }
            }
            return InvalidItem;
        }
    };

    class ContentBrowserPanel
    {
    public:
        static ContentBrowserPanel& Get() { return *sInstance; }

    public:
        ContentBrowserPanel(Ref<Project> project);

        void ImGuiRender();
        void OnEvent(Event& e);

        const SelectionStack& GetSelectionStack() const { return mSelectionStack; }
        ContentBrowserItemList& GetCurrentItems() { return mCurrentItems; }

        Ref<DirectoryInfo> GetDirectory(const std::filesystem::path& filepath) const;

    private:
        AssetHandle ProcessDirectory(const std::filesystem::path& directoryPath, const Ref<DirectoryInfo>& parent);

        void ChangeDirectory(Ref<DirectoryInfo>& directory);
        void RenderDirectoryHierarchy(Ref<DirectoryInfo>& directory);
        void RenderTopBar();
        void RenderItems();
        void RenderBottomBar();

        void Refresh();

        void UpdateInput();

        bool OnKeyPressedEvent(KeyPressedEvent& e);
        void PasteCopiedAssets();

        void ClearSelections();

        void RenderDeleteDialogue();
        void RemoveDirectory(Ref<DirectoryInfo>& directory, bool removeFromParent = true);

        void UpdateDropArea(const Ref<DirectoryInfo>& target);
        void SortItemList();

        ContentBrowserItemList Search(const std::string& query, const Ref<DirectoryInfo>& directoryInfo);

        void FileSystemChanged(FileSystemChangedEvent event);
        
        void DirectoryAdded(FileSystemChangedEvent event);
        void DirectoryDeleted(FileSystemChangedEvent event);
        void DirectoryDeleted(Ref<DirectoryInfo> directory, uint32_t depth = 0);
        void DirectoryRenamed(FileSystemChangedEvent event);
        
        void AssetAdded(FileSystemChangedEvent event);
        void AssetDeleted(FileSystemChangedEvent event);
        void AssetDeleted(AssetMetadata metadata, Ref<DirectoryInfo> directory);
        void AssetRenamed(FileSystemChangedEvent event);

        void UpdateDirectoryPath(Ref<DirectoryInfo>& directoryInfo, const std::filesystem::path& newParentPath, const std::string& newName);

    private:
        template<typename T, typename... Args>
        Ref<T> CreateAsset(const std::string& filename, Args&&... args)
        {
            FileSystem::SkipNextFileSystemChange();
            Ref<T> asset = AssetManager::CreateNewAsset<T>(filename, mCurrentDirectory->FilePath.string(), std::forward<Args>(args)...);
            if (!asset)
            {
                return nullptr;
            }

            mCurrentDirectory->Assets.push_back(asset->Handle);
            ChangeDirectory(mCurrentDirectory);
            auto& item = mCurrentItems[mCurrentItems.FindItem(asset->Handle)];
            mSelectionStack.Select(asset->Handle);
            item->SetSelected(true);
            item->StartRenaming();
            return asset;
        }

    private:
        Ref<Project> mProject;

        Ref<Texture2D> mFileTex;
        Ref<Texture2D> mFolderIcon;
        Ref<Texture2D> mBackbtnTex;
        Ref<Texture2D> mFwrdbtnTex;
        Ref<Texture2D> mRefreshIcon;
        Ref<Texture2D> mShadow;

        std::map<std::string, Ref<Texture2D>> mAssetIconMap;

        ContentBrowserItemList mCurrentItems;

        Ref<DirectoryInfo> mCurrentDirectory;
        Ref<DirectoryInfo> mBaseDirectory;
        Ref<DirectoryInfo> mNextDirectory, mPreviousDirectory;

        bool mIsAnyItemHovered = false;

        SelectionStack mSelectionStack, mCopiedAssets;

        std::unordered_map<AssetHandle, Ref<DirectoryInfo>> mDirectories;

        char mSearchBuffer[MAX_INPUT_BUFFER_LENGTH];

        std::vector<Ref<DirectoryInfo>> mBreadCrumbData;
        bool mUpdateNavigationPath = false;

        bool mIsContentBrowserHovered = false;
        bool mIsContentBrowserFocused = false;

    private:
        static ContentBrowserPanel* sInstance;
    };

}