#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>

#include "NotRed/Asset/AssetMetadata.h"

//TODO: This shouldn't be here
#include "NotRed/Asset/AssetManager.h"

#include "NotRed/Renderer/Texture.h"
 
#include "imgui/imgui.h"

namespace NR::UI 
{
	static int sUIContextID = 0;
	static uint32_t sCounter = 0;
	static char sIDBuffer[16];
	static bool sNextItemReadOnly = false;

	static bool IsMouseEnabled()
	{
		return ImGui::GetIO().ConfigFlags & ~ImGuiConfigFlags_NoMouse;
	}

	static void SetMouseEnabled(const bool enable)
	{
		if (enable)
		{
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		}
		else
		{
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
		}
	}

	static void PushID()
	{
		ImGui::PushID(sUIContextID++);
		sCounter = 0;
	}

	static void PopID()
	{
		ImGui::PopID();
		--sUIContextID;
	}

	static void BeginPropertyGrid()
	{
		PushID();
		ImGui::Columns(2);
	}

	static void Separator()
	{
		ImGui::Separator();
	}

	static void SetNextPropertyReadOnly(bool isReadOnly = true)
	{
		sNextItemReadOnly = isReadOnly;
	}

	static bool Property(const char* label, std::string& value, bool error = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char buffer[256];
		strcpy_s<256>(buffer, value.c_str());

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

		if (error)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
		}
		if (ImGui::InputText(sIDBuffer, buffer, 256))
		{
			value = buffer;
			modified = true;
		}
		if (error)
		{
			ImGui::PopStyleColor();
		}
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static void Property(const char* label, const char* value)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		ImGui::InputText(sIDBuffer, (char*)value, 256, ImGuiInputTextFlags_ReadOnly);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	static bool Property(const char* label, char* value, size_t length)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		
		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		bool changed = ImGui::InputText(sIDBuffer, value, length, sNextItemReadOnly ? ImGuiInputTextFlags_ReadOnly : 0);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		return changed;
	}

	static bool Property(const char* label, bool& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (ImGui::Checkbox(sIDBuffer, &value))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, int& value, int min = 0, int max = 0)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (sNextItemReadOnly)
		{
			ImGui::InputInt(sIDBuffer, &value, 0, 0, ImGuiInputTextFlags_ReadOnly);
		}
		else
		{
			if (ImGui::DragInt(sIDBuffer, &value))
			{
				modified = true;
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		sNextItemReadOnly = false;
		return modified;
	}

	static bool PropertySlider(const char* label, int& value, int min, int max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (ImGui::SliderInt(sIDBuffer, &value, min, max))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}
	
	static bool PropertySlider(const char* label, float& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (ImGui::SliderFloat(sIDBuffer, &value, min, max))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertySlider(const char* label, glm::vec2& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (ImGui::SliderFloat2(sIDBuffer, glm::value_ptr(value), min, max))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertySlider(const char* label, glm::vec3& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (ImGui::SliderFloat3(sIDBuffer, glm::value_ptr(value), min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool PropertySlider(const char* label, glm::vec4& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (ImGui::SliderFloat4(sIDBuffer, glm::value_ptr(value), min, max))
			modified = true;

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, float& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);

		if (sNextItemReadOnly)
		{
			ImGui::InputFloat(sIDBuffer, &value, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
		}
		else
		{
			if (ImGui::DragFloat(sIDBuffer, &value, delta, min, max))
			{
				modified = true;
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		sNextItemReadOnly = false;
		return modified;
	}

	static bool Property(const char* label, glm::vec2& value, float delta = 0.1f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (sNextItemReadOnly)
		{
			ImGui::InputFloat2(sIDBuffer, glm::value_ptr(value), "%.3f", ImGuiInputTextFlags_ReadOnly);
		}
		else
		{
			if (ImGui::DragFloat2(sIDBuffer, glm::value_ptr(value), delta))
			{
				modified = true;
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		sNextItemReadOnly = false;
		return modified;
	}

	static bool PropertyColor(const char* label, glm::vec3& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (ImGui::ColorEdit3(sIDBuffer, glm::value_ptr(value)))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, glm::vec3& value, float delta = 0.1f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (sNextItemReadOnly)
		{
			ImGui::InputFloat3(sIDBuffer, glm::value_ptr(value), "%.3f", ImGuiInputTextFlags_ReadOnly);
		}
		else
		{
			if (ImGui::DragFloat3(sIDBuffer, glm::value_ptr(value), delta))
			{
				modified = true;
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		sNextItemReadOnly = false;
		return modified;
	}

	static bool Property(const char* label, glm::vec4& value, float delta = 0.1f)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);		
		if (sNextItemReadOnly)
		{
			ImGui::InputFloat4(sIDBuffer, glm::value_ptr(value), "%.3f", ImGuiInputTextFlags_ReadOnly);
		}
		else
		{
			if (ImGui::DragFloat4(sIDBuffer, glm::value_ptr(value), delta))
			{
				modified = true;
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		sNextItemReadOnly = false;
		return modified;
	}

	static void Property(const char* label, const std::string& value)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';

		memset(sIDBuffer + 2, 0, 14);
		itoa(sCounter++, sIDBuffer + 2, 16);

		ImGui::InputText(sIDBuffer, (char*)value.c_str(), value.size(), ImGuiInputTextFlags_ReadOnly);
		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	static bool PropertyDropdown(const char* label, const char** options, int32_t optionCount, int32_t* selected)
	{
		const char* current = options[*selected];
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		bool changed = false;
		const std::string id = "##" + std::string(label);
		if (ImGui::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; ++i)
			{
				const bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i], is_selected))
				{
					current = options[i];
					*selected = i;
					changed = true;
				}
				if (is_selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
		ImGui::NextColumn();
		return changed;
	}

	static bool PropertyDropdown(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected)
	{
		const char* current = options[*selected].c_str();
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		bool changed = false;

		const std::string id = "##" + std::string(label);
		if (ImGui::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; ++i)
			{
				const bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i].c_str(), is_selected))
				{
					current = options[i].c_str();
					*selected = i;
					changed = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return changed;
	}

	struct PropertyAssetReferenceSettings
	{
		bool AdvanceToNextColumn = true;
		bool NoItemSpacing = false; // After label
		float WidthOffset = 0.0f;
	};

	enum class PropertyAssetReferenceError
	{
		None, InvalidMetadata
	};

	static AssetHandle sPropertyAssetReferenceAssetHandle;

	template<typename T, typename Fn>
	static bool PropertyAssetReferenceTarget(
		const char* label, 
		const char* assetName, 
		Ref<T> source, 
		Fn&& targetFunc, 
		const PropertyAssetReferenceSettings& settings = PropertyAssetReferenceSettings()
	)
	{
		bool modified = false;
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		if (settings.NoItemSpacing)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });
		}

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
		float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
		UI::PushID();
		float itemHeight = 28.0f;

		if (source)
		{
			if (!source->IsFlagSet(AssetFlag::Missing))
			{
				ImGui::Button((char*)assetName, { width, itemHeight });
			}
			else
			{
				ImGui::Button("Missing", { width, itemHeight });
			}
		}
		else
		{
			ImGui::Button("Null", { width, itemHeight });
		}

		UI::PopID();
		ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

		if (ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("asset_payload");
			if (data)
			{
				AssetHandle assetHandle = *(AssetHandle*)data->Data;
				sPropertyAssetReferenceAssetHandle = assetHandle;
				Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
				if (asset->GetAssetType() == T::GetStaticType())
				{
					targetFunc(asset.As<T>());
					modified = true;
				}
			}
		}

		ImGui::PopItemWidth();

		if (settings.AdvanceToNextColumn)
		{
			ImGui::NextColumn();
		}
		if (settings.NoItemSpacing)
		{
			ImGui::PopStyleVar();
		}

		return modified;
	}

	static void EndPropertyGrid()
	{
		ImGui::Columns(1);
		PopID();
	}

	static bool BeginTreeNode(const char* name, bool defaultOpen = true)
	{
		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
		if (defaultOpen)
		{
			treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		return ImGui::TreeNodeEx(name, treeNodeFlags);
	}

	static void EndTreeNode()
	{
		ImGui::TreePop();
	}

	static int sCheckboxCount = 0;

	static void BeginCheckboxGroup(const char* label)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
	}

	static bool PropertyCheckboxGroup(const char* label, bool& value)
	{
		bool modified = false;

		if (++sCheckboxCount > 1)
		{
			ImGui::SameLine();
		}

		ImGui::Text(label);
		ImGui::SameLine();

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		sprintf_s(sIDBuffer + 2, 14, "%o", sCounter++);
		if (ImGui::Checkbox(sIDBuffer, &value))
		{
			modified = true;
		}

		return modified;
	}

	static bool Button(const char* label, const ImVec2& size = ImVec2(0, 0))
	{
		bool result = ImGui::Button(label, size);
		ImGui::NextColumn();
		return result;
	}

	static void EndCheckboxGroup()
	{
		ImGui::PopItemWidth();
		ImGui::NextColumn();
		sCheckboxCount = 0;
	}

	template<typename T>
	static bool PropertyAssetReference(
		const char* label, 
		Ref<T>& object, 
		PropertyAssetReferenceError* outError = nullptr,
		const PropertyAssetReferenceSettings& settings = PropertyAssetReferenceSettings()
	)
	{
		bool modified = false;
		if (outError)
		{
			*outError = PropertyAssetReferenceError::None;
		}

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
		float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
		UI::PushID();
		float itemHeight = 28.0f;
		
		if (object)
		{
			if (!object->IsFlagSet(AssetFlag::Missing))
			{
				std::string assetFileName = AssetManager::GetMetadata(object->Handle).FilePath.stem().string();
				ImGui::Button((char*)assetFileName.c_str(), { width, itemHeight });
			}
			else
			{
				ImGui::Button("Missing", { width, itemHeight });
			}
		}
		else
		{
			ImGui::Button("Null", { width, itemHeight });
		}

		UI::PopID();
		ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

		if (ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("asset_payload");

			if (data)
			{
				AssetHandle assetHandle = *(AssetHandle*)data->Data;
				sPropertyAssetReferenceAssetHandle = assetHandle;
				Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
				if (asset->GetAssetType() == T::GetStaticType())
				{
					object = asset.As<T>();
					modified = true;
				}
			}
		}

		ImGui::PopItemWidth();
		if (settings.AdvanceToNextColumn)
		{
			ImGui::NextColumn();
		}
		return modified;
	}

	template<typename TAssetType, typename TConversionType, typename Fn>
	static bool PropertyAssetReferenceWithConversion(
		const char* label, 
		Ref<TAssetType>& object,
		Fn&& conversionFunc, 
		PropertyAssetReferenceError* outError = nullptr, 
		const PropertyAssetReferenceSettings& settings = PropertyAssetReferenceSettings()
	)
	{
		bool succeeded = false;
		if (outError)
		{
			*outError = PropertyAssetReferenceError::None;
		}

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		ImVec2 originalButtonTextAlign = ImGui::GetStyle().ButtonTextAlign;
		ImGui::GetStyle().ButtonTextAlign = { 0.0f, 0.5f };
		float width = ImGui::GetContentRegionAvail().x - settings.WidthOffset;
		UI::PushID();
		float itemHeight = 28.0f;
		
		if (object)
		{
			if (!object->IsFlagSet(AssetFlag::Missing))
			{
				std::string assetFileName = AssetManager::GetMetadata(object->Handle).FilePath.stem().string();
				ImGui::Button((char*)assetFileName.c_str(), { width, itemHeight });
			}
			else
			{
				ImGui::Button("Missing", { width, itemHeight });
			}
		}
		else
		{
			ImGui::Button("Null", { width, itemHeight });
		}

		UI::PopID();
		ImGui::GetStyle().ButtonTextAlign = originalButtonTextAlign;

		if (ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("asset_payload");
			if (data)
			{
				AssetHandle assetHandle = *(AssetHandle*)data->Data;
				sPropertyAssetReferenceAssetHandle = assetHandle;
				Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
				if (asset)
				{
					// No conversion necessary 
					if (asset->GetAssetType() == TAssetType::GetStaticType())
					{
						object = asset.As<TAssetType>();
						succeeded = true;
					}
					// Convert
					else if (asset->GetAssetType() == TConversionType::GetStaticType())
					{
						conversionFunc(asset.As<TConversionType>());
						succeeded = false; // Must be handled my conversion function
					}
				}
				else
				{
					if (outError)
					{
						*outError = PropertyAssetReferenceError::InvalidMetadata;
					}
				}
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		
		return succeeded;
	}

	void Image(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void Image(const Ref<Image2D>& image, uint32_t layer, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void Image(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	void ImageMip(const Ref<Image2D>& image, uint32_t mip, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));
	bool ImageButton(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool ImageButton(const char* stringID, const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool ImageButton(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool ImageButton(const char* stringID, const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
}