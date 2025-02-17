#include "nrpch.h"
#include "ImGui.h"

namespace NR::UI
{
	Ref<Texture2D> Widgets::sSearchIcon;
	Ref<Texture2D> Widgets::sClearIcon;
	Ref<Texture2D> Widgets::sGearIcon;

	void Widgets::Init()
	{
		sSearchIcon = Texture2D::Create("Resources/Editor/icon_search_24px.png", { TextureWrap::Clamp });
		sClearIcon = Texture2D::Create("Resources/Editor/close.png", { TextureWrap::Clamp });
		sGearIcon = Texture2D::Create("Resources/Editor/gear_icon.png", { TextureWrap::Clamp });
	}

	void Widgets::Shutdown() 
	{
		sSearchIcon.Reset();
		sClearIcon.Reset();
		sGearIcon.Reset();
	}

	bool Widgets::AssetSearchPopup(const char* ID, AssetType assetType, AssetHandle& selected, bool* cleared, const char* hint /*= "Search Assets"*/, const ImVec2& size)
	{
		UI::ScopedColor popupBG(ImGuiCol_PopupBg, UI::ColorWithMultipliedValue(Colors::Theme::background, 1.6f));

		bool modified = false;

		std::string preview;
		float itemHeight = size.y / 20.0f;

		const auto& assetRegistry = AssetManager::GetAssetRegistry();
		AssetHandle current = selected;

		if (UI::IsItemDisabled())
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		ImGui::SetNextWindowSize({ size.x, 0.0f });

		static bool grabFocus = true;

		if (UI::BeginPopup(ID, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			static std::string searchString;

			if (ImGui::GetCurrentWindow()->Appearing)
			{
				grabFocus = true;
				searchString.clear();
			}

			// Search widget
			UI::ShiftCursor(3.0f, 2.0f);
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() * 2.0f);
			SearchWidget(searchString, hint, &grabFocus);

			const bool searching = !searchString.empty();

			// Clear property button
			if (cleared != nullptr)
			{
				UI::ScopedColorStack buttonColors(
					ImGuiCol_Button, UI::ColorWithMultipliedValue(Colors::Theme::background, 1.0f),
					ImGuiCol_ButtonHovered, UI::ColorWithMultipliedValue(Colors::Theme::background, 1.2f),
					ImGuiCol_ButtonActive, UI::ColorWithMultipliedValue(Colors::Theme::background, 0.9f));

				UI::ScopedStyle border(ImGuiStyleVar_FrameBorderSize, 0.0f);

				ImGui::SetCursorPosX(0);

				ImGui::PushItemFlag(ImGuiItemFlags_NoNav, searching);

				if (ImGui::Button("CLEAR", { ImGui::GetWindowWidth(), 0.0f }))
				{
					*cleared = true;
					modified = true;
				}

				ImGui::PopItemFlag();
			}

			// List of assets
			{
				UI::ScopedColor listBoxBg(ImGuiCol_FrameBg, IM_COL32_DISABLE);
				UI::ScopedColor listBoxBorder(ImGuiCol_Border, IM_COL32_DISABLE);

				ImGuiID listID = ImGui::GetID("##SearchListBox");
				if (ImGui::BeginListBox("##SearchListBox", ImVec2(-FLT_MIN, 0.0f)))
				{
					bool forwardFocus = false;

					ImGuiContext& g = *GImGui;
					if (g.NavJustMovedToId != 0)
					{
						if (g.NavJustMovedToId == listID)
						{
							forwardFocus = true;
							// ActivateItem moves keyboard navigation focuse inside of the window
							ImGui::ActivateItem(listID);
							ImGui::SetKeyboardFocusHere(1);
						}
					}

					for (auto it = assetRegistry.cbegin(); it != assetRegistry.cend(); it++)
					{
						const auto& [path, metadata] = *it;

						if (metadata.Type != assetType)
						{
							continue;
						}

						const std::string assetName = metadata.FilePath.stem().string();

						if (!searchString.empty() && !UI::IsMatchingSearch(assetName, searchString))
						{
							continue;
						}

						bool is_selected = (current == metadata.Handle);
						if (ImGui::Selectable(assetName.c_str(), is_selected))
						{
							current = metadata.Handle;
							selected = metadata.Handle;
							modified = true;
						}

						if (forwardFocus)
						{
							forwardFocus = false;
						}
						else if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					//ImGui::EndChild();
					ImGui::EndListBox();
				}
			}
			if (modified)
			{
				ImGui::CloseCurrentPopup();
			}

			UI::EndPopup();
		}

		if (UI::IsItemDisabled())
		{
			ImGui::PopStyleVar();
		}

		return modified;
	}

	bool Widgets::AssetSearchPopup(const char* ID, AssetHandle& selected, bool* cleared, const char* hint, const ImVec2& size, std::initializer_list<AssetType> assetTypes)
	{
		UI::ScopedColor popupBG(ImGuiCol_PopupBg, UI::ColorWithMultipliedValue(Colors::Theme::background, 1.6f));
		bool modified = false;
		std::string preview;
		float itemHeight = size.y / 20.0f;
		const auto& assetRegistry = AssetManager::GetAssetRegistry();
		
		AssetHandle current = selected;
		if (UI::IsItemDisabled())
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		ImGui::SetNextWindowSize({ size.x, 0.0f });
		static bool grabFocus = true;
		if (UI::BeginPopup(ID, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			static std::string searchString;
			if (ImGui::GetCurrentWindow()->Appearing)
			{
				grabFocus = true;
				searchString.clear();
			}

			// Search widget
			UI::ShiftCursor(3.0f, 2.0f);
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() * 2.0f);
			SearchWidget(searchString, hint, &grabFocus);
			const bool searching = !searchString.empty();

			// Clear property button
			if (cleared != nullptr)
			{
				UI::ScopedColorStack buttonColors(
					ImGuiCol_Button, UI::ColorWithMultipliedValue(Colors::Theme::background, 1.0f),
					ImGuiCol_ButtonHovered, UI::ColorWithMultipliedValue(Colors::Theme::background, 1.2f),
					ImGuiCol_ButtonActive, UI::ColorWithMultipliedValue(Colors::Theme::background, 0.9f));
				UI::ScopedStyle border(ImGuiStyleVar_FrameBorderSize, 0.0f);
				ImGui::SetCursorPosX(0);
				ImGui::PushItemFlag(ImGuiItemFlags_NoNav, searching);
				if (ImGui::Button("CLEAR", { ImGui::GetWindowWidth(), 0.0f }))
				{
					*cleared = true;
					modified = true;
				}
				ImGui::PopItemFlag();
			}

			// List of assets
			{
				UI::ScopedColor listBoxBg(ImGuiCol_FrameBg, IM_COL32_DISABLE);
				UI::ScopedColor listBoxBorder(ImGuiCol_Border, IM_COL32_DISABLE);
				ImGuiID listID = ImGui::GetID("##SearchListBox");
				
				if (ImGui::BeginListBox("##SearchListBox", ImVec2(-FLT_MIN, 0.0f)))
				{
					bool forwardFocus = false;
					ImGuiContext& g = *GImGui;
					if (g.NavJustMovedToId != 0)
					{
						if (g.NavJustMovedToId == listID)
						{
							forwardFocus = true;
							// ActivateItem moves keyboard navigation focuse inside of the window
							ImGui::ActivateItem(listID);
							ImGui::SetKeyboardFocusHere(1);
						}
					}
					for (auto it = assetRegistry.cbegin(); it != assetRegistry.cend(); it++)
					{
						const auto& [path, metadata] = *it;
						bool isValidType = false;
						for (AssetType type : assetTypes)
						{
							if (metadata.Type == type)
							{
								isValidType = true;
								break;
							}
						}
						
						if (!isValidType)
						{
							continue;
						}

						const std::string assetName = metadata.FilePath.stem().string();
						if (!searchString.empty() && !UI::IsMatchingSearch(assetName, searchString))
						{
							continue;
						}

						bool is_selected = (current == metadata.Handle);
						if (ImGui::Selectable(assetName.c_str(), is_selected))
						{
							current = metadata.Handle;
							selected = metadata.Handle;
							modified = true;
						}

						{
							const std::string& assetType = Utils::ToUpper(Utils::AssetTypeToString(metadata.Type));
							ImVec2 textSize = ImGui::CalcTextSize(assetType.c_str());
							ImVec2 rectSize = ImGui::GetItemRectSize();
							float paddingX = ImGui::GetStyle().FramePadding.x;
							ImGui::SameLine(rectSize.x - textSize.x - paddingX);
							UI::ScopedColor textColor(ImGuiCol_Text, Colors::Theme::textDarker);
							ImGui::TextUnformatted(assetType.c_str());
						}

						if (forwardFocus)
						{
							forwardFocus = false;
						}
						else if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					//ImGui::EndChild();
					ImGui::EndListBox();
				}
			}

			if (modified)
			{
				ImGui::CloseCurrentPopup();
			}

			UI::EndPopup();
		}
		
		if (UI::IsItemDisabled())
		{
			ImGui::PopStyleVar();
		}

		return modified;
	}
}