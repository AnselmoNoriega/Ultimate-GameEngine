#include <nrpch.h>
#include "NodeGraphEditor.h"

#include "NotRed/Asset/AssetManager.h"
#include "NodeGraphAsset.h"
#include "NodeEditorModel.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "imgui-node-editor/imgui_node_editor_internal.h"

#include "imgui-node-editor/builders.h"
#include "imgui-node-editor/widgets.h"

#include "NotRed/ImGui/ImGuiUtil.h"
#include "NotRed/ImGui/Colors.h"
#include "NotRed/Core/Input.h"

#include "choc/text/choc_StringUtilities.h"
#include "choc/text/choc_FloatToString.h"

namespace ed = ax::NodeEditor;
namespace utils = ax::NodeEditor::Utilities;

static ax::NodeEditor::Detail::EditorContext* GetEditorContext()
{
	return (ax::NodeEditor::Detail::EditorContext*)(ed::GetCurrentEditor());
}

namespace NR
{
	void NodeGraphEditorBase::InitializeEditorStyle(ax::NodeEditor::Style* style)
	{
		// Style
		auto& editorStyle = ed::GetStyle();
		editorStyle.NodePadding = { 0.0f, 4.0f, 0.0f, 0.0f }; // This mostly affects Comment nodes
		editorStyle.NodeRounding = 3.0f;
		editorStyle.NodeBorderWidth = 1.0f;
		editorStyle.HoveredNodeBorderWidth = 2.0f;
		editorStyle.SelectedNodeBorderWidth = 3.0f;
		editorStyle.PinRounding = 2.0f;
		editorStyle.PinBorderWidth = 0.0f;
		editorStyle.LinkStrength = 80.0f;
		editorStyle.ScrollDuration = 0.35f;
		editorStyle.FlowMarkerDistance = 30.0f;
		editorStyle.FlowDuration = 2.0f;
		editorStyle.GroupRounding = 0.0f;
		editorStyle.GroupBorderWidth = 0.0f;
		editorStyle.GridSnap = 8.0f;

		// Extra (for now just using defaults)
		editorStyle.PivotAlignment = ImVec2(0.5f, 0.5f);    // This one is changed in Draw
		editorStyle.PivotSize = ImVec2(0.0f, 0.0f);         // This one is changed in Draw
		editorStyle.PivotScale = ImVec2(1, 1);              // This one is not used
		editorStyle.PinCorners = ImDrawFlags_RoundCornersAll;
		editorStyle.PinRadius = 0.0f;       // This one is changed in Draw
		editorStyle.PinArrowSize = 0.0f;    // This one is changed in Draw
		editorStyle.PinArrowWidth = 0.0f;   // This one is changed in Draw

		// Colors
		editorStyle.Colors[ed::StyleColor_Bg] = ImColor(23, 24, 28, 200);
		editorStyle.Colors[ed::StyleColor_Grid] = ImColor(60, 60, 60, 40);
		editorStyle.Colors[ed::StyleColor_NodeBg] = ImColor(31, 33, 38, 255);
		editorStyle.Colors[ed::StyleColor_NodeBorder] = ImColor(51, 54, 62, 140);
		editorStyle.Colors[ed::StyleColor_HovNodeBorder] = ImColor(60, 180, 255, 150);
		editorStyle.Colors[ed::StyleColor_SelNodeBorder] = ImColor(255, 225, 135, 255);
		editorStyle.Colors[ed::StyleColor_NodeSelRect] = ImColor(5, 130, 255, 64);
		editorStyle.Colors[ed::StyleColor_NodeSelRectBorder] = ImColor(5, 130, 255, 128);
		editorStyle.Colors[ed::StyleColor_HovLinkBorder] = ImColor(60, 180, 255, 255);
		editorStyle.Colors[ed::StyleColor_SelLinkBorder] = ImColor(255, 225, 135, 255);
		editorStyle.Colors[ed::StyleColor_LinkSelRect] = ImColor(5, 130, 255, 64);
		editorStyle.Colors[ed::StyleColor_LinkSelRectBorder] = ImColor(5, 130, 255, 128);
		editorStyle.Colors[ed::StyleColor_PinRect] = ImColor(60, 180, 255, 0);
		editorStyle.Colors[ed::StyleColor_PinRectBorder] = ImColor(60, 180, 255, 0);
		editorStyle.Colors[ed::StyleColor_Flow] = ImColor(255, 128, 64, 255);
		editorStyle.Colors[ed::StyleColor_FlowMarker] = ImColor(255, 128, 64, 255);
		editorStyle.Colors[ed::StyleColor_GroupBg] = ImColor(255, 255, 255, 30);
		editorStyle.Colors[ed::StyleColor_GroupBorder] = ImColor(0, 0, 0, 0);
	}

	struct NodeGraphEditorBase::ContextState
	{
		bool CreateNewNode = false;
		Pin* NewNodeLinkPin = nullptr;
		Pin* NewLinkPin = nullptr;
		bool DraggingInputField = false;

		ed::NodeId ContextNodeId = 0;
		ed::LinkId ContextLinkId = 0;
		ed::PinId ContextPinId = 0;
	};

	NodeGraphEditorBase::NodeGraphEditorBase()
		: AssetEditor("Node Editor")
	{
		mHeaderBackground = Texture2D::Create("Resources/Editor/NodeGraphEditor/translucency_2_hor.png");
		mShadow = Texture2D::Create("Resources/Editor/NodeGraphEditor/shadow line_top.png");
		mSaveIcon = Texture2D::Create("Resources/Editor/ic_save_white_24dp.png");
		mCompileIcon = Texture2D::Create("Resources/Editor/NodeGraphEditor/compile_38px.png");

		mSearchIcon = Texture2D::Create("Resources/Editor/search.png");

		mIconPin_V_C = Texture2D::Create("Resources/Editor/NodeGraphEditor/pin_value_con.png");
		mIconPin_V_D = Texture2D::Create("Resources/Editor/NodeGraphEditor/pin_value_disc.png");
		mIconPin_F_C = Texture2D::Create("Resources/Editor/NodeGraphEditor/pin_func_con.png");
		mIconPin_F_D = Texture2D::Create("Resources/Editor/NodeGraphEditor/pin_func_disc.png");
		mIconPin_A_C = Texture2D::Create("Resources/Editor/NodeGraphEditor/pin_audio_con.png");
		mIconPin_A_D = Texture2D::Create("Resources/Editor/NodeGraphEditor/pin_audio_disc.png");

		mState = std::make_unique<ContextState>();
	}

