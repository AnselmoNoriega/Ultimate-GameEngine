#pragma once

#include <unordered_map>
#include <utility>

#include "NotRed/Core/Core.h"
#include "NotRed/Core/Log.h"
#include "NotRed/Core/Hash.h"

#include "EditorPanel.h"

namespace NR 
{
	struct PanelData
	{
		const char* ID = "";
		const char* Name = "";
		Ref<EditorPanel> Panel = nullptr;
		bool IsOpen = false;
	};

	class PanelManager
	{
	public:
		PanelManager() = default;
		~PanelManager()
		{
			mPanels.clear();
		}

		template<typename TPanel>
		Ref<TPanel> AddPanel(const PanelData& panelData)
		{
			static_assert(std::is_base_of<EditorPanel, TPanel>::value, "PanelManager::AddPanel requires TPanel to inherit from EditorPanel");

			uint32_t id = Hash::GenerateFNVHash(panelData.ID);
			if (mPanels.find(id) != mPanels.end())
			{
				NR_CORE_ERROR("[PanelManager]: A panel with the id '{0}' has already been added.", panelData.ID);
				return nullptr;
			}

			mPanels[id] = panelData;
			return panelData.Panel.As<TPanel>();
		}

		template<typename TPanel, typename... TArgs>
		Ref<TPanel> AddPanel(const char* strID, bool isOpenByDefault, TArgs&&... args)
		{
			return AddPanel<TPanel>(PanelData{ strID, strID, Ref<TPanel>::Create(std::forward<TArgs>(args)...), isOpenByDefault });
		}

		template<typename TPanel, typename... TArgs>
		Ref<TPanel> AddPanel(const char* strID, const char* displayName, bool isOpenByDefault, TArgs&&... args)
		{
			return AddPanel<TPanel>(PanelData{ strID, displayName, Ref<TPanel>::Create(std::forward<TArgs>(args)...), isOpenByDefault });
		}

		template<typename TPanel>
		Ref<TPanel> GetPanel(const char* strID)
		{
			static_assert(std::is_base_of<EditorPanel, TPanel>::value, "PanelManager::AddPanel requires TPanel to inherit from EditorPanel");

			uint32_t id = Hash::GenerateFNVHash(strID);
			if (mPanels.find(id) == mPanels.end())
			{
				NR_CORE_ERROR("[PanelManager]: Couldn't find panel with id '{0}'", strID);
				return nullptr;
			}

			return mPanels[id].Panel.As<TPanel>();
		}

		void RemovePanel(const char* strID)
		{
			uint32_t id = Hash::GenerateFNVHash(strID);
			if (mPanels.find(id) == mPanels.end())
			{
				NR_CORE_ERROR("[PanelManager]: Couldn't find panel with id '{0}'", strID);
				return;
			}

			mPanels.erase(id);
		}

		void ImGuiRender()
		{
			for (auto& [id, panelData] : mPanels)
			{
				if (panelData.IsOpen)
				{
					panelData.Panel->ImGuiRender(panelData.IsOpen);
				}
			}
		}

		void OnEvent(Event& e)
		{
			for (auto& [id, panelData] : mPanels)
				panelData.Panel->OnEvent(e);
		}

		void SetSceneContext(const Ref<Scene>& context)
		{
			for (auto& [id, panelData] : mPanels)
				panelData.Panel->SetSceneContext(context);
		}

		void OnProjectChanged(const Ref<Project>& project)
		{
			for (auto& [id, panelData] : mPanels)
			{
				panelData.Panel->ProjectChanged(project);
			}
		}

		std::unordered_map<uint32_t, PanelData>& GetPanels() { return mPanels; }
		const std::unordered_map<uint32_t, PanelData>& GetPanels() const { return mPanels; }

	private:
		std::unordered_map<uint32_t, PanelData> mPanels;
	};
}