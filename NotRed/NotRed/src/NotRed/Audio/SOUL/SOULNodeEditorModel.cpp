#include <nrpch.h>
#include "SOULNodeEditorModel.h"

#include "SOULGraphEditor.h"

#include "imgui-node-editor/imgui_node_editor.h"

namespace NR
{
    bool SOULNodeEditorModel::SaveGraphState(const char* data, size_t size)
    {
        mGraphAsset->GraphState = data;
        return true;
    }

    const std::string& SOULNodeEditorModel::LoadGraphState()
    {
        return mGraphAsset->GraphState;
    }

    void SOULNodeEditorModel::OnCompileGraph()
    {
        NR_CORE_INFO("Compiling graph...");

        SaveAll();

        if (onCompile)
        {
            if (onCompile(mGraphAsset.As<Asset>())) NR_CORE_INFO("Graph has been compiled.");
            else                                     NR_CORE_ERROR("Failed to compile graph!");
        }
        else
        {
            NR_CORE_ERROR("Failed to compile graph!");
        }
    }

    void SOULNodeEditorModel::AddInputToGraph(const choc::value::ValueView& value)
    {
        std::string name = soul::addSuffixToMakeUnique("New Input", [&](const std::string& s)
            {
                auto inputs = mGraphAsset->GraphInputs.GetNames();
                return std::any_of(inputs.begin(), inputs.end(), [&s](const std::string name) { return name == s; });
            });

        mGraphAsset->GraphInputs.Set(name, value);

        //? DBG
        //NR_CORE_WARN("Added Input {} to the graph. Inputs:", name);
        //NR_LOG_TRACE(mGraphInputs.ToJSON());
    }
    void SOULNodeEditorModel::RemoveInputFromGraph(const std::string& name)
    {
        mGraphAsset->GraphInputs.Remove(name);

        //? DBG
        //NR_CORE_WARN("Removed Input {} from the graph. Inputs:", name);
    }

    void SOULNodeEditorModel::OnGraphInputNameChanged(const std::string& oldName, const std::string& newName)
    {
        for (auto& node : mGraphAsset->Nodes)
        {
            if (node.Name == "Input") //? not ideal to have string literal like this
            {
                auto& output = node.Outputs[0];
                if (output.Name == oldName)
                    output.Name = newName;
            }
        }
    }

    void SOULNodeEditorModel::OnGraphInputTypeChanged(const std::string& inputName)
    {
        auto newVariable = mGraphAsset->GraphInputs.GetValue(inputName);

        const bool isArray = newVariable.isArray();
        std::pair<PinType, StorageKind> type = SOULNodeGraphEditor::GetPinTypeAndStorageKindForValue(newVariable);

        for (auto& node : mGraphAsset->Nodes)
        {
            if (node.Name == "Input")
            {
                auto& output = node.Outputs[0];
                if (output.Name == inputName)
                {
                    output.Type = type.first;
                    output.Storage = type.second;
                    output.Value = newVariable; // TODO: should use one or the other
                    node.Color = SOULNodeGraphEditor::GetTypeColor(newVariable);

                    if (IsPinLinked(output.ID))
                    {
                        if (Link* connectedLink = GetLinkConnectedToPin(output.ID))
                        {
                            ax::NodeEditor::DeleteLink(ax::NodeEditor::LinkId(connectedLink->ID));
                            RemoveLink(connectedLink->ID);
                        }
                    }
                }
            }
        }
    }

    void SOULNodeEditorModel::OnGraphInputValueChanged(const std::string& inputName)
    {
        auto newValue = mGraphAsset->GraphInputs.GetValue(inputName);

        for (auto& node : mGraphAsset->Nodes)
        {
            if (node.Name == "Input")
            {
                auto& output = node.Outputs[0];
                if (output.Name == inputName)
                    output.Value = newValue;
            }
        }
    }

    Node* SOULNodeEditorModel::SpawnGraphInputNode(const std::string& inputName)
    {
        if (!mGraphAsset->GraphInputs.HasValue(inputName))
        {
            NR_CORE_ERROR("SpawnGraphInputNode() - input with the name \"{}\" doesn't exist!", inputName);
            return nullptr;
        }

        //? Should GetValue() return it by reference?
        choc::value::Value input = mGraphAsset->GraphInputs.GetValue(inputName);

        ImColor typeColor = SOULNodeGraphEditor::GetTypeColor(input);
        std::pair<PinType, StorageKind> type = SOULNodeGraphEditor::GetPinTypeAndStorageKindForValue(input);

        if (input.isVoid())
        {
            NR_CORE_ERROR("SpawnGraphInputNode() - invalid Graph Input value to spawn Graph Input node.");
            return nullptr;
        }

        auto* node = new Node(0, "Input", typeColor);
        node->Category = "Graph Input";

        node->Outputs.emplace_back(0, inputName.c_str(), type.first, type.second, input);

        OnNodeSpawned(node);
        BuildNode(&GetNodes().back());

        OnNodeCreated();

        return &GetNodes().back();
    }

} // namespace NR