	void NodeGraphEditorBase::Close()
	{
		GetModel()->SaveAll();

		if (mEditor)
		{
			ed::DestroyEditor(mEditor);
			mEditor = nullptr;
		}

		mGraphAsset = nullptr;
	}

	void NodeGraphEditorBase::SetAsset(const Ref<Asset>& asset)
	{
		mGraphAsset = asset;
	}

	bool NodeGraphEditorBase::InitializeEditor()
	{
		if (!mGraphAsset)
			return false;

		// TODO: need to make sure to delete the .graphstate file when deleting graph asset
		auto& assetMetadata = AssetManager::GetMetadata(mGraphAsset->Handle);
		std::filesystem::path settingsFilePath = AssetManager::GetFileSystemPath(assetMetadata);
		settingsFilePath.replace_extension(".graphstate");
		mGraphStatePath = settingsFilePath.string();

		ed::Config config;
		config.SettingsFile = mGraphStatePath.c_str();
		config.UserPointer = this;

		// Restore node location
		config.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
			{
				auto self = static_cast<NodeGraphEditorBase*>(userPointer);

				if (!self->GetModel())
				{
					return 0;
				}

				auto node = self->GetModel()->FindNode(UUID(nodeId.Get()));
				if (!node)
				{
					return 0;
				}


				if (data != nullptr)
				{
					memcpy(data, node->State.data(), node->State.size());
				}
				return node->State.size();
			};

		// Store node location
		config.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
			{
				auto self = static_cast<NodeGraphEditorBase*>(userPointer);

				if (!self->GetModel())
					return false;

				auto node = self->GetModel()->FindNode(UUID(nodeId.Get()));
				if (!node)
					return false;

				node->State.assign(data, size);

				////self->TouchNode(nodeId);

				return true;
			};

		mEditor = ed::CreateEditor(&config);
		ed::SetCurrentEditor(mEditor);

		InitializeEditorStyle(&ed::GetStyle());

		// Centre viewport around content of the graph
		ed::NavigateToContent();

		return true;
	}

	void NodeGraphEditorBase::DrawNodeContextMenu(Node* node)
	{
		ImGui::TextUnformatted("Node Context Menu");
		ImGui::Separator();
		if (node)
		{
			ImGui::Text("ID: %s", std::to_string(node->ID).c_str());
			ImGui::Text("Inputs: %d", (int)node->Inputs.size());
			ImGui::Text("Outputs: %d", (int)node->Outputs.size());
		}
		else
			ImGui::Text("Unknown node: %p", mState->ContextNodeId.AsPointer());
		ImGui::Separator();
		if (ImGui::MenuItem("Delete"))
			ed::DeleteNode(mState->ContextNodeId);
	}

	void NodeGraphEditorBase::DrawPinContextMenu(Pin* pin)
	{
		ImGui::TextUnformatted("Pin Context Menu");
		ImGui::Separator();
		if (pin)
		{
			ImGui::Text("ID: %s", std::to_string(pin->ID).c_str());
			if (pin->Node)
			{
				ImGui::Text("Node: %s", std::to_string(pin->Node->ID).c_str());
			}
			else
				ImGui::Text("Node: %s", "<none>");
		}
		else
			ImGui::Text("Unknown pin: %s", mState->ContextPinId.AsPointer());
	}

	void NodeGraphEditorBase::DrawLinkContextMenu(Link* link)
	{
		ImGui::TextUnformatted("Link Context Menu");
		ImGui::Separator();
		if (link)
		{
			ImGui::Text("ID: %s", std::to_string(link->ID).c_str());
			ImGui::Text("From: %s", std::to_string(link->StartPinID).c_str());
			ImGui::Text("To: %s", std::to_string(link->EndPinID).c_str());
		}
		else
			ImGui::Text("Unknown link: %p", mState->ContextLinkId.AsPointer());
		ImGui::Separator();
		if (ImGui::MenuItem("Delete"))
			ed::DeleteLink(mState->ContextLinkId);
	}

	static AssetHandle sAssetComboSelectedHandle = 0;
	static Pin* sAssetComboPin = nullptr;
	static ImVec2 sAssetComboStart;
	static ImVec2 sAssetComboSize = ImVec2(0.0f, 280.0f);
	static bool sShouldAssetComboOpen = false;
	static const char* sAssetComboPopupID = "AssetPinCombo";

	void NodeGraphEditorBase::Render()
	{
		if (!GetModel())
			return;

		if (!mEditor || !mInitialized)
		{
			mInitialized = InitializeEditor();
		}

		// Main Docking Space
		//-------------------
		ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_AutoHideTabBar;
		mMainDockID = ImGui::DockSpace(ImGui::GetID("NodeEditor"), ImVec2(0, 0), dockFlags);

		UI::ScopedStyle stylePadding(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));

		// TODO: this might have to be unique ID of the asset to prevend docking with other graph windows
		mWindowClass.ClassId = ImGui::GetID("NodeEditor");
		mWindowClass.DockingAllowUnclassed = false;

