#include <nrpch.h>
#include "SOULGraphEditor.h"
#include "SOULNodeEditorModel.h"

#include "NotRed/Asset/AssetManager.h"

#include "imgui-node-editor/imgui_node_editor.h"

#include "choc/text/choc_StringUtilities.h"

namespace ed = ax::NodeEditor;
namespace utils = ax::NodeEditor::Utilities;

namespace NR
{
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

	//=============================================================================================
	/// SOUL Node Graph Editor
	SOULNodeGraphEditor::SOULNodeGraphEditor()
	{
		SetTitle("SOUL Sound");

		TextureProperties clampProps;
		clampProps.SamplerWrap = TextureWrap::Clamp;
		mIconPlay = Texture2D::Create("Resources/Editor/PlayButton.png", clampProps);
		mIconStop = Texture2D::Create("Resources/Editor/StopButton.png", clampProps);

		onNodeListPopup = [&](bool searching, std::string_view searchedString)
			{
				Node* newNode = nullptr;

				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(170, 170, 170, 255));
				if (ImGui::CollapsingHeader("Graph Inputs", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::PopStyleColor(); // header Text
					ImGui::Separator();


					ImGui::Indent();
					//-------------------------------------------------
					if (searching)
					{
						for (const auto& graphInput : mModel->GetInputs().GetNames())
						{
							if (UI::IsMatchingSearch("Graph Inputs", searchedString)
								|| UI::IsMatchingSearch(graphInput, searchedString))
							{
								if (ImGui::MenuItem(choc::text::replace(graphInput, "_", " ").c_str()))
									newNode = mModel->SpawnGraphInputNode(graphInput);
							}
						}
					}
					else
					{
						for (const auto& graphInput : mModel->GetInputs().GetNames())
						{
							if (ImGui::MenuItem(choc::text::replace(graphInput, "_", " ").c_str()))
								newNode = mModel->SpawnGraphInputNode(graphInput);
						}
					}

					ImGui::Unindent();
				}
				else
				{
					ImGui::PopStyleColor(); // header Text
				}

				if (newNode)
					return newNode;
			};
	};
	SOULNodeGraphEditor::~SOULNodeGraphEditor() = default;

	ImGuiWindowFlags SOULNodeGraphEditor::GetWindowFlags()
	{
		return ImGuiWindowFlags_NoCollapse;
	}

	NodeEditorModel* SOULNodeGraphEditor::GetModel()
	{
		return mModel.get();
	}

	void SOULNodeGraphEditor::SetAsset(const Ref<Asset>& asset)
	{
		mModel = std::make_unique<SOULNodeEditorModel>(asset.As<SOULSound>());
		NodeGraphEditorBase::SetAsset(asset);

		mModel->onPinValueChanged = [&](UUID nodeID, UUID pinID) { mGraphDirty = true; };
		mModel->onNodeCreated = [&]() { mGraphDirty = true; };
		mModel->onNodeDeleted = [&]() { mGraphDirty = true; };
		mModel->onLinkCreated = [&]() { mGraphDirty = true; };
		mModel->onLinkDeleted = [&]() { mGraphDirty = true; };

		mModel->onCompiledSuccessfully = [&] { mGraphDirty = false; };

		const AssetMetadata& metadata = AssetManager::GetMetadata(asset->Handle);
		SetTitle(fmt::format("{}###{}", metadata.FilePath.stem().string(), "SOUL Sound").c_str());
	}

	const char* SOULNodeGraphEditor::GetIconForSimpleNode(const std::string& simpleNodeIdentifier) const
	{
		if (choc::text::contains(simpleNodeIdentifier, "Mult")) return "x";
		if (choc::text::contains(simpleNodeIdentifier, "Divide")) return "/";
		if (choc::text::contains(simpleNodeIdentifier, "Add")) return "+";
		if (choc::text::contains(simpleNodeIdentifier, "Subtract")) return "-";
		if (choc::text::contains(simpleNodeIdentifier, "Less")) return "<";
		if (choc::text::contains(simpleNodeIdentifier, "Modulo")) return "%";

		else return nullptr;
	}

