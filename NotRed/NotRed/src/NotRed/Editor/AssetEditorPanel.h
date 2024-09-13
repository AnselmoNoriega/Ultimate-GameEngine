#pragma once

#include "NotRed/ImGui/ImGui.h"

namespace NR
{
	class AssetEditor
	{
	protected:
		AssetEditor(const char* title);

	public:
		virtual ~AssetEditor() = default;

		void ImGuiRender();		
		void SetOpen(bool isOpen);
		virtual void SetAsset(const Ref<Asset>& asset) = 0;

	private:		
		virtual void Close() = 0;
		virtual void Render() = 0;

	protected:
		void SetMinSize(uint32_t width, uint32_t height);
		void SetMaxSize(uint32_t width, uint32_t height);

	private:
		const char* mTitle;
		bool mIsOpen = false;

		ImGuiWindowFlags mFlags = 0;
		ImVec2 mMinSize, mMaxSize;
	};

	class AssetEditorPanel
	{
	public:
		static void RegisterDefaultEditors();		
		static void UnregisterAllEditors();
		static void ImGuiRender();
		static void OpenEditor(const Ref<Asset>& asset);

		template<typename T>
		static void RegisterEditor(AssetType type)
		{
			static_assert(std::is_base_of<AssetEditor, T>::value, "AssetEditorPanel::RegisterEditor requires template type to inherit from AssetEditor");
			NR_CORE_ASSERT(sEditors.find(type) == sEditors.end(), "There's already an editor for that asset!");
			sEditors[type] = CreateScope<T>();
		}

	private:
		static std::unordered_map<AssetType, Scope<AssetEditor>> sEditors;
	};

}