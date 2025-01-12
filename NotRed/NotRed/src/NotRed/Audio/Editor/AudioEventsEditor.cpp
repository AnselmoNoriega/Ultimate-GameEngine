#include <nrpch.h>
#include "AudioEventsEditor.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "NotRed/ImGui/ImGui.h"
#include "NotRed/ImGui/Colors.h"
#include "NotRed/ImGui/CustomTreeNode.h"

#include "NotRed/Audio/AudioEvents/AudioCommandRegistry.h"
#include "NotRed/Audio/AudioEvents/CommandID.h"

#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Core/Input.h"
#include "NotRed/Renderer/Texture.h"

#include "yaml-cpp/yaml.h"

#include "NotRed/Util/SerializationMacros.h"

#include <iomanip>

namespace NR
{
	static bool PropertyDropdownNoLabel(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected)
	{
		const char* current = options[*selected].c_str();

		bool changed = false;

		std::string id = "##" + std::string(label);
		if (ImGui::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; i++)
			{
				bool isselected = (current == options[i]);
				if (ImGui::Selectable(options[i].c_str(), isselected))
				{
					current = options[i].c_str();
					*selected = i;
					changed = true;
				}
				if (isselected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		return changed;
	}

	template<typename T>
	static bool PropertyAssetReferenceNoLabel(const char* label, Ref<T>& object, AssetType supportedType, bool dropTargetEnabled = true)
	{
		bool modified = false;

		if (object)
		{
			if (!object->IsFlagSet(AssetFlag::Missing))
			{
				auto assetFileName = Utils::RemoveExtension(AssetManager::GetMetadata(object->Handle).FilePath.filename().string());
				ImGui::InputText("##assetRef", (char*)assetFileName.c_str(), 256, ImGuiInputTextFlags_ReadOnly);
			}
			else
			{
				ImGui::InputText("##assetRef", (char*)"Missing", 256, ImGuiInputTextFlags_ReadOnly);
			}
		}
		else
		{
			ImGui::InputText("##assetRef", (char*)"Null", 256, ImGuiInputTextFlags_ReadOnly);
		}

		if (dropTargetEnabled)
		{
			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("asset_payload");

				if (data)
				{
					AssetHandle assetHandle = *(AssetHandle*)data->Data;
					Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
					if (asset->GetAssetType() == T::GetStaticType())
					{
						object = asset.As<T>();
						modified = true;
					}
				}
			}
		}

		return modified;
	}

	static bool NameProperty(const char* label, std::string& value, bool error = false, ImGuiInputTextFlags flags = 0)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char buffer[256];
		strcpy_s<256>(buffer, value.c_str());

		//sIDBuffer[0] = '#';
		//sIDBuffer[1] = '#';
		//memset(sIDBuffer + 2, 0, 14);
		//itoa(sCounter++, sIDBuffer + 2, 16);
		std::string id = "##" + std::string(label);

		if (error)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
		}
		if (ImGui::InputText(id.c_str(), buffer, 256, flags))
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


	using namespace Audio;

	OrderedVector<TreeModel<CommandID>::Node*> AudioEventsEditor::sSelection;

	std::vector<TreeModel<CommandID>::Node*> AudioEventsEditor::sMarkedForDelete;
	std::vector <AudioEventsEditor::ReorderData> AudioEventsEditor::sMarkedForReorder;

	TreeModel<CommandID>::Node* AudioEventsEditor::sRenamingEntry = nullptr;

	bool AudioEventsEditor::sopen = false;
	TreeModel<CommandID>::Node* AudioEventsEditor::sNewlyCreatedNode = nullptr;

	TreeModel<CommandID> AudioEventsEditor::sTree;

	ImGuiWindow* AudioEventsEditor::sWindowHandle = nullptr;

	bool AudioEventsEditor::f_Renaming = false;
	std::string AudioEventsEditor::sRenameConflict;

	Ref<Texture2D> AudioEventsEditor::sFolderIcon;
	Ref<Texture2D> AudioEventsEditor::sMoveIcon;
	Ref<Texture2D> AudioEventsEditor::sDeleteIcon;

	void AudioEventsEditor::Init()
	{
		AudioCommandRegistry::onRegistryChange = [] { OnRegistryChanged(); };
		sFolderIcon = Texture2D::Create("Resources/Editor/folder_closed.png");
		sMoveIcon = Texture2D::Create("Resources/Editor/move.png");
		sDeleteIcon = Texture2D::Create("Resources/Editor/close.png");
	}

	void AudioEventsEditor::OnOpennesChange(bool opened)
	{
		if (!opened)
		{
			AudioCommandRegistry::WriteRegistryToFile();
		}
	}

	void AudioEventsEditor::Shutdown()
	{
		sDeleteIcon.Reset();
		sMoveIcon.Reset();
		sFolderIcon.Reset();
		AudioCommandRegistry::onRegistryChange = nullptr;
	}

	void AudioEventsEditor::OnRegistryChanged()
	{
	}

	bool AudioEventsEditor::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		switch (e.GetKeyCode())
		{
		case KeyCode::Escape:
			if (f_Renaming)
			{
				// If text renaming is active, deactivate it
				// Just clearing the flag, focus handled by ImGui
				f_Renaming = false;
				sRenamingEntry = nullptr;
			}
			else if (!sSelection.IsEmpty())
			{
				sSelection.Clear();
			}
			return true;
			break;
		case KeyCode::Delete:
			if (!sSelection.IsEmpty())
			{
				for (auto* item : sSelection.GetVector())
					OnDeleteNode(item);
				sSelection.Clear();
				return true;
			}
			break;
		case KeyCode::F2:
			ActivateRename(sSelection.GetLast());
			return true;
			break;
		}
		//if (Input::IsKeyPressed(NR_KEY_LEFT_SHIFT))
		//{
		//	switch (e.GetKeyCode())
		//	{
		//	case KeyCode::Up:
		//	{
		//		//? This messes up with default selection via arrow keys when focesed item changes
		//		
		//		//auto id = CommandID(m_SelectionContext.c_str());
		//		//sOrderVector.SetNewPosition(id, sOrderVector.GetIndex(id) - 1);
		//		return true;
		//		break;
		//	}
		//	case KeyCode::Down:
		//		//auto id = CommandID(m_SelectionContext.c_str());
		//		//sOrderVector.SetNewPosition(id, sOrderVector.GetIndex(id) + 1);
		//		return true;
		//		break;
		//	}
		//}