		ImGui::SetNextWindowDockID(mMainDockID, ImGuiCond_Always);
		ImGui::SetNextWindowClass(GetWindowClass());

		ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoCollapse);

		// This ensures that we don't start selection rectangle when dragging Canvas window titlebar
		bool draggingnCanvasTitlebar = false;
		if (ImGui::IsItemActive())
			draggingnCanvasTitlebar = true;


		ed::SetCurrentEditor(mEditor);

		ed::NodeId& contextNodeId = mState->ContextNodeId;
		ed::LinkId& contextLinkId = mState->ContextLinkId;
		ed::PinId& contextPinId = mState->ContextPinId;
		auto& createNewNode = mState->CreateNewNode;
		auto& newNodeLinkPin = mState->NewNodeLinkPin;
		auto& newLinkPin = mState->NewLinkPin;

		// Set to 'true' to prevent editor from creating selection rectangle
		// when draggin node input field
		auto& draggingInputField = mState->DraggingInputField;

		static bool sNewNodePopupOpening = false;

		ed::Begin("Node editor");
		{
			auto cursorTopLeft = ImGui::GetCursorScreenPos();

			DrawNodes();

			// Links
			for (auto& link : GetModel()->GetLinks())
				ed::Link(ed::LinkId(link.ID), ed::PinId(link.StartPinID), ed::PinId(link.EndPinID), link.Color, 2.0f);

			mAcceptingLinkPins = { nullptr, nullptr };

			// Draw dragging Link
			if (!createNewNode)
			{
				if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
				{
					auto showLabel = [](const char* label, ImColor color)
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
							auto size = ImGui::CalcTextSize(label);

							auto padding = ImGui::GetStyle().FramePadding;
							auto spacing = ImGui::GetStyle().ItemSpacing;

							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

							auto rectMin = ImGui::GetCursorScreenPos() - padding;
							auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

							auto drawList = ImGui::GetWindowDrawList();
							drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
							ImGui::TextUnformatted(label);
						};

					ed::PinId startPinId = 0, endPinId = 0;
					if (ed::QueryNewLink(&startPinId, &endPinId))
					{
						auto startPin = GetModel()->FindPin(startPinId.Get());
						auto endPin = GetModel()->FindPin(endPinId.Get());

						newLinkPin = startPin ? startPin : endPin;

						if (startPin->Kind == PinKind::Input)
						{
							std::swap(startPin, endPin);
							std::swap(startPinId, endPinId);
						}

						if (startPin && endPin)
						{
							NodeEditorModel::LinkQueryResult canConnect = GetModel()->CanCreateLink(startPin, endPin);
							if (canConnect)
							{
								mAcceptingLinkPins.first = startPin;
								mAcceptingLinkPins.second = endPin;
								showLabel("+ Create Link", ImColor(32, 45, 32, 180));
								if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
								{
									GetModel()->CreateLink(startPin, endPin);
								}
							}
							else
							{
								switch (canConnect.Reason)
								{
								case NodeEditorModel::LinkQueryResult::IncompatibleStorageKind:
								case NodeEditorModel::LinkQueryResult::IncompatiblePinKind:
								{
									showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
									ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
									break;
								}
								case NodeEditorModel::LinkQueryResult::IncompatibleType:
								{
									showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
									ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
									break;
								}
								case NodeEditorModel::LinkQueryResult::SamePin:
								case NodeEditorModel::LinkQueryResult::SameNode:
								{
									showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
									ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
									break;
								}
								case NodeEditorModel::LinkQueryResult::InvalidStartPin:
								case NodeEditorModel::LinkQueryResult::InvalidEndPin:
								default:
									break;
								}
							}
						}
					}

					ed::PinId pinId = 0;
					if (ed::QueryNewNode(&pinId))
					{
						newLinkPin = GetModel()->FindPin(pinId.Get());
						if (newLinkPin)
							showLabel("+ Create Node", ImColor(32, 45, 32, 180));

						/*if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
						{
							createNewNode = true;
							newNodeLinkPin = GetModel()->FindPin(pinId.Get());
							newLinkPin = nullptr;

							ed::Suspend();
							ImGui::OpenPopup("Create New Node");
							ed::Resume();
						}*/
						if (ed::AcceptNewItem())
						{
							createNewNode = true;
							newNodeLinkPin = GetModel()->FindPin(pinId.Get());
							newLinkPin = nullptr;

							ed::Suspend();
							ImGui::OpenPopup("Create New Node");
							sNewNodePopupOpening = true;
							ed::Resume();
						}
					}
				}
				else
					newLinkPin = nullptr;

				ed::EndCreate();

				if (ed::BeginDelete())
				{
					ed::LinkId linkId = 0;
					while (ed::QueryDeletedLink(&linkId))
					{
						if (ed::AcceptDeletedItem())
							GetModel()->RemoveLink(linkId.Get());
					}

					ed::NodeId nodeId = 0;
					while (ed::QueryDeletedNode(&nodeId))
					{
						if (ed::AcceptDeletedItem())
							GetModel()->RemoveNode(nodeId.Get());
					}
				}
				ed::EndDelete();
			}

			ImGui::SetCursorScreenPos(cursorTopLeft);

			ed::Suspend();

			if (ed::ShowNodeContextMenu(&contextNodeId))
			{
				ImGui::OpenPopup("Node Context Menu");
			}
			else if (ed::ShowPinContextMenu(&contextPinId))
			{
				ImGui::OpenPopup("Pin Context Menu");
			}
			else if (ed::ShowLinkContextMenu(&contextLinkId))
			{
				ImGui::OpenPopup("Link Context Menu");
			}
			else if (ed::ShowBackgroundContextMenu())
			{
				ImGui::OpenPopup("Create New Node");
				sNewNodePopupOpening = true;
				newNodeLinkPin = nullptr;
			}
			ed::Resume();
			ed::Suspend();

			UI::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
			UI::ScopedColorStack popupColors(ImGuiCol_HeaderHovered, IM_COL32(45, 46, 51, 255),
				ImGuiCol_Separator, IM_COL32(90, 90, 90, 255),
				ImGuiCol_Text, IM_COL32(210, 210, 210, 255));

			if (UI::BeginPopup("Node Context Menu"))
			{
				auto node = GetModel()->FindNode(contextNodeId.Get());
				DrawNodeContextMenu(node);

				UI::EndPopup();
			}

			if (UI::BeginPopup("Pin Context Menu"))
			{
				auto pin = GetModel()->FindPin(contextPinId.Get());
				DrawPinContextMenu(pin);

				UI::EndPopup();
			}

			if (UI::BeginPopup("Link Context Menu"))
			{
				auto link = GetModel()->FindLink(contextLinkId.Get());
				DrawLinkContextMenu(link);

				UI::EndPopup();
			}

			if (UI::BeginPopup("Create New Node"))
			{
				auto newNodePostion = ed::ScreenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup());

				Node* node = nullptr;

				const auto& nodeRegistry = GetModel()->GetNodeTypes();

				static bool searching = false;

				static char searchBuffer[256];
				{
					UI::ScopedStyle rounding(ImGuiStyleVar_FrameRounding, 3.0f);
					ImGui::InputText("##regsearch", searchBuffer, 256);
					UI::DrawItemActivityOutline(3.0f, true, Colors::Theme::niceBlue);

					// Search icon
					if (!searching)
					{
						ImGui::SetItemAllowOverlap();
						ImGui::SameLine(13.0f);
						UI::ShiftCursorY(3.0f);
						const ImVec2 searchIconSize(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight());
						UI::Image(mSearchIcon, searchIconSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
					}

					if (!ImGui::IsItemFocused() && sNewNodePopupOpening)
					{
						ImGui::SetKeyboardFocusHere(0);
					}
				}

				searching = searchBuffer[0] != 0;

				for (const auto& [categoryName, category] : nodeRegistry)
				{
					UI::ScopedColorStack headerColors(ImGuiCol_Header, IM_COL32(255, 255, 255, 0),
						ImGuiCol_HeaderActive, IM_COL32(45, 46, 51, 255),
						ImGuiCol_HeaderHovered, IM_COL32(45, 46, 51, 255));

					// Can use this instead of the collapsing header
					//UI::PopupMenuHeader(categoryName, true, false);

					if (searching)
						ImGui::SetNextItemOpen(searchBuffer[0] != 0);

					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(170, 170, 170, 255));
					if (ImGui::CollapsingHeader(categoryName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::PopStyleColor(); // header Text
						ImGui::Separator();

						ImGui::Indent();
						//-------------------------------------------------
						if (searching)
						{
							std::string searchString = Utils::String::ToLowerCopy(searchBuffer);

							for (const auto& [nodeName, spawnFunction] : category)
							{
								const std::string nameNoUnderscores = choc::text::replace(nodeName, "_", " ");

								const std::string nameSanitized = Utils::String::ToLowerCopy(choc::text::replace(nameNoUnderscores, " ", ""));
								const std::string catSanitized = Utils::String::ToLowerCopy(choc::text::replace(categoryName, " ", ""));
								searchString = choc::text::replace(searchString, " ", "");

								if (choc::text::contains(catSanitized, searchString)
									|| choc::text::contains(Utils::String::ToLowerCopy(nameSanitized), searchString))
								{
									if (ImGui::MenuItem(nameNoUnderscores.c_str()))
										node = GetModel()->CreateNode(categoryName, nodeName);

								}

							}
						}
						else
						{
							for (const auto& [nodeName, spawnFunction] : category)
							{
								if (ImGui::MenuItem(choc::text::replace(nodeName, "_", " ").c_str()))
								{
									node = GetModel()->CreateNode(categoryName, nodeName);
								}
							}
						}

						if (nodeRegistry.find(categoryName) != nodeRegistry.end())
							ImGui::Spacing();
						ImGui::Unindent();
					}
					else
					{
						ImGui::PopStyleColor(); // header Text
					}
				}

				if (node)
				{
					createNewNode = false;

					ed::SetNodePosition(ed::NodeId(node->ID), newNodePostion);

					if (auto startPin = newNodeLinkPin)
					{
						auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;

						for (auto& pin : pins)
						{
							if (GetModel()->CanCreateLink(startPin, &pin))
							{
								auto endPin = &pin;
								if (startPin->Kind == PinKind::Input)
									std::swap(startPin, endPin);

								GetModel()->CreateLink(startPin, endPin);

								break;
							}
						}
					}
				}
				auto* popupWindow = ImGui::GetCurrentWindow();
				auto shadowRect = ImRect(popupWindow->Pos, popupWindow->Pos + popupWindow->Size);

				sNewNodePopupOpening = false;

				UI::EndPopup();
				UI::DrawShadow(mShadow, 14.0f, shadowRect, 1.3f);

			}
			else
				createNewNode = false;

			ed::Resume();
		}

		if (sAssetComboPin != nullptr)
		{
			ed::Suspend();

			if (sShouldAssetComboOpen && !ImGui::IsPopupOpen(sAssetComboPopupID))
			{
				ImGuiIO& io = ImGui::GetIO();
				io.MousePosPrev = io.MousePos;
				io.MousePos = sAssetComboStart;
				ImGui::OpenPopup(sAssetComboPopupID);
				io.MousePos = io.MousePosPrev;

				sShouldAssetComboOpen = false;
			}

			Ref<AudioFile> asset;
			if (UI::PropertyAssetDropdown(sAssetComboPopupID, asset, sAssetComboSize, &sAssetComboSelectedHandle))
			{
				NR_CORE_INFO("Asset selected");
				choc::value::Value customValueType = choc::value::createObject("AssetHandle",
					"TypeName", "AssetHandle",
					"Value", std::to_string((uint64_t)sAssetComboSelectedHandle));
				sAssetComboPin->Value = customValueType;
				sAssetComboPin = nullptr;
				sAssetComboSelectedHandle = 0;
				ImGui::CloseCurrentPopup();
			}

			ed::Resume();
		}

		ed::End();
		auto editorMin = ImGui::GetItemRectMin();
		auto editorMax = ImGui::GetItemRectMax();

		// Draw current zoom level
		{
			UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);
			const ImVec2 zoomLabelPos({ editorMin.x + 20.0f, editorMax.y - 40.0f });
			const std::string zoomLabelText = "Zoom " + choc::text::floatToString(ed::GetCurrentZoom(), 1, true);

			ImGui::GetWindowDrawList()->AddText(zoomLabelPos, IM_COL32(255, 255, 255, 60), zoomLabelText.c_str());
		}

		UI::DrawShadowInner(mShadow, 50.0f, editorMin, editorMax, 0.3f, (editorMax.y - editorMin.y) / 4.0f);

		// A big of a hack to prevent editor from dragging and selecting while dragging an input field
		// It still draws a dot where the Selection Action should be, but it's not too bad
		if (draggingInputField || draggingnCanvasTitlebar)
		{
			auto* editor = GetEditorContext();
			if (auto* action = editor->GetCurrentAction())
			{
				if (auto* dragAction = action->AsDrag())
					dragAction->m_Clear = true;
				if (auto* selectAction = action->AsSelect())
					selectAction->m_IsActive = false;
			}
		}

		ImGui::End(); // Canvas

		// Draw windows of subclasses
		OnRender();
	}

	void NodeGraphEditorBase::EnsureWindowIsDocked(ImGuiWindow* childWindow)
	{
		if (childWindow->DockNode && childWindow->DockNode->ParentNode)
			mDockIDs[childWindow->ID] = childWindow->DockNode->ParentNode->ID;

		if (!childWindow->DockIsActive && !ImGui::IsAnyMouseDown())
		{
			if (!mDockIDs[childWindow->ID])
				mDockIDs[childWindow->ID] = mMainDockID;

			ImGui::SetWindowDock(childWindow, mDockIDs[childWindow->ID], ImGuiCond_Always);
		}
	}

	//static ed::PinId sCurrentComboPinID;

	void NodeGraphEditorBase::DrawNodes()
	{
		utils::BlueprintNodeBuilder builder(UI::GetTextureID(mHeaderBackground), mHeaderBackground->GetWidth(), mHeaderBackground->GetHeight());

		auto drawItemActivityOutline = [](float rounding = 0.0f)
			{
				auto* drawList = ImGui::GetWindowDrawList();
				if (ImGui::IsItemHovered() && !ImGui::IsItemActive())
				{
					drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
						ImColor(60, 60, 60), rounding, 0, 1.5f);
				}
				if (ImGui::IsItemActive())
				{
					drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
						ImColor(50, 50, 50), rounding, 0, 1.0f);
				}
			};

		//=============================================================
		/// Basic nodes
		for (auto& node : GetModel()->GetNodes())
		{
			if (node.Type != NodeType::Blueprint && node.Type != NodeType::Simple)
				continue;

			const auto isSimple = node.Type == NodeType::Simple;

			bool hasOutputDelegates = false;
			for (auto& output : node.Outputs)
				if (output.Type == PinType::Delegate)
					hasOutputDelegates = true;

			builder.Begin(ed::NodeId(node.ID));
			if (!isSimple)
			{

				builder.Header(node.Color);
				{
					ImGui::Spring(0);
					UI::ScopedColor headerTextColor(ImGuiCol_Text, IM_COL32(210, 210, 210, 255));

					ImGui::TextUnformatted(node.Name.c_str());

					if (mShowNodeIDs)
						ImGui::TextUnformatted(("(" + std::to_string(node.ID) + ")").c_str());
				}
				ImGui::Spring(1);

				const float nodeHeaderHeight = 18.0f;
				ImGui::Dummy(ImVec2(0, nodeHeaderHeight));
				if (hasOutputDelegates)
				{
					ImGui::BeginVertical("delegates", ImVec2(0, nodeHeaderHeight));
					ImGui::Spring(1, 0);
					for (auto& output : node.Outputs)
					{
						if (output.Type != PinType::Delegate)
							continue;

						auto alpha = ImGui::GetStyle().Alpha;
						if (mState->NewLinkPin && !GetModel()->CanCreateLink(mState->NewLinkPin, &output) && &output != mState->NewLinkPin)
						{
							alpha = alpha * (48.0f / 255.0f);
						}

						ed::BeginPin(ed::PinId(output.ID), ed::PinKind::Output);
						ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
						ed::PinPivotSize(ImVec2(0, 0));
						ImGui::BeginHorizontal(output.ID);
						UI::ScopedStyle alphaStyle(ImGuiStyleVar_Alpha, alpha);
						if (!output.Name.empty())
						{
							ImGui::TextUnformatted(output.Name.c_str());
							ImGui::Spring(0);
						}
						DrawPinIcon(output, GetModel()->IsPinLinked(output.ID), (int)(alpha * 255));
						ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
						ImGui::EndHorizontal();
						ed::EndPin();
					}
					ImGui::Spring(1, 0);
					ImGui::EndVertical();
					ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
				}
				else
					ImGui::Spring(0);
				builder.EndHeader();

			}

			for (auto& input : node.Inputs)
			{
				auto alpha = ImGui::GetStyle().Alpha;
				if (mState->NewLinkPin && !GetModel()->CanCreateLink(mState->NewLinkPin, &input) && &input != mState->NewLinkPin)
				{
					alpha = alpha * (48.0f / 255.0f);
				}

				builder.Input(ed::PinId(input.ID));
				UI::ScopedStyle alphaStyle(ImGuiStyleVar_Alpha, alpha);
				DrawPinIcon(input, GetModel()->IsPinLinked(input.ID), (int)(alpha * 255));
				ImGui::Spring(0);

				// Draw input pin name
				if (!isSimple && !input.Name.empty())
				{
					ImGui::TextUnformatted(input.Name.c_str());
					ImGui::Spring(0);
				}

				// Need to disable editor shortcuts and drag-selecting while editing or dragging input field
				auto deactivateInputIfDragging = [&](bool& wasActive)
					{
						if (ImGui::IsItemActive() && !wasActive)
						{
							mState->DraggingInputField = true;
							ed::EnableShortcuts(false);
							wasActive = true;
						}
						else if (!ImGui::IsItemActive() && wasActive)
						{
							mState->DraggingInputField = false;
							ed::EnableShortcuts(true);
							wasActive = false;
						}
					};

				// Draw input fields for non-array pins
				if (input.Type == PinType::Bool && input.Storage != StorageKind::Array)
				{
					// Draw input field if the pin is not linked
					if (!GetModel()->IsPinLinked(input.ID))
					{
						static bool wasActive = false;

						UI::ScopedStyle frameRounding(ImGuiStyleVar_FrameRounding, 2.5f);
						bool pinValue = !input.Value.isVoid() ? input.Value.get<bool>() : false;
						if (ImGui::Checkbox("##bool", &pinValue))
							input.Value = choc::value::Value(pinValue);

						drawItemActivityOutline(2.5f);
						deactivateInputIfDragging(wasActive);

						ImGui::Spring(0);
					}
				}
				if ((input.Storage != StorageKind::Array) && (input.Type == PinType::Float
					|| input.Type == PinType::Int
					|| input.Type == PinType::String))
				{
					// Draw input field if the pin is not linked
					if (!GetModel()->IsPinLinked(input.ID))
					{
						UI::ScopedStyle frameRounding(ImGuiStyleVar_FrameRounding, 2.5f);

						if (input.Type == PinType::Float)
						{
							static bool wasActive = false;

							ImGui::PushItemWidth(40.0f);
							float pinValue = !input.Value.isVoid() ? input.Value.get<float>() : 0.0f;
							if (ImGui::DragFloat("##floatIn", &pinValue, 0.01f, 0.0f, 0.0f, "%.2f"))
								input.Value = choc::value::Value(pinValue);

							deactivateInputIfDragging(wasActive);
						}
						else if (input.Type == PinType::Int)
						{
							static bool wasActive = false;

							ImGui::PushItemWidth(30.0f);
							int pinValue = !input.Value.isVoid() ? input.Value.get<int>() : 0;
							if (ImGui::DragInt("##int", &pinValue, 0.1f))
								input.Value = choc::value::Value(pinValue);

							deactivateInputIfDragging(wasActive);
						}
						else if (input.Type == PinType::String)
						{
							static bool wasActive = false;

							static char buffer[128]{};
							if (!input.Value.isVoid())
							{
								memset(buffer, 0, sizeof(buffer));
								memcpy(buffer, input.Value.get<std::string>().data(), std::min(input.Value.get<std::string>().size(), sizeof(buffer)));
							}
							const auto inputTextFlags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
							ImGui::PushItemWidth(100.0f);
							ImGui::InputText("##edit", buffer, 127, inputTextFlags);

							if (ImGui::IsItemDeactivatedAfterEdit())
							{
								input.Value = choc::value::Value(std::string(buffer));
							}

							deactivateInputIfDragging(wasActive);
						}

						drawItemActivityOutline(2.5f);
						ImGui::PopItemWidth();

						ImGui::Spring(0);
					}
				}

				if (input.Type == PinType::Object && input.Storage != StorageKind::Array)
				{
					if (!GetModel()->IsPinLinked(input.ID))
					{
						// TODO: the dropdown combobox doesn't work
						// But this shoud demonstrate how to set pin values with custom types like object reference, vector, etc.

						AssetHandle selected = 0;
						Ref<AudioFile> asset;

						static std::string sComboPreview;

						if (input.Value.isObjectWithClassName("AssetHandle"))
						{
							auto handleStr = input.Value["Value"].get<std::string>();
							selected = std::stoull(handleStr);

							if (AssetManager::IsAssetHandleValid(selected))
							{
								asset = AssetManager::GetAsset<AudioFile>(selected);
							}

							if (asset)
							{
								if (!asset->IsFlagSet(AssetFlag::Missing))
									sComboPreview = AssetManager::GetMetadata(asset->Handle).FilePath.stem().string();
								else
									sComboPreview = "Missing";
							}
							else
							{
								sComboPreview = "Null";
							}
						}

						if (UI::DrawComboPreview(sComboPreview.c_str()))
						{
							sAssetComboSelectedHandle = selected;
							sAssetComboPin = &input;
							ed::NodeId hoveredNode = ed::GetHoveredNode();
							ImVec2 canvasPosition = ed::GetNodePosition(hoveredNode);
							canvasPosition.y += ed::GetNodeSize(hoveredNode).y;
							sAssetComboStart = ed::CanvasToScreen(canvasPosition);
							// Uncomment this to have the popup scale with the canvas (I don't recommend it since the font size doesn't scale)
							//sComboNodeSize = ed::GetNodeSize(hoveredNode) / ed::GetCurrentZoom();
							sShouldAssetComboOpen = true;
						}
					}
				}

				builder.EndInput();
			}

			// Setting colour of the Message Node input field to dark
			UI::ScopedColor frameBgColor(ImGuiCol_FrameBg, ImVec4{ 0.08f, 0.08f, 0.08f, 1.0f });

			if (isSimple)
			{
				builder.Middle();

				ImGui::Spring(1, 0);
				UI::ScopedColor textColor(ImGuiCol_Text, IM_COL32(210, 210, 210, 255));

				// TODO: handle simple nodes display icon properly
				if (const char* displayIcon = GetIconForSimpleNode(node.Name))
					ImGui::TextUnformatted(displayIcon);
				else
					ImGui::TextUnformatted(node.Name.c_str());

				if (mShowNodeIDs)
					ImGui::TextUnformatted(("(" + std::to_string(node.ID) + ")").c_str());

				ImGui::Spring(1, 0);
			}

			for (auto& output : node.Outputs)
			{
				if (!isSimple && output.Type == PinType::Delegate)
					continue;

				auto alpha = ImGui::GetStyle().Alpha;
				if (mState->NewLinkPin && !GetModel()->CanCreateLink(mState->NewLinkPin, &output) && &output != mState->NewLinkPin)
				{
					alpha = alpha * (48.0f / 255.0f);
				}

				UI::ScopedStyle alphaStyleOverride(ImGuiStyleVar_Alpha, alpha);

				builder.Output(ed::PinId(output.ID));

				// Draw output pin name
				if (!isSimple && !output.Name.empty() && output.Name != "Message")
				{
					ImGui::Spring(0);
					ImGui::TextUnformatted(output.Name.c_str());
				}
				ImGui::Spring(0);
				DrawPinIcon(output, GetModel()->IsPinLinked(output.ID), (int)(alpha * 255));
				builder.EndOutput();
			}

			builder.End();

			int radius = mShadow->GetHeight();// *1.4;
			UI::DrawShadow(mShadow, radius);
		}

		//=============================================================
		/// Comment nodes
		for (auto& node : GetModel()->GetNodes())
		{
			if (node.Type != NodeType::Comment)
				continue;

			//! This demonstrates low-level custom node drawing

			const float commentAlpha = 0.75f;

			// Need to disable editor shortcuts and drag-selecting while editing or dragging input field
			auto deactivateInputIfDragging = [&](bool& wasActive)
				{
					if (ImGui::IsItemActive() && !wasActive)
					{
						mState->DraggingInputField = true;
						ed::EnableShortcuts(false);
						wasActive = true;
					}
					else if (!ImGui::IsItemActive() && wasActive)
					{
						mState->DraggingInputField = false;
						ed::EnableShortcuts(true);
						wasActive = false;
					}
				};

			ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 255, 255, 60));
			ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));
			ed::BeginNode(ed::NodeId(node.ID));
			{
				UI::ScopedStyle(ImGuiStyleVar_Alpha, commentAlpha);
				UI::ScopedID nodeID(node.ID);
				ImGui::BeginVertical("content");
				ImGui::BeginHorizontal("horizontal");
				{
					UI::ShiftCursorX(6.0f);
					UI::ScopedColor frameColor(ImGuiCol_FrameBg, IM_COL32(255, 255, 255, 0));
					UI::ScopedColor textColor(ImGuiCol_Text, IM_COL32(210, 210, 210, 255));
					UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);

					static bool wasActive = false;

					static char buffer[256]{};
					memset(buffer, 0, sizeof(buffer));
					memcpy(buffer, node.Name.data(), std::min(node.Name.size(), sizeof(buffer)));

					const auto inputTextFlags = ImGuiInputTextFlags_AutoSelectAll;

					float textWidth = ImGui::CalcTextSize((node.Name + "00").c_str()).x;
					textWidth = std::min(textWidth, node.Size.x - 16.0f);

					ImGui::PushItemWidth(textWidth);
					{
						if (ImGui::InputText("##edit", buffer, 255, inputTextFlags))
						{
							node.Name = buffer;
						}
					}

					ImGui::PopItemWidth();

					deactivateInputIfDragging(wasActive);
				}
				ImGui::EndHorizontal();
				ed::Group(node.Size);
				ImGui::EndVertical();
			}
			ed::EndNode();
			ed::PopStyleColor(2);

			// Pupup hint when zoomed out
			if (ed::BeginGroupHint(ed::NodeId(node.ID)))
			{
				auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

				auto min = ed::GetGroupMin();

				ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
				ImGui::BeginGroup();
				ImGui::TextUnformatted(node.Name.c_str());
				ImGui::EndGroup();

				auto drawList = ed::GetHintBackgroundDrawList();

				auto hintBounds = UI::GetItemRect();
				auto hintFrameBounds = UI::RectExpanded(hintBounds, 8, 4);

				drawList->AddRectFilled(
					hintFrameBounds.GetTL(),
					hintFrameBounds.GetBR(),
					IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 1.0f);

				drawList->AddRect(
					hintFrameBounds.GetTL(),
					hintFrameBounds.GetBR(),
					IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 1.0f);
			}
			ed::EndGroupHint();
		}
	}

	void NodeGraphEditorBase::DrawPinIcon(const Pin& pin, bool connected, int alpha)
	{
		const int pinIconSize = GetPinIconSize();

		//IconType iconType;
		ImColor  color = GetIconColor(pin.Type);
		color.Value.w = alpha / 255.0f;

		// Draw a highlight if hovering over this pin or its label
		if (ed::PinId(pin.ID) == ed::GetHoveredPin())
		{
			auto* drawList = ImGui::GetWindowDrawList();
			auto size = ImVec2(static_cast<float>(pinIconSize), static_cast<float>(pinIconSize));
			auto cursorPos = ImGui::GetCursorScreenPos();
			const auto outline_scale = size.x / 24.0f;
			const auto extra_segments = static_cast<int>(2 * outline_scale); // for full circle
			const auto radius = 0.5f * size.x / 2.0f - 0.5f;
			const auto centre = ImVec2(cursorPos.x + size.x / 2.0f, cursorPos.y + size.y / 2.0f);
			drawList->AddCircleFilled(centre, 0.5f * size.x, IM_COL32(255, 255, 255, 30), 12 + extra_segments);
		}

		// If this pin accepting a link, draw it as connected
		bool acceptingLink = IsPinAcceptingLink(pin);


		if (pin.Storage == StorageKind::Array)
			ax::Widgets::IconGrid(ImVec2(static_cast<float>(pinIconSize), static_cast<float>(pinIconSize)), connected || acceptingLink, color, ImColor(32, 32, 32, alpha));
		else
		{
			Ref<Texture2D> image;

			if (pin.Type == PinType::Audio)
			{
				image = connected || acceptingLink ? mIconPin_A_C : mIconPin_A_D;
			}
			else
			{
				image = pin.Type == PinType::Flow ? connected || acceptingLink ? mIconPin_F_C : mIconPin_F_D
					: connected || acceptingLink ? mIconPin_V_C : mIconPin_V_D;
			}

			const float iconWidth = image->GetWidth();
			const float iconHeight = image->GetHeight();
			ax::Widgets::ImageIcon(ImVec2(static_cast<float>(iconWidth), static_cast<float>(iconHeight)), UI::GetTextureID(image), connected, (float)pinIconSize, color, ImColor(32, 32, 32, alpha));
		}
	}


	//=============================================================================================
	/// Demo Node Graph Editor
	DemoNodeGraphEditor::DemoNodeGraphEditor() = default;
	DemoNodeGraphEditor::~DemoNodeGraphEditor() = default;

	ImGuiWindowFlags DemoNodeGraphEditor::GetWindowFlags()
	{
		return ImGuiWindowFlags_NoCollapse;
	}

	NodeEditorModel* DemoNodeGraphEditor::GetModel()
	{
		return mModel.get();
	}

	void DemoNodeGraphEditor::SetAsset(const Ref<Asset>& asset)
	{
		mModel = std::make_unique<DemoNodeEditorModel>(asset.As<DemoGraph>());
		NodeGraphEditorBase::SetAsset(asset);
	}

	const char* DemoNodeGraphEditor::GetIconForSimpleNode(const std::string& simpleNodeIdentifier) const
	{
		if (choc::text::contains(simpleNodeIdentifier, "Mult")) return "MULT";
		if (choc::text::contains(simpleNodeIdentifier, "Less")) return "<";

		else return nullptr;
	}

	void DemoNodeGraphEditor::OnRender()
	{
		ImGui::SetNextWindowClass(GetWindowClass());
		ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoCollapse);
		EnsureWindowIsDocked(ImGui::GetCurrentWindow());
		{
			DrawToolbar();
		}
		ImGui::End(); // Toolbar

		ImGui::SetNextWindowClass(GetWindowClass());
		ImGui::Begin("Graph Details", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse);
		EnsureWindowIsDocked(ImGui::GetCurrentWindow());
		{
			DrawDetailsPanel();
		}
		ImGui::End(); // VariablesPanel
	}

	void DemoNodeGraphEditor::DrawToolbar()
	{
		auto* drawList = ImGui::GetWindowDrawList();

		UI::ShiftCursorX(5.0f);

		// Compile Button
		{
			int compileIconWidth = mCompileIcon->GetWidth() * 0.9f;
			int compileIconHeight = mCompileIcon->GetWidth() * 0.9f;
			if (ImGui::InvisibleButton("compile", ImVec2(compileIconWidth, compileIconHeight)))
			{
				mModel->CompileGraph();
			}
			UI::DrawButtonImage(mCompileIcon, IM_COL32(235, 165, 36, 200),
				IM_COL32(235, 165, 36, 255),
				IM_COL32(235, 165, 36, 120));

			UI::SetTooltip("Compile");
		}

		ImGui::SameLine(0, 10.0f);

		// Save Button
		{
			const int saveIconWidth = mSaveIcon->GetWidth();
			const int saveIconHeight = mSaveIcon->GetWidth();

			if (ImGui::InvisibleButton("saveGraph", ImVec2(saveIconWidth, saveIconHeight)))
			{
				mModel->SaveAll();
			}
			const int iconOffset = 4;
			auto iconRect = UI::GetItemRect();
			iconRect.Min.y += iconOffset;
			iconRect.Max.y += iconOffset;

			UI::DrawButtonImage(mSaveIcon, IM_COL32(102, 204, 163, 200),
				IM_COL32(102, 204, 163, 255),
				IM_COL32(102, 204, 163, 120), iconRect);

			UI::SetTooltip("Save graph");
		}
	}

	void DemoNodeGraphEditor::DrawDetailsPanel()
	{
		auto& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		std::vector<ed::NodeId> selectedNodes;
		std::vector<ed::LinkId> selectedLinks;
		selectedNodes.resize(ed::GetSelectedObjectCount());
		selectedLinks.resize(ed::GetSelectedObjectCount());

		int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
		int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

		selectedNodes.resize(nodeCount);
		selectedLinks.resize(linkCount);

		UI::ShiftCursorY(12.0f);
		{
			ImGui::CollapsingHeader("##NODES", ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf);
			UI::DrawBorderHorizontal();
			ImGui::SetItemAllowOverlap();
			ImGui::SameLine();
			UI::ShiftCursorX(-8.0f);
			ImGui::Text("NODES");
		}

		if (ImGui::BeginListBox("##nodesListBox", ImVec2(FLT_MIN, 0)))
		{
			for (auto& node : mModel->GetNodes())
			{
				UI::ScopedID nodeID(node.ID);
				auto start = ImGui::GetCursorScreenPos();

				bool isSelected = std::find(selectedNodes.begin(), selectedNodes.end(), ed::NodeId(node.ID)) != selectedNodes.end();
				if (ImGui::Selectable((node.Name + "##" + std::to_string(node.ID)).c_str(), &isSelected))
				{
					if (io.KeyCtrl)
					{
						if (isSelected)
							ed::SelectNode(ed::NodeId(node.ID), true);
						else
							ed::DeselectNode(ed::NodeId(node.ID));
					}
					else
						ed::SelectNode(ed::NodeId(node.ID), false);

					ed::NavigateToSelection();
				}

				if (UI::IsItemHovered() && !node.State.empty())
				{
					ImGui::SetTooltip("State: %s", node.State.c_str());
				}
			}

			ImGui::EndListBox();
		}
	}
} // namespace NotRed