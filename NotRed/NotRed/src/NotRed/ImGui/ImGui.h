#pragma once

#include "NotRed/Asset/Assets.h"

//TODO: This shouldn't be here
#include "NotRed/Asset/AssetManager.h"

#include "imgui/imgui.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace NR::UI 
{
	static int sUIContextID = 0;
	static uint32_t sCounter = 0;
	static char sIDBuffer[16];

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

	static bool Property(const char* label, std::string& value, bool error = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char buffer[256];
		strcpy(buffer, value.c_str());

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		itoa(sCounter++, sIDBuffer + 2, 16);

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
		itoa(sCounter++, sIDBuffer + 2, 16);
		ImGui::InputText(sIDBuffer, (char*)value, 256, ImGuiInputTextFlags_ReadOnly);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
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
		itoa(sCounter++, sIDBuffer + 2, 16);
		if (ImGui::Checkbox(sIDBuffer, &value))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, int& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		itoa(sCounter++, sIDBuffer + 2, 16);
		if (ImGui::DragInt(sIDBuffer, &value))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

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
		itoa(sCounter++, sIDBuffer + 2, 16);
		if (ImGui::SliderInt(sIDBuffer, &value, min, max))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	static bool Property(const char* label, float& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f, bool readOnly = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		sIDBuffer[0] = '#';
		sIDBuffer[1] = '#';
		memset(sIDBuffer + 2, 0, 14);
		itoa(sCounter++, sIDBuffer + 2, 16);

		if (!readOnly)
		{
			if (ImGui::DragFloat(sIDBuffer, &value, delta, min, max))
			{
				modified = true;
			}
		}
		else
		{
			ImGui::InputFloat(sIDBuffer, &value, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

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
		itoa(sCounter++, sIDBuffer + 2, 16);
		if (ImGui::DragFloat2(sIDBuffer, glm::value_ptr(value), delta))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

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
		itoa(sCounter++, sIDBuffer + 2, 16);
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
		itoa(sCounter++, sIDBuffer + 2, 16);
		if (ImGui::DragFloat3(sIDBuffer, glm::value_ptr(value), delta))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

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
		itoa(sCounter++, sIDBuffer + 2, 16);
		if (ImGui::DragFloat4(sIDBuffer, glm::value_ptr(value), delta))
		{
			modified = true;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

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
		itoa(sCounter++, sIDBuffer + 2, 16);
		if (ImGui::Checkbox(sIDBuffer, &value))
		{
			modified = true;
		}

		return modified;
	}

	static void EndCheckboxGroup()
	{
		ImGui::PopItemWidth();
		ImGui::NextColumn();
		sCheckboxCount = 0;
	}

	template<typename T>
	static bool PropertyAssetReference(const char* label, Ref<T>& object, AssetType supportedType)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char* assetName = ((Ref<Asset>&)object)->FileName.data();
		if (object)
		{
			char* assetName = ((Ref<Asset>&)object)->FileName.data();
			ImGui::InputText("##assetRef", assetName, 256, ImGuiInputTextFlags_ReadOnly);
		}
		else
		{
			ImGui::InputText("##assetRef", (char*)"Null", 256, ImGuiInputTextFlags_ReadOnly);
		}

		if (ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("asset_payload");

			if (data)
			{
				AssetHandle assetHandle = *(AssetHandle*)data->Data;
				if (AssetManager::IsAssetType(assetHandle, supportedType))
				{
					object = AssetManager::GetAsset<T>(assetHandle);
					modified = true;
				}
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		return modified;
	}
}