		return false;
	}


	void AudioEventsEditor::ImGuiRender(bool& show)
	{
		if (!show)
		{
			if (sopen)
			{
				sopen = false;
				OnOpennesChange(false);
			}
			return;
		}

		if (!sopen)
		{
			OnOpennesChange(true);
		}
		sopen = true;

		ImGui::SetNextWindowSizeConstraints({ 600.0f, 400.0f }, ImGui::GetWindowContentRegionMax());

		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::text);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.0f });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 8.0f });
		ImGui::Begin("Audio Events Editor", &show, ImGuiWindowFlags_NoCollapse);
		sWindowHandle = ImGui::GetCurrentWindow();
		ImGui::PopStyleVar(2);

		const auto width = ImGui::GetContentRegionAvail().x;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Theme::backgroundDark);
		ImGui::PushStyleColor(ImGuiCol_Border, Colors::Theme::backgroundDark);

		ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable
			| ImGuiTableFlags_SizingFixedFit
			| ImGuiTableFlags_PadOuterX
			| ImGuiTableFlags_NoHostExtendY
			| ImGuiTableFlags_BordersInnerV;

		UI::ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 8.0f));

		UI::ShiftCursorX(4.0f);

		UI::PushID();

		if (ImGui::BeginTable(
			UI::GenerateID(), 2, tableFlags, 
			ImVec2(ImGui::GetContentRegionAvail().x,
			ImGui::GetContentRegionAvail().y - 8.0f)))
		{
			ImGui::TableSetupColumn("Events", 0, 300.0f);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			DrawEventsList();

			ImGui::TableSetColumnIndex(1);

			DrawSelectionDetails();
			ImGui::EndTable();
		}

		UI::PopID();
		ImGui::PopStyleColor(3);

		ImGui::End(); // Audio Evnets Editor

		sWindowHandle = nullptr;
	}

	void AudioEventsEditor::DrawTreeNode(CommandsTree::Node* treeNode, bool isRootNode /*= false*/)
	{
		// ImGui item height tweaks
		auto* window = ImGui::GetCurrentWindow();
		window->DC.CurrLineSize.y = 20.0f;
		window->DC.CurrLineTextBaseOffset = 3.0f;
		//---------------------------------------------

		const bool isFolder = !treeNode->Value.has_value();
		const bool isParentRootNode = treeNode->Parent == &treeNode->Tree->RootNode;

		// TODO: need Selection Manager!
		auto isSelected = [&] {
			for (auto s : sSelection.GetVector())
			{
				if (isFolder)
				{
					if (s == treeNode)
					{
						return true;
					}
				}
				else if (s->Value)
				{
					if (*s->Value == *treeNode->Value)
					{
						return true;
					}
				}
			}
			return false;
			};

		bool selected = !isRootNode && isSelected();
		bool opened = false;
		bool deleteNode = false;

		if (isRootNode)
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;
			ImGuiItemFlags itemFlags_ = ImGuiItemFlags_Disabled | ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus;
			ImGui::PushItemFlag(itemFlags_, true);
			opened = ImGui::TreeNodeWithIcon(nullptr, "##RootNode", flags);
			ImGui::PopItemFlag();
			ImGui::SetItemAllowOverlap();
			ImGui::SameLine();
			ImGui::Unindent();
		}
		else
		{
			const char* name = isFolder ? treeNode->Name.c_str() : [&]
				{
					CommandID commandID = *treeNode->Value;
					auto& command = AudioCommandRegistry::GetCommand<TriggerCommand>(commandID);
					return command.DebugName.c_str();
				}();

			const ImRect itemRect = { window->WorkRect.Min.x, window->DC.CursorPos.y,
									  window->WorkRect.Max.x, window->DC.CursorPos.y + window->DC.CurrLineSize.y };
			const bool isHovering = ImGui::ItemHoverable(itemRect, ImGui::GetID(name));
			
			const bool isItemClicked = [&itemRect, &name, isHovering]
				{
					if (isHovering)
					{
						return ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);
					}

					return false;
				}();
			
			const bool isWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			ImGuiTreeNodeFlags flags = (selected ? ImGuiTreeNodeFlags_Selected : 0)
				| ImGuiTreeNodeFlags_SpanFullWidth
				| (isFolder ? 0 : ImGuiTreeNodeFlags_Leaf);

			// Fill background
			//----------------

			auto fillWithColor = [&](const ImColor& color)
				{
					ImGui::GetWindowDrawList()->AddRectFilled(itemRect.Min, itemRect.Max, color);
				};

			// If any descendant node selected, fill this folder with light color
			if (selected || isItemClicked)
			{
				if (isWindowFocused)
				{
					fillWithColor(Colors::Theme::selection);
				}
				else
				{
					const ImColor col = UI::ColorWithMultipliedValue(Colors::Theme::selection, 0.8f);
					fillWithColor(UI::ColorWithMultipliedSaturation(col, 0.7f));
				}
				ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::backgroundDark);
			}
			else if (isFolder && !selected)
			{
				auto isAnyDescendantSelected = [](CommandsTree::Node* treeNode, auto isAnyDescendantSelected) -> bool
					{
						if (!treeNode->Children.empty())
						{
							for (auto& [name, childNode] : treeNode->Children)
							{
								if (isAnyDescendantSelected(childNode.Raw(), isAnyDescendantSelected))
								{
									return true;
								}
							}
						}
						else
						{
							return sSelection.Contains(treeNode);
						}

						return false;
					};

				if (isAnyDescendantSelected(treeNode, isAnyDescendantSelected))
				{
					fillWithColor(Colors::Theme::selectionMuted);
				}
			}

			// Tree Node
			//----------
			auto isDescendant = [](CommandsTree::Node* descendant, CommandsTree::Node* treeNode, auto isDescendant) -> bool
				{
					const bool isFolder = !treeNode->Value.has_value();
					if (!isFolder)
					{
						return false;
					}
					else
					{
						if (treeNode->Children.find(descendant->Name) != treeNode->Children.end())
						{
							return true;
						}

						for (auto& [name, childNode] : treeNode->Children)
						{
							if (isDescendant(descendant, childNode.Raw(), isDescendant))
							{
								return true;
							}
						}
					}

					return false;
				};

			if (isFolder)
			{
				if (sNewlyCreatedNode != nullptr && isDescendant(sNewlyCreatedNode, treeNode, isDescendant))
				{
					ImGui::SetNextItemOpen(true);
				}
				opened = ImGui::TreeNodeWithIcon(sFolderIcon, ImGui::GetID(name), flags, name, nullptr);
			}
			else
			{
				opened = ImGui::TreeNodeWithIcon(nullptr, ImGui::GetID(name), flags, name, nullptr);
			}

			// Draw an outline around the last selected item which details are now displayed
			if (selected && sSelection.GetLast() == treeNode && isWindowFocused)
			{
				const ImU32 frameColor = Colors::Theme::text;
				ImGui::PushStyleColor(ImGuiCol_Border, frameColor);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
				ImGui::RenderFrameBorder(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0.0f);
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			}

			if (selected || isItemClicked)
			{
				ImGui::PopStyleColor();
			}

			// Handle selection
			//-----------------
			const bool itemClicked = ImGui::IsMouseReleased(ImGuiMouseButton_Left)
				&& ImGui::IsItemHovered()
				&& !ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.01f);

			// Activate newly created item and its name text input
			const bool entryWasJustCreated = sNewlyCreatedNode == treeNode;
			if (entryWasJustCreated)
			{
				sSelection.Clear();
				sSelection.PushBack(treeNode);
			}
			else if (itemClicked || UI::NavigatedTo())
			{
				if (Input::IsKeyPressed(KeyCode::LeftControl))
				{
					if (selected)
						sSelection.Erase(treeNode);
					else
						sSelection.PushBack(treeNode);
				}
				else if (Input::IsKeyPressed(KeyCode::LeftShift))
				{
					// Select all items between the first one selected and this one (within this parent folder?)
					auto& siblings = treeNode->Parent->Children;
					auto selectionStart = siblings.find(sSelection.GetFirst()->Name);

					// Only allowing Shift select within the same folder
					if (selectionStart != siblings.end())
					{
						const auto& thisNode = siblings.find(treeNode->Name);

						auto compare = siblings.key_comp();
						const bool before = compare(treeNode->Name, selectionStart->first);
						const bool after = compare(selectionStart->first, treeNode->Name);

						if (before || after)
							sSelection.Clear();

						if (before)
						{
							auto end = siblings.find(treeNode->Name);
							auto rend = std::make_reverse_iterator(end);
							auto rstart = std::make_reverse_iterator(++selectionStart);

							for (auto& it = rstart; it != rend; it++)
							{
								sSelection.PushBack(it->second.Raw());
							}
						}
						else if (after)
						{
							auto end = siblings.upper_bound(treeNode->Name);
							for (auto it = siblings.lower_bound(selectionStart->first); it != end; it++)
							{
								sSelection.PushBack(it->second.Raw());
							}
						}
					}
				}
				else
				{
					sSelection.Clear();
					sSelection.PushBack(treeNode);
				}
			}

			// Context menu
			//-------------
			if (ImGui::BeginPopupContextItem())
			{
				// TODO: add "add event", "add folder" options

				if (ImGui::MenuItem("Rename", "F2"))
				{
					// This prevents loosing selection when Right-Clicking to rename not selected entry
					if (Input::IsKeyPressed(KeyCode::LeftControl))
					{
						if (!sSelection.Contains(treeNode))
						{
							sSelection.PushBack(treeNode);
						}
					}

					ActivateRename(treeNode);
				}

				if (ImGui::MenuItem("Delete", "Delete"))
				{
					deleteNode = true;
				}

				ImGui::EndPopup();
			}

			// Drag & Drop
			//------------
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::Text(name);

				auto& map = treeNode->Parent->Children;
				auto it = map.find(treeNode->Name);

				NR_CORE_ASSERT(it != map.end());

				Ref<CommandsTree::Node>& nodeRef = it->second;
				ImGui::SetDragDropPayload("audio_eventseditor", &nodeRef, sizeof(Ref<CommandsTree::Node>));
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("audio_eventseditor", isFolder ? 0 : ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

				if (payload)
				{
					Ref<CommandsTree::Node> droppedHandle = *(Ref<CommandsTree::Node>*)payload->Data;

					ReorderData reorderData
					{
						droppedHandle,
						isFolder ? treeNode : treeNode->Parent,
					};

					sMarkedForReorder.push_back(reorderData);
				}

				ImGui::EndDragDropTarget();
			}
		}


		//--- Draw Children ---
		//---------------------

		if (opened)
		{
			if (isFolder)
			{
				// First draw nested folders
				for (auto& [name, childNode] : treeNode->Children)
				{
					const bool childIsFolder = !childNode->Value.has_value();

					if (childIsFolder)
						DrawTreeNode(childNode.Raw());
				}

				// Then draw nested Events
				for (auto& [name, childNode] : treeNode->Children)
				{
					const bool childIsFolder = !childNode->Value.has_value();

					if (!childIsFolder)
						DrawTreeNode(childNode.Raw());
				}
			}

			ImGui::TreePop();
		}


		// Defer deletion until end of node UI
		if (deleteNode)
		{
			if (isSelected())
			{
				sSelection.Erase(treeNode);
			}

			OnDeleteNode(treeNode);
		}
	}

	void AudioEventsEditor::OnSelectionChanged(CommandID commandID)
	{
	}

	void AudioEventsEditor::OnDeleteNode(CommandsTree::Node* treeNode)
	{
		// Mark for deletion and delete after entries all has been drawn
		sMarkedForDelete.push_back(treeNode);
	}

	std::string AudioEventsEditor::GetUniqueName()
	{
		int counter = 0;
		auto checkID = [&counter](auto checkID) -> std::string
			{
				++counter;
				const std::string counterStr = [&counter] {
					if (counter < 10)
						return "0" + std::to_string(counter);
					else
						return std::to_string(counter);
					}();  // Pad with 0 if < 10;

				std::string idstr = "New_Event_" + counterStr;
				CommandID id = CommandID::FromString(idstr.c_str());
				if (AudioCommandRegistry::DoesCommandExist<TriggerCommand>(id))
					return checkID(checkID);
				else
				{
					return idstr;
				}
			};

		return checkID(checkID);
	}

	std::string AudioEventsEditor::GetUniqueFolderName(CommandsTree::Node* parentNode, const std::string& nameBase)
	{
		int counter = 0;
		auto checkName = [&counter, &parentNode, &nameBase](auto checkName) -> std::string
			{
				++counter;
				const std::string counterStr = [&counter] {
					if (counter < 10)
						return "0" + std::to_string(counter);
					else
						return std::to_string(counter);
					}();  // Pad with 0 if < 10;

				std::string idstr = nameBase + "_" + counterStr;

				auto nameTaken = [&parentNode](const std::string name) {
					const auto& siblings = parentNode->Children;
					return siblings.find(name) != siblings.end();
					};

				if (nameTaken(idstr))
					return checkName(checkName);
				else
				{
					return idstr;
				}
			};

		return checkName(checkName);
	}

	void AudioEventsEditor::ActivateRename(CommandsTree::Node* treeNode)
	{
		f_Renaming = true;
		// Used to make renaming text field active
		sRenamingEntry = treeNode;
	}

	void AudioEventsEditor::OnEntryRenamed(CommandsTree::Node* treeNode, const char* newName)
	{
		CommandID oldCommandID = *treeNode->Value;

		if (AudioCommandRegistry::AddNewCommand<TriggerCommand>(newName))
		{
			CommandID newID = CommandID::FromString(newName);
			auto& newCommand = AudioCommandRegistry::GetCommand<TriggerCommand>(newID);

			// Copy old command content into the new command
			newCommand = AudioCommandRegistry::GetCommand<TriggerCommand>(oldCommandID);

			// We copied old name, so need to change it back to the new one
			newCommand.DebugName = newName;

			// Remove old command
			AudioCommandRegistry::RemoveCommand<TriggerCommand>(oldCommandID);

			auto* parent = treeNode->Parent;
			auto nodeHandle = parent->Children.extract(treeNode->Name);

			treeNode->Value = newID;
			treeNode->Name = newName;

			nodeHandle.key() = newName;
			bool inserted = parent->Children.insert(std::move(nodeHandle)).inserted;
			NR_CORE_ASSERT(inserted);
		}
		else
		{
			NR_CORE_ERROR("Trigger with the name {0} already exists! Audio command names must be unique.", newName);
		}

		f_Renaming = false;
		sRenamingEntry = nullptr;
	}


	void AudioEventsEditor::DrawEventsList()
	{
		auto singleColumnSeparator = []
			{
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				ImVec2 p = ImGui::GetCursorScreenPos();
				draw_list->AddLine(ImVec2(p.x - 9999, p.y), ImVec2(p.x + 9999, p.y), ImGui::GetColorU32(ImGuiCol_Border));
			};

		if (ImGui::Button("New Event", { 80.0, 28.0f }))
		{
			const std::string name = GetUniqueName();
			AudioCommandRegistry::AddNewCommand<TriggerCommand>(name.c_str());

			auto id = CommandID(name.c_str());

			auto newNode = Ref<CommandsTree::Node>::Create(&sTree, id);
			newNode->Name = name;
			sNewlyCreatedNode = newNode.Raw();

			if (!sSelection.IsEmpty())
			{
				auto* selectedNode = sSelection.GetLast();
				const bool isSelectedFolder = !selectedNode->Value.has_value();

				if (!isSelectedFolder)
				{
					// Add sibling node after this one
					auto* parent = selectedNode->Parent;
					parent->Children.emplace(name, newNode);
					newNode->Parent = parent;
				}
				else
				{
					// Add child node
					selectedNode->Children.emplace(name, newNode);
					newNode->Parent = selectedNode;
				}
			}
			else
			{
				sTree.RootNode.Children.emplace(name, newNode);
				newNode->Parent = &sTree.RootNode;
			}

			ActivateRename(newNode.Raw());
		}

		ImGui::SameLine();
		if (ImGui::Button("Add Folder", { 80.0, 28.0f }))
		{
			auto newFolderNode = Ref<CommandsTree::Node>::Create(&sTree);
			sNewlyCreatedNode = newFolderNode.Raw();

			if (!sSelection.IsEmpty())
			{
				auto* selectedNode = sSelection.GetLast();
				if (selectedNode->Value)
				{
					// Add sibling node after this one
					auto* parent = selectedNode->Parent;
					newFolderNode->Parent = parent;
					newFolderNode->Name = GetUniqueFolderName(parent, "New Folder");
					parent->Children.emplace(newFolderNode->Name, newFolderNode);
				}
				else
				{
					// Add child node
					newFolderNode->Parent = selectedNode;
					newFolderNode->Name = GetUniqueFolderName(selectedNode, "New Folder");
					selectedNode->Children.emplace(newFolderNode->Name, newFolderNode);

				}
			}
			else
			{
				newFolderNode->Parent = &sTree.RootNode;
				newFolderNode->Name = GetUniqueFolderName(&sTree.RootNode, "New Folder");
				sTree.RootNode.Children.emplace(newFolderNode->Name, newFolderNode);
			}
		}

		//--- Handle items deletion ---
		//-----------------------------

		for (auto& node : sMarkedForDelete)
		{
			auto deleteNode = [](CommandsTree::Node* treeNode, auto deleteNode) -> void
				{
					const bool isFolder = !treeNode->Value.has_value();
					if (isFolder)
					{
						for (auto& [name, childNode] : treeNode->Children)
							deleteNode(childNode.Raw(), deleteNode);

						treeNode->Children.clear();
					}
					else
					{
						CommandID id = *treeNode->Value;
						AudioCommandRegistry::RemoveCommand<TriggerCommand>(id);
					}
				};

			// Delete node and child nodes recursively
			deleteNode(node, deleteNode);

			node->Parent->Children.erase(node->Name);
		}
		sMarkedForDelete.clear();

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::BeginChild("EventsList");
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 0.0f });


		//--- Draw the events Tree ---
		//----------------------------
		{
			UI::ScopedColorStack itemBg(ImGuiCol_Header, IM_COL32_DISABLE,
				ImGuiCol_HeaderActive, IM_COL32_DISABLE);
			DrawTreeNode(&sTree.RootNode, true);
		}

		//--- Handle items reorder ---
		//----------------------------
		bool reorderResolved = true;

		for (auto& [node, newParent] : sMarkedForReorder)
		{
			// TODO: handle folder name collisions!!!

			auto* oldParent = node->Parent;
			if (oldParent == newParent)
			{
				// If moved item was selected, move all other selected items to its parent folder
				if (sSelection.Contains(node.Raw()))
				{
					for (auto& selectedNode : sSelection.GetVector())
					{
						auto* sNodeParent = selectedNode->Parent;

						if (selectedNode != newParent && sNodeParent != newParent)
						{
							// If parent folder also selected, resolve move as if moving the parent folder
							if (sSelection.Contains(selectedNode->Parent))
								continue;

							const bool isFolder = !selectedNode->Value.has_value();
							if (isFolder && newParent->Children.find(selectedNode->Name) != newParent->Children.end())
							{
								reorderResolved = CheckAndResolveNameConflict(newParent, selectedNode, sNodeParent);
							}
							else
							{
								newParent->Children.emplace(selectedNode->Name, selectedNode);
								sNodeParent->Children.erase(selectedNode->Name);
								selectedNode->Parent = newParent;
							}
						}
					}
				}
			}
			else
			{
				// If moved item was selected, move all selected items to the new parent folder

				if (sSelection.Contains(node.Raw()))
				{
					// If moved item was selected, move all selected items


					for (auto& selectedNode : sSelection.GetVector())
					{
						// If parent folder also selected, resolve move as if moving the parent folder
						if (sSelection.Contains(selectedNode->Parent))
							continue;

						auto* sNodeParent = selectedNode->Parent;

						if (selectedNode != newParent && sNodeParent != newParent)
						{

							const bool isFolder = !selectedNode->Value.has_value();

							if (isFolder && newParent->Children.find(selectedNode->Name) != newParent->Children.end())
							{
								reorderResolved = CheckAndResolveNameConflict(newParent, selectedNode, sNodeParent);
							}
							else
							{
								newParent->Children.emplace(selectedNode->Name, selectedNode);
								sNodeParent->Children.erase(selectedNode->Name);
								selectedNode->Parent = newParent;
							}
						}
					}
				}
				else
				{
					const bool isFolder = !node->Value.has_value();

					if (isFolder && newParent->Children.find(node->Name) != newParent->Children.end())
					{
						reorderResolved = CheckAndResolveNameConflict(newParent, node.Raw(), oldParent);
					}
					else
					{
						newParent->Children.emplace(node->Name, node);
						oldParent->Children.erase(node->Name);
						node->Parent = newParent;
					}
				}
			}
		}

		if (reorderResolved)
			sMarkedForReorder.clear();

		ImGui::PopStyleVar();
		ImGui::EndChild(); // EventsList

		// If dragged items dropped onto the background, move itmes up to the root node
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("audio_eventseditor");

			if (payload)
			{
				Ref<CommandsTree::Node> droppedHandle = *(Ref<CommandsTree::Node>*)payload->Data;

				ReorderData reorderData
				{
					droppedHandle,
					&sTree.RootNode,
				};

				sMarkedForReorder.push_back(reorderData);
			}

			ImGui::EndDragDropTarget();
		}
	}

	std::optional<std::string> AudioEventsEditor::CheckNameConflict(const std::string& name, CommandsTree::Node* parentNode, std::function<void()> onReplaceDestination, std::function<void()> onCancel)
	{
		// Setting default spacing values in case they were changed
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 4.0f });

		ImGui::OpenPopup("Name Conflict", ImGuiPopupFlags_NoOpenOverExistingPopup);

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2{ 400,0 });

		std::string conflictName = name;
		std::optional<std::string> resolvedName;

		if (ImGui::BeginPopupModal("Name Conflict", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("The destination contains folder with the same name.");
			ImGui::Separator();

			auto nameTaken = [parentNode](const std::string& name)
				{
					return parentNode->Children.find(name) != parentNode->Children.end();
				};

			ImGui::Spacing();

			UI::BeginPropertyGrid();
			if (NameProperty("Name", conflictName, true, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				if (!nameTaken(conflictName))
				{
					ImGui::CloseCurrentPopup();
					resolvedName = conflictName;
				}
			}

			UI::EndPropertyGrid();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Button("ASSIGN UNIQUE NAME"))
			{
				resolvedName = GetUniqueFolderName(parentNode, name);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("REPLACE DESTINATION"))
			{
				onReplaceDestination();
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("CANCEL"))
			{
				if (onCancel)
					onCancel();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

		return resolvedName;
	}

	bool AudioEventsEditor::CheckAndResolveNameConflict(CommandsTree::Node* newParent, CommandsTree::Node* node, CommandsTree::Node* oldParent)
	{
		bool resolved = false;

		auto onReplaceDestination = [&]
			{
				// If node name conflicts with its parent in the destination folder
				if (newParent->Children.at(node->Name) == oldParent)
				{
					// 1. Exctract node from the parent with the same name
					auto handle = oldParent->Children.extract(node->Name);

					// 2. Erase the parent from the destination folder
					newParent->Children.erase(node->Name);

					// 3. Insert moving node to the destination folder and assign new parent
					const auto& it = newParent->Children.insert(std::move(handle)).position;
					it->second->Parent = newParent;
				}
				else
				{
					newParent->Children.erase(node->Name);
					newParent->Children.emplace(node->Name, node);
					oldParent->Children.erase(node->Name);
					node->Parent = newParent;
				}


				resolved = true;
			};

		auto onCancel = [&resolved]
			{
				resolved = true;
			};

		if (auto newName = CheckNameConflict(node->Name, newParent, onReplaceDestination, onCancel))
		{
			auto handle = oldParent->Children.extract(node->Name);
			handle.key() = newName.value();
			const auto& it = newParent->Children.insert(std::move(handle)).position;
			it->second->Name = newName.value();
			it->second->Parent = newParent;

			resolved = true;
		}

		return resolved;
	}

	int name_edited_callback(ImGuiInputTextCallbackData* data)
	{
		if (data->EventChar == ' ')
			data->EventChar = '_';
		return 0;
	}

	void AudioEventsEditor::DrawEventDetails()
	{
		auto* window = ImGui::GetCurrentWindow();

		auto propertyGridSpacing = []
			{
				ImGui::Spacing();
				ImGui::NextColumn();
				ImGui::NextColumn();
			};
		auto singleColumnSeparator = []
			{
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				ImVec2 p = ImGui::GetCursorScreenPos();
				draw_list->AddLine(p/*ImVec2(p.x - 9999, p.y)*/, ImVec2(p.x + 9999, p.y), ImGui::GetColorU32(ImGuiCol_Border));
			};

		auto* selectedNode = sSelection.GetLast();
		auto& trigger = AudioCommandRegistry::GetCommand<TriggerCommand>(*selectedNode->Value);

		//--- Event Name
		//--------------

		ImGui::Spacing();

		auto name = trigger.DebugName;
		char buffer[256];
		strcpy_s(buffer, name.c_str());
		ImGui::Text("Event Name: ");

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.08f,0.08f,0.08f,1.0f });

		// Fixing misalignment of name editing field and "Event Name: " Text
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);

		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackCharFilter;

		if (ImGui::InputText("##eventname", buffer, 256, flags, name_edited_callback))
		{
			OnEntryRenamed(sSelection.GetLast(), buffer);
		}
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);


		if (sNewlyCreatedNode == selectedNode || sRenamingEntry == selectedNode)
		{
			// Make text input active if this is a newly created event
			ImGui::SetKeyboardFocusHere(0);
			sRenamingEntry = nullptr;
			sNewlyCreatedNode = nullptr;
		}
		else
		{
			//f_Renaming = false;
		}

		ImGui::PopStyleColor();

		ImGui::Spacing();
		singleColumnSeparator();
		ImGui::Spacing();

		//--- Event Details
		//-----------------

		// TODO: need ImGui tables API!
		const float width = ImGui::GetColumnWidth();
		const float borderOffset = 7.0f;

		const float bottomAreaHeight = 50.0f;
		const float deleteButtonWidth = ImGui::GetFrameHeight();// 29.0f;
		const ImVec2 deleteButtonSize{ deleteButtonWidth, ImGui::GetFrameHeight() };

		const float moveButtonWidth = ImGui::GetFrameHeight();// 29.0f;
		const ImVec2 moveButtonSize{ moveButtonWidth, ImGui::GetFrameHeight() };

		const float tableWidth = width - borderOffset - moveButtonWidth - deleteButtonWidth - 5.5f; // 5.5 here ensures the jumpy "delete" button has some border
		const float numColumns = 3.0f;
		const float spacing = 2.0f;
		const float itemWidth = tableWidth / numColumns - spacing;
		const float headerWidth = tableWidth / numColumns;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + borderOffset + moveButtonWidth);
		ImGui::Text("Type");
		ImGui::SameLine(borderOffset + moveButtonWidth + spacing + headerWidth, spacing);
		ImGui::Text("Target");
		ImGui::SameLine(borderOffset + moveButtonWidth + spacing + headerWidth * 2.0f, spacing);
		ImGui::Text("Context");

		ImGui::Spacing();

		//--- Actions Scroll view
		//-----------------------

		auto actionsBounds = ImGui::GetContentRegionAvail();
		actionsBounds.x = width - borderOffset;
		actionsBounds.y -= bottomAreaHeight;
		ImGui::BeginChild("Actions", actionsBounds, false);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 1.0f });
		ImGui::Dummy({ width, 0.0f }); // 1px space offset

		const float scrollbarWidth = ImGui::GetCurrentWindow()->ScrollbarSizes.x;

		std::vector<std::pair<int, int>> actionsToReorder;

		for (int i = 0; i < (int)trigger.Actions.GetSize(); ++i)
		{
			std::string idstr = std::to_string(i);
			auto& action = trigger.Actions[i];
			ImGui::PushItemWidth(itemWidth);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1.0f); // adding 1px "border"

			//--- Move Button

			ImGui::PushID(std::string("MoveAction" + std::to_string(i)).c_str());
			UI::ImageButton(sMoveIcon, ImVec2(ImGui::GetFrameHeight() - 12.0f, ImGui::GetFrameHeight() - 12.0f),
				ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 6, ImVec4(0.0f, 0.0f, 0.0f, 0.0f), ImGui::ColorConvertU32ToFloat4(Colors::Theme::text));
			ImGui::PopID();

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				const auto& assetName = AssetManager::GetMetadata(action.Target->Handle).FilePath.filename().string();
				const std::string dragTooltip = std::string(Utils::AudioActionTypeToString(action.Type))
					+ " | " + assetName
					+ " | " + std::string(Utils::AudioActionContextToString(action.Context));

				ImGui::Text(dragTooltip.c_str()); // TODO: draw children names, or number of as well?
				// TODO: draw Action type icon

				ImGui::SetDragDropPayload("audio_eventseditor_action", &i, sizeof(int));
				ImGui::EndDragDropSource();
			}

			//--- Type

			ImGui::SameLine(0.0f, spacing);
			int selectedType = (int)action.Type;
			if (PropertyDropdownNoLabel(("Type" + idstr).c_str(), { "Play", "Stop", "StopAll", "Pause", "PauseAll", "Resume", "ResumeAll" }, 7, &selectedType))
				action.Type = (EActionType)selectedType;

			//--- Target

			bool targetDisabled = false;
			if (action.Type == EActionType::PauseAll || action.Type == EActionType::ResumeAll
				|| action.Type == EActionType::StopAll || action.Type == EActionType::SeekAll)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::muted);
				targetDisabled = true;
			}

			ImGui::SameLine(0.0f, spacing);

			auto isPayload = [](const char* payloadType) {
				if (auto* dragPayload = ImGui::GetDragDropPayload())
					return dragPayload->IsDataType(payloadType);
				return false;
				};

			PropertyAssetReferenceNoLabel(("Target" + idstr).c_str(), action.Target, AssetType::Audio, !targetDisabled && !isPayload("audio_eventseditor_action"));
			ImGui::PopItemWidth();

			if (targetDisabled)
			{
				ImGui::PopItemFlag();	// ImGuiItemFlags_Disabled
				ImGui::PopStyleColor(); // ImGuiCol_Text
			}

			//--- Context

			// Disable context selection for Play and PostTrigger Action types,
			// they should only be used in GameObject context

			bool contextDisabled = false;
			if (action.Type == EActionType::Play || action.Type == EActionType::PostTrigger) // TODO: disable also for SetSwitch
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::muted);
				contextDisabled = true;
			}

			ImGui::SameLine(0.0f, spacing);
			ImGui::PushItemWidth(itemWidth - scrollbarWidth);
			int selectedContext = action.Type == EActionType::Play ? 0 : (int)action.Context;
			if (PropertyDropdownNoLabel(("Context" + idstr).c_str(), { "GameObject", "Global" }, 2, &selectedContext))
				action.Context = (EActionContext)selectedContext;
			ImGui::PopItemWidth();

			if (contextDisabled)
			{
				ImGui::PopItemFlag();	// ImGuiItemFlags_Disabled
				ImGui::PopStyleColor(); // ImGuiCol_Text
			}

			//--- Delete Action Button

			ImGui::SameLine(0.0f, spacing);
			ImGui::PushID(std::string("DeleteAction" + std::to_string(i)).c_str());
			UI::ImageButton(sDeleteIcon, ImVec2(ImGui::GetFrameHeight() - 6.0f, ImGui::GetFrameHeight() - 6.0f),
				ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 3, ImVec4(0.0f, 0.0f, 0.0f, 0.0f), ImGui::ColorConvertU32ToFloat4(Colors::Theme::textDarker));
			/*{
				// TODO: this should cause issues with itereation, somehow it doesn't
				trigger.Actions.ErasePosition(i);
			}*/
			//? For some reason clicks for ImageButton not registrering, so have to do this manual check
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				trigger.Actions.ErasePosition(i);
			}
			ImGui::PopID();

			//--- Drag & Drop Target

			ImVec2 dropTargetSize{ tableWidth + moveButtonWidth + deleteButtonWidth , ImGui::GetFrameHeight() };
			ImGui::SameLine(0.01f, 0.0f);
			ImGui::Dummy(dropTargetSize);
			if (isPayload("audio_eventseditor_action"))
			{
				if (ImGui::BeginDragDropTarget())
				{
					const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("audio_eventseditor_action");

					if (payload)
					{
						int droppedIndex = *(int*)payload->Data;
						actionsToReorder.push_back({ droppedIndex, i });
					}

					ImGui::EndDragDropTarget();
				}
			}
		}

		// Handle Action reorder
		for (auto& [source, destination] : actionsToReorder)
		{
			trigger.Actions.SetNewPosition(source, destination);
		}
		actionsToReorder.clear();

		ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing
		ImGui::EndChild();

		//-- Bottom Area
		//--------------

		//--- Add Action Button

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

		const float butonHeight = 50.0f;
		auto posY = ImGui::GetWindowContentRegionMax().y - butonHeight;
		ImGui::SetCursorPosY(posY);

		ImGui::Spacing();
		ImGui::Spacing();
		if (ImGui::Button("Add Action"))
		{
			ImGui::OpenPopup("AddActionsPopup");
		}

		if (UI::BeginPopup("AddActionsPopup"))
		{
			if (ImGui::MenuItem("Play", nullptr, false))
			{
				TriggerAction triggerAction{ EActionType::Play, Ref<SoundConfig>(), EActionContext::GameObject };
				trigger.Actions.PushBack(triggerAction);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Stop", nullptr, false))
			{
				TriggerAction triggerAction{ EActionType::Stop, Ref<SoundConfig>(), EActionContext::GameObject };
				trigger.Actions.PushBack(triggerAction);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Pause", nullptr, false))
			{
				TriggerAction triggerAction{ EActionType::Pause, Ref<SoundConfig>(), EActionContext::GameObject };
				trigger.Actions.PushBack(triggerAction);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Resume", nullptr, false))
			{
				TriggerAction triggerAction{ EActionType::Resume, Ref<SoundConfig>(), EActionContext::GameObject };
				trigger.Actions.PushBack(triggerAction);
				ImGui::CloseCurrentPopup();
			}
			UI::EndPopup();
		}
	}

	void AudioEventsEditor::DrawFolderDetails()
	{
		ImGui::Spacing();

		//--- Folder Name
		//---------------

		auto* selectedFolder = sSelection.GetLast();
		auto* parent = selectedFolder->Parent;

		auto& name = selectedFolder->Name;
		char buffer[256];
		strcpy_s(buffer, name.c_str());
		ImGui::Text("Folder Name: ");

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.08f,0.08f,0.08f,1.0f });

		// Fixing misalignment of name editing field and "Event Name: " Text
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);

		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll /*| ImGuiInputTextFlags_CallbackCharFilter*/;

		if (ImGui::InputText("##foldername", buffer, 256, flags, nullptr))
		{
			// Handle name conflict
			if (auto it = parent->Children.find(buffer) != parent->Children.end())
			{
				auto conflictNode = parent->Children.find(buffer)->second.Raw();
				if (conflictNode == selectedFolder)
					sRenameConflict.clear();
				else
					sRenameConflict = buffer;
			}
			else
			{
				auto handle = parent->Children.extract(name);
				handle.key() = buffer;
				const auto& pos = parent->Children.insert(std::move(handle)).position;
				pos->second->Name = buffer;
			}
		}
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);


		if (!sRenameConflict.empty())
		{
			auto onReplaceDestination = [&]
				{
					parent->Children.erase(sRenameConflict);

					auto handle = parent->Children.extract(name);
					handle.key() = sRenameConflict;
					const auto& it = parent->Children.insert(std::move(handle)).position;
					it->second->Name = sRenameConflict;

					sRenameConflict.clear();
				};

			auto onCancel = []
				{
					sRenameConflict.clear();
				};

			if (auto newName = CheckNameConflict(sRenameConflict, parent, onReplaceDestination, onCancel))
			{
				auto handle = parent->Children.extract(name);
				handle.key() = newName.value();
				const auto& it = parent->Children.insert(std::move(handle)).position;
				it->second->Name = newName.value();

				sRenameConflict.clear();
			}
		}

		if (sNewlyCreatedNode == selectedFolder || sRenamingEntry == selectedFolder)
		{
			// Make text input active if this is a newly created event
			ImGui::SetKeyboardFocusHere(0);
			sRenamingEntry = nullptr;
			sNewlyCreatedNode = nullptr;
		}
		else
		{
			//f_Renaming = false;
		}

		ImGui::PopStyleColor();
	}

	void AudioEventsEditor::DrawSelectionDetails()
	{
		if (sSelection.IsEmpty())
			return;

		//? workaround for Right-click renaming entry that's not the last one added to the selection
		if (sRenamingEntry != nullptr && sRenamingEntry != sSelection.GetLast())
		{
			if (!sSelection.Contains(sRenamingEntry))
			{
				// If Renaming Entry is not in selection, replace selection with Renaming Entry
				sSelection.Clear();
				sSelection.PushBack(sRenamingEntry);
			}
			else
			{
				sSelection.MoveToBack(sRenamingEntry);
			}
		}

		if (auto* node = sSelection.GetLast())
		{
			if (node->Value) DrawEventDetails();
			else			 DrawFolderDetails();
		}
	}

	void AudioEventsEditor::SerializeTree(YAML::Emitter& out)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "TriggersTreeView" << YAML::BeginSeq;

		auto serializeNode = [&out](CommandsTree::Node* treeNode, auto serializeNode) ->void
			{
				const bool isFolder = !treeNode->Value.has_value();
				if (isFolder)
				{
					// Serialize Folder

					out << YAML::BeginMap; // Folder entry
					out << YAML::Key << "Name" << YAML::Value << treeNode->Name;
					out << YAML::Key << "Children" << YAML::BeginSeq;

					for (auto& [name, childNode] : treeNode->Children)
						serializeNode(childNode.Raw(), serializeNode);

					out << YAML::EndSeq; // Children
					out << YAML::EndMap; // Folder entry
				}
				else
				{
					// Serialize Triger

					out << YAML::BeginMap; // Trigger entry
					NR_SERIALIZE_PROPERTY(Name, treeNode->Name, out);
					NR_SERIALIZE_PROPERTY(CommandID, (uint32_t)(*treeNode->Value), out);
					out << YAML::EndMap;

				}
			};

		serializeNode(&sTree.RootNode, serializeNode);

		out << YAML::EndSeq;// TriggersTreeView
		out << YAML::EndMap;
	}

	void AudioEventsEditor::DeserializeTree(std::vector<YAML::Node>& data)
	{
		// TODO: should probably get these from shared editor resources of some sort

		if (!sFolderIcon)
			sFolderIcon = Texture2D::Create("Resources/Editor/folder.png");
		if (!sMoveIcon)
			sMoveIcon = Texture2D::Create("Resources/Editor/move.png");
		if (!sDeleteIcon)
			sDeleteIcon = Texture2D::Create("Resources/Editor/close.png");


		auto it = std::find_if(data.begin(), data.end(), [](const YAML::Node& doc) { return doc["TriggersTreeView"].IsDefined(); });

		if (it == data.end())
		{
			NR_CORE_ERROR("Invalid, or missing TriggersTreeView cache.");
			return;
		}

		YAML::Node treeView = (*it)["TriggersTreeView"];

		auto deserializeNode = [](const YAML::Node& node, CommandsTree::Node* parentFolder, auto deserializeNode) ->void
			{
				const bool isFolder = node["Children"].IsDefined();
				if (isFolder)
				{
					auto folder = Ref<CommandsTree::Node>::Create();

					folder->Tree = &sTree;
					NR_DESERIALIZE_PROPERTY(Name, folder->Name, node, std::string(""));
					folder->Parent = parentFolder;

					for (auto& childNode : node["Children"])
					{
						deserializeNode(childNode, folder.Raw(), deserializeNode);
					}

					parentFolder->Children.try_emplace(folder->Name, folder);
				}
				else
				{
					auto trigger = Ref<CommandsTree::Node>::Create();

					trigger->Tree = &sTree;
					NR_DESERIALIZE_PROPERTY(Name, trigger->Name, node, std::string(""));
					trigger->Value = node["CommandID"] ? CommandID::FromUnsignedInt(node["CommandID"].as<uint32_t>()) : CommandID::InvalidID();
					trigger->Parent = parentFolder;

					parentFolder->Children.try_emplace(trigger->Name, trigger);
				}
			};

		auto rootNode = treeView[0];

		for (auto node : rootNode["Children"])
			deserializeNode(node, &sTree.RootNode, deserializeNode);
	}

} // namespace NotRed