	void SOULNodeGraphEditor::OnRender()
	{
		ImGui::SetNextWindowClass(GetWindowClass());

		if (ImGui::Begin("Toolbar##SOULGraphEditor", nullptr, ImGuiWindowFlags_NoCollapse))
		{
			EnsureWindowIsDocked(ImGui::GetCurrentWindow());
			DrawToolbar();
		}
		ImGui::End(); // Toolbar

		ImGui::SetNextWindowClass(GetWindowClass());
		if (ImGui::Begin("Graph Details", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse))
		{
			EnsureWindowIsDocked(ImGui::GetCurrentWindow());
			DrawDetailsPanel();
		}
		ImGui::End(); // VariablesPanel

		ImGui::SetNextWindowClass(GetWindowClass());
		if (ImGui::Begin("Graph Inputs##SOULGraphEditor", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse))
		{
			EnsureWindowIsDocked(ImGui::GetCurrentWindow());
			DrawGraphIO();
		}
		ImGui::End(); // VariablesPanel
	}

	void SOULNodeGraphEditor::DrawToolbar()
	{
		auto* drawList = ImGui::GetWindowDrawList();

		const float spacing = 16.0f;

		ImGui::BeginHorizontal("ToolbarHorizontalLayout", ImGui::GetContentRegionAvail());
		ImGui::Spring(0.0f, spacing);

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

			if (mGraphDirty)
			{
				UI::ShiftCursorX(-6.0f);
				ImGui::Text("*");
			}
		}

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

		// Viewport to content
		{
			if (ImGui::Button("Navigate to Content"))
				ed::NavigateToContent();
		}

		const float leftSizeOffset = ImGui::GetCursorPosX();

		ImGui::Spring();

		// Playback butons
		// ---------------

		// Play Button
		{
			if (ImGui::InvisibleButton("PlayGraphButton", ImVec2(ImGui::GetTextLineHeightWithSpacing() + 4.0f,
				ImGui::GetTextLineHeightWithSpacing() + 4.0f)))
			{
				if (mGraphDirty)
					mModel->CompileGraph();

				if (!mGraphDirty && mModel->onPlay)
					mModel->onPlay();
			}

			UI::DrawButtonImage(mIconPlay, IM_COL32(102, 204, 163, 200),
				IM_COL32(102, 204, 163, 255),
				IM_COL32(102, 204, 163, 120),
				UI::RectExpanded(UI::GetItemRect(), 1.0f, 1.0f));
		}

		// Stop Button
		{
			if (ImGui::InvisibleButton("StopGraphButton", ImVec2(ImGui::GetTextLineHeightWithSpacing() + 4.0f,
				ImGui::GetTextLineHeightWithSpacing() + 4.0f)))
			{
				if (mModel->onStop) mModel->onStop();
			}

			UI::DrawButtonImage(mIconStop, IM_COL32(102, 204, 163, 200),
				IM_COL32(102, 204, 163, 255),
				IM_COL32(102, 204, 163, 120),
				UI::RectExpanded(UI::GetItemRect(), -1.0f, -1.0f));
		}

		ImGui::Spring(1.0f, leftSizeOffset);

		ImGui::Spring(0.0f, spacing);
		ImGui::EndHorizontal();
	}

	void SOULNodeGraphEditor::DrawDetailsPanel()
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
					ImGui::SetTooltip("State: %s", node.State.c_str());

				//? Display node IDs
				/*auto id = std::string("(") + std::to_string(node.ID) + ")";
				auto textSize = ImGui::CalcTextSize(id.c_str(), nullptr);

				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - textSize.x);
				ImGui::Text(id.c_str());*/
			}

			ImGui::EndListBox();
		}
	}

	void SOULNodeGraphEditor::DrawGraphIO()
	{
		const float panelWidth = ImGui::GetContentRegionAvail().x;

		//=============================================================================
		/// Inputs & Outputs

		if (ImGui::BeginChild("Inputs Outputs"))
		{

			auto addInputButton = [&]
				{
					ImGui::SameLine(panelWidth - ImGui::GetFrameHeight() - ImGui::CalcTextSize("+ Output").x);
					if (ImGui::SmallButton("+ Input"))
					{
						mModel->AddInputToGraph(choc::value::createFloat32(0.0f));
					}
				};

			const int leafFlags = ImGuiTreeNodeFlags_Leaf
				| ImGuiTreeNodeFlags_NoTreePushOnOpen
				| ImGuiTreeNodeFlags_SpanAvailWidth
				| ImGuiTreeNodeFlags_FramePadding
				| ImGuiTreeNodeFlags_AllowItemOverlap;

			const int headerFlags = ImGuiTreeNodeFlags_CollapsingHeader
				| ImGuiTreeNodeFlags_AllowItemOverlap
				| ImGuiTreeNodeFlags_DefaultOpen;

			// INPUTS list
			ImGui::Spacing();
			if (ImGui::CollapsingHeader("INPUTS", headerFlags))
			{
				addInputButton();

				ImGui::TreeNodeEx("Input Action", leafFlags);

				std::string inputToRemove;

				for (auto& input : mModel->GetInputs().GetNames())
				{
					bool selected = input == mSelectedInputOutput;
					int flags = selected ? leafFlags | ImGuiTreeNodeFlags_Selected : leafFlags;

					// Coloring text, just because
					ImColor textColor = GetTypeColor(mModel->GetInputs().GetValue(input));

					ImGui::PushStyleColor(ImGuiCol_Text, textColor.Value);
					ImGui::TreeNodeEx(input.c_str(), flags);
					ImGui::PopStyleColor();

					if (ImGui::IsItemClicked()) mSelectedInputOutput = input;

					// Remove Input buton
					ImGui::SameLine();
					ImGui::SetCursorPosX(panelWidth - ImGui::GetFrameHeight());

					ImGui::PushID((input + "removeInput").c_str());
					if (ImGui::SmallButton("x"))
						inputToRemove = input;
					ImGui::PopID();
				}

				if (!inputToRemove.empty())
				{
					if (inputToRemove == mSelectedInputOutput)
						mSelectedInputOutput = "";
					mModel->RemoveInputFromGraph(inputToRemove);
				}
			}
			else
			{
				addInputButton();
			}

			auto addOutputButton = [&]
				{
					ImGui::SameLine(panelWidth - ImGui::GetFrameHeight() - ImGui::CalcTextSize("+ Output").x);
					if (ImGui::SmallButton("+ Output"))
					{
						// TODO: add output
						NR_CORE_WARN("Add Output to Graph not implemented.");
					}
				};

			// OUTPUTS list
			if (ImGui::CollapsingHeader("OUTPUTS", headerFlags))
			{
				addOutputButton();

				ImGui::TreeNodeEx("Output Audio", leafFlags);
			}
			else
			{
				addOutputButton();
			}
		}
		ImGui::EndChild();

		//=============================================================================
		/// Details

		ImGui::SetNextWindowClass(GetWindowClass());
		if (ImGui::Begin("Details##SOULNodeGraph"))
		{
			if (!mSelectedInputOutput.empty())
			{
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Indent();

				std::string oldName = mSelectedInputOutput;
				std::string newName = mSelectedInputOutput;

				choc::value::Value value = mModel->GetInputs().GetValue(newName);

				// Name
				{
					static char buffer[128]{};
					memset(buffer, 0, sizeof(buffer));
					memcpy(buffer, mSelectedInputOutput.data(), std::min(mSelectedInputOutput.size(), sizeof(buffer)));

					const auto inputTextFlags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;

					ImGui::Text("Input Name");
					ImGui::SameLine();
					UI::ShiftCursorY(-3.0f);

					ImGui::InputText("##inputName", buffer, 127, inputTextFlags);

					UI::DrawItemActivityOutline(2.5f, true, Colors::Theme::accent);


					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						// Rename item in the list
						newName = buffer;
						mModel->GetInputs().Remove(oldName);
						mModel->GetInputs().Set(newName, value);

						mSelectedInputOutput = buffer;

						mModel->OnGraphInputNameChanged(oldName, newName);
					}
				}

				// Type & Value
				{
					std::string typeName;
					int32_t selected = 0;

					static std::vector<std::string> types{ "Float", "Int", "Bool", "String", "WaveAsset" }; // TODO: Function/Event/Trigger

					choc::value::Type type = value.getType();
					choc::value::Type arrayType;

					bool isArrayOrVector = false;
					bool isAssetHandle = false;

					if (value.isArray())
					{
						arrayType = type;
						isArrayOrVector = true;
						type = type.getElementType();
						if (type.isObjectWithClassName("AssetHandle"))
							isAssetHandle = true;
					}

					if (type.isPrimitive() || type.isString())
					{
						if (type.isFloat())   selected = 0;
						else if (type.isInt32())   selected = 1;
						else if (type.isBool())    selected = 2;
						else if (type.isString())  selected = 3;

						else NR_CORE_ERROR("Invalid type of Graph Input!");
					}
					else if (type.isObjectWithClassName("AssetHandle"))// isObject() && isArrayOrVector ? value[0].hasObjectMember("AssetHandle")
						//: value.hasObjectMember("AssetHandle"))
					{
						selected = 4;
					}

					auto createValueFromSelectedIndex = [](int32_t index)
						{
							if (index == 0) return choc::value::Value(0.0f);
							if (index == 1) return choc::value::Value(0);
							if (index == 2) return choc::value::Value(false);
							if (index == 3) return choc::value::Value(std::string());
							if (index == 4)
							{
								return CreateAssetRefObject();
							}

							return choc::value::Value();
						};


					// Change the type the value, or the type of array element
					ImGui::SetCursorPosX(0.0f);

					const float checkboxWidth = 84.0f;
					ImGui::BeginChildFrame(ImGui::GetID("TypeRegion"), ImVec2(ImGui::GetContentRegionAvail().x - checkboxWidth, ImGui::GetFrameHeight() * 1.4f), ImGuiWindowFlags_NoBackground);
					ImGui::Columns(2);

					auto getColorForType = [&](int optionNumber)
						{
							return GetTypeColor(createValueFromSelectedIndex(optionNumber));
						};

					if (UI::PropertyDropdown("Type", types, (int32_t)types.size(), &selected, getColorForType))
					{
						choc::value::Value newValue = createValueFromSelectedIndex(selected);

						isAssetHandle = newValue.getType().isObjectWithClassName("AssetHandle");

						if (isArrayOrVector)
						{
							choc::value::Value arrayValue = choc::value::createArray(value.size(), [&](uint32_t i)
								{
									if (isAssetHandle)
										return CreateAssetRefObject();
									else
										return choc::value::Value(newValue.getType());
								});

							mModel->GetInputs().Set(newName, arrayValue);
						}
						else
						{
							mModel->GetInputs().Set(newName, newValue);
						}

						mModel->OnGraphInputTypeChanged(newName);
					}
					UI::DrawItemActivityOutline(2.5f, false);
					ImGui::EndColumns();
					ImGui::EndChildFrame();

					// Update value handle in case type or name has changed
					value = mModel->GetInputs().GetValue(newName);
					isArrayOrVector = value.isArray();

					// Switch between Array vs Value

					ImGui::SameLine(0.0f, 0.0f);
					UI::ShiftCursorY(3);

					bool isArray = value.isArray();
					if (ImGui::Checkbox("Is Array", &isArray))
					{
						if (isArray)
						{
							// Became array

							choc::value::Value arrayValue = choc::value::createEmptyArray();
							arrayValue.addArrayElement(value);

							mModel->GetInputs().Set(newName, arrayValue);
							isArrayOrVector = true;
							value = arrayValue;
						}
						else
						{
							// Became simple
							auto newValue = choc::value::Value(value.getType().getElementType());

							mModel->GetInputs().Set(newName, newValue);
							isArrayOrVector = false;
							value = newValue;
						}

						mModel->OnGraphInputTypeChanged(newName);
					}

					UI::DrawItemActivityOutline(2.5f, true);

					ImGui::Unindent();

					ImGui::Dummy(ImVec2(panelWidth, 10.0f));

					// Assign default value to the Value or Array elements

					auto valueEditField = [](const char* label, choc::value::ValueView& valueView, bool* removeButton = nullptr)
						{
							ImGui::Text(label);
							ImGui::NextColumn();
							ImGui::PushItemWidth(-ImGui::GetFrameHeight());

							bool changed = false;
							const std::string id = "##" + std::string(label);

							if (valueView.isFloat())
							{
								float v = valueView.get<float>();
								if (ImGui::DragFloat(id.c_str(), &v, 0.01f, 0.0f, 0.0f, "%.2f"))
								{
									valueView.set(v);
									changed = true;
								}

							}
							else if (valueView.isInt32())
							{
								int32_t v = valueView.get<int32_t>();
								if (ImGui::DragInt(id.c_str(), &v, 0.1f, 0, 0))
								{
									valueView.set(v);
									changed = true;
								}
							}
							else if (valueView.isBool())
							{
								bool v = valueView.get<bool>();
								if (ImGui::Checkbox(id.c_str(), &v))
								{
									valueView.set(v);
									changed = true;
								}
							}
							else if (valueView.isString())
							{
								std::string v = valueView.get<std::string>();

								static char buffer[128]{};
								memset(buffer, 0, sizeof(buffer));
								memcpy(buffer, v.data(), std::min(v.size(), sizeof(buffer)));

								const auto inputTextFlags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
								ImGui::InputText(id.c_str(), buffer, 127, inputTextFlags);

								if (ImGui::IsItemDeactivatedAfterEdit())
								{
									valueView.set(std::string(buffer));
									changed = true;
								}
							}
							else if (valueView.isObjectWithClassName("AssetHandle"))
							{
								ImGui::PushID((id + "AssetReference").c_str());

								AssetHandle selected = 0;
								Ref<AudioFile> asset;

								auto handleStr = valueView["Value"].get<std::string>();
								selected = handleStr.empty() ? 0 : std::stoull(handleStr);

								if (AssetManager::IsAssetHandleValid(selected))
								{
									asset = AssetManager::GetAsset<AudioFile>(selected);
								}
								// TODO: get supported asset type from the pin
								//		Initialize choc::Value class object with correct asset type using helper function
								bool assetDropped = false;
								if (UI::AssetReferenceDropTargetButton(id.c_str(), asset, AssetType::Audio, assetDropped))
								{
									ImGui::OpenPopup((id + "GraphInputWaveAssetPopup").c_str());
								}

								if (assetDropped)
								{
									valueView["Value"].set(std::to_string((uint64_t)selected));
									changed = true;
								}

								bool clear = false;
								if (UI::Widgets::AssetSearchPopup((id + "GraphInputWaveAssetPopup").c_str(), AssetType::Audio, selected, &clear))
								{
									if (clear)
									{
										selected = 0;
										valueView["Value"].set(std::string("0"));
									}
									else
									{
										valueView["Value"].set(std::to_string((uint64_t)selected));
									}


									changed = true;
								}

								ImGui::PopID();
							}
							else
							{
								NR_CORE_ERROR("Invalid type of Graph Input!");
							}

							UI::DrawItemActivityOutline(2.5f, true, Colors::Theme::accent);

							ImGui::PopItemWidth();

							if (removeButton != nullptr)
							{
								ImGui::SameLine();

								ImGui::PushID((id + "removeButton").c_str());

								if (ImGui::SmallButton("x"))
									*removeButton = true;

								ImGui::PopID();
							}

							ImGui::NextColumn();

							return changed;
						};


					ImGui::GetWindowDrawList()->AddRectFilled(
						ImGui::GetCursorScreenPos() - ImVec2(0.0f, 2.0f),
						ImGui::GetCursorScreenPos() + ImVec2(panelWidth, ImGui::GetTextLineHeightWithSpacing()),
						ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), 2.5f);

					ImGui::Spacing(); ImGui::SameLine(); UI::ShiftCursorY(2.0f);
					ImGui::Text("Default Value");

					ImGui::Spacing();
					ImGui::Dummy(ImVec2(panelWidth, 4.0f));

					ImGui::Indent();

					if (!isArrayOrVector)
					{
						ImGui::Columns(2);
						if (valueEditField("Value", value.getViewReference()))
						{
							mModel->GetInputs().Set(newName, value);
							mModel->OnGraphInputValueChanged(newName);
						}
						ImGui::EndColumns();
					}
					else // Array
					{
						ImGui::BeginHorizontal("ArrayElementsInfo", { ImGui::GetContentRegionAvail().x, 0.0f });
						uint32_t size = value.size();
						ImGui::Text((std::to_string(size) + " Array elements").c_str());

						ImGui::Spring();

						if (ImGui::SmallButton("Add Element"))
						{
							if (isAssetHandle)
								value.addArrayElement(CreateAssetRefObject());// choc::value::Value(value.getType().getElementType()));
							else
								value.addArrayElement(choc::value::Value(value.getType().getElementType()));

							mModel->GetInputs().Set(newName, value);
						}
						ImGui::Spring(0.0f, 4.0f);
						ImGui::EndHorizontal();

						ImGui::Spacing();

						ImGui::Columns(2);

						bool changed = false;
						int indexToRemove = -1;
						UI::PushID();
						for (uint32_t i = 0; i < value.size(); ++i)
						{
							choc::value::ValueView vv = value[i];

							bool removeThis = false;
							if (valueEditField(("[ " + std::to_string(i) + " ]").c_str(), vv, &removeThis))
								changed = true;

							if (value.size() > 1 && removeThis) indexToRemove = i;
						}
						UI::PopID();

						if (indexToRemove >= 0)
						{
							bool skipIteration = false;
							value = choc::value::createArray(value.size() - 1, [&value, &skipIteration, indexToRemove](uint32_t i)
								{
									if (!skipIteration)
										skipIteration = i == indexToRemove;
									return skipIteration ? value[i + 1] : value[i];
								});
							changed = true;
						}

						if (changed)
						{
							mModel->GetInputs().Set(newName, value);
							mModel->OnGraphInputValueChanged(newName);
						}

						ImGui::EndColumns();
					}
					// TODO: invalidate connections that were using this input

					ImGui::Unindent();
				}
			}

			//? DBG Info
			ImGui::Dummy(ImVec2(panelWidth, 40.0f));
			ImGui::BeginHorizontal("##debug");
			ImGui::Checkbox("Show IDs", &mShowNodeIDs);
			ImGui::EndHorizontal();
		}
		ImGui::End();
	}

	ImColor SOULNodeGraphEditor::GetTypeColor(const choc::value::Value& v)
	{
		choc::value::Type type = v.isArray() ? v.getType().getElementType() : v.getType();

		if (v.isObject() && v.hasObjectMember("Type"))
		{
			NR_CORE_WARN("Found pin with custom type {}, need to handle it!", v["Type"].getString());
		}
		else if (type.isFloat())  return GetIconColor(PinType::Float);
		else if (type.isInt())    return GetIconColor(PinType::Int);
		else if (type.isBool())   return GetIconColor(PinType::Bool);
		else if (type.isString()) return GetIconColor(PinType::String);
		else if (type.isObjectWithClassName("AssetHandle"))
			return GetIconColor(PinType::Object);
		else
			return ImColor(220, 220, 220, 255);
	}

	std::pair<PinType, StorageKind> SOULNodeGraphEditor::GetPinTypeAndStorageKindForValue(const choc::value::Value& v)
	{
		PinType pinType;
		const bool isArray = v.isArray();
		auto type = isArray ? v.getType().getElementType() : v.getType();

		if (v.isObject() && v.hasObjectMember("Type"))
		{
			NR_CORE_WARN("Found pin with custom type {}, need to handle it!", v["Type"].getString());
		}
		else if (type.isFloat())  pinType = PinType::Float;
		else if (type.isInt())    pinType = PinType::Int;
		else if (type.isBool())   pinType = PinType::Bool;
		else if (type.isString()) pinType = PinType::String;
		else if (type.isObjectWithClassName("AssetHandle"))
			pinType = PinType::Object;

		else NR_CORE_ASSERT(false);

		return { pinType, isArray ? StorageKind::Array : StorageKind::Value };
	}

} // namespace NotRed