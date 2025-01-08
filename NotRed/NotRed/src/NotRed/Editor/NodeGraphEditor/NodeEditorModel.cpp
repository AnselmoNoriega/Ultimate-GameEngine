#include <nrpch.h>
#include "NodeEditorModel.h"

#include <sstream>

#include "NotRed/Asset/AssetManager.h"

#include "choc/text/choc_StringUtilities.h"

namespace NR
{
    //=============================================================================
    /// IDs

#pragma region IDs

    uintptr_t NodeEditorModel::GetIDFromString(const std::string& idString) const
    {
        std::stringstream stream(idString);
        uintptr_t ret;
        stream >> ret;
        return ret;
    }

    void NodeEditorModel::AssignIds(Node* node)
    {
        node->ID = UUID();

        for (auto& input : node->Inputs)
            input.ID = UUID();

        for (auto& output : node->Outputs)
            output.ID = UUID();

    }

    void NodeEditorModel::OnNodeSpawned(Node* node)
    {
        if (node)
        {
            AssignIds(node);
            GetNodes().push_back(*node);
        }
    }

#pragma endregion IDs


    //=============================================================================
    /// Find items

#pragma region Find objects

    Node* NodeEditorModel::FindNode(UUID id)
    {
        for (auto& node : GetNodes())
            if (node.ID == id)
                return &node;

        return nullptr;
    }

    const Node* NodeEditorModel::FindNode(UUID id) const
    {
        for (auto& node : GetNodes())
            if (node.ID == id)
                return &node;

        return nullptr;
    }

    Link* NodeEditorModel::FindLink(UUID id)
    {
        for (auto& link : GetLinks())
            if (link.ID == id)
                return &link;

        return nullptr;
    }

    const Link* NodeEditorModel::FindLink(UUID id) const
    {
        for (auto& link : GetLinks())
            if (link.ID == id)
                return &link;

        return nullptr;
    }

    Pin* NodeEditorModel::FindPin(UUID id)
    {
        if (!id)
            return nullptr;

        for (auto& node : GetNodes())
        {
            for (auto& pin : node.Inputs)
                if (pin.ID == id)
                    return &pin;

            for (auto& pin : node.Outputs)
                if (pin.ID == id)
                    return &pin;
        }

        return nullptr;
    }

    const Pin* NodeEditorModel::FindPin(UUID id) const
    {
        if (!id)
            return nullptr;

        for (auto& node : GetNodes())
        {
            for (auto& pin : node.Inputs)
                if (pin.ID == id)
                    return &pin;

            for (auto& pin : node.Outputs)
                if (pin.ID == id)
                    return &pin;
        }

        return nullptr;
    }

#pragma endregion Find items


    //=============================================================================
    /// Links

#pragma region Links

    Link* NodeEditorModel::GetLinkConnectedToPin(UUID id)
    {
        if (!id)
            return nullptr;

        for (auto& link : GetLinks())
            if (link.StartPinID == id || link.EndPinID == id)
                return &link;

        return nullptr;
    }

    const Link* NodeEditorModel::GetLinkConnectedToPin(UUID id) const
    {
        if (!id)
            return nullptr;

        for (auto& link : GetLinks())
            if (link.StartPinID == id || link.EndPinID == id)
                return &link;

        return nullptr;
    }

    bool NodeEditorModel::IsPinLinked(UUID id)
    {
        return GetLinkConnectedToPin(id) != nullptr;
    }

    void NodeEditorModel::DeleteDeadLinks(UUID nodeId)
    {
        auto wasConnectedToTheNode = [&](const Link& link)
            {
                return (!FindPin(link.StartPinID)) || (!FindPin(link.EndPinID))
                    || FindPin(link.StartPinID)->Node->ID == nodeId
                    || FindPin(link.EndPinID)->Node->ID == nodeId;
            };

        auto& links = GetLinks();

        auto removeIt = std::remove_if(links.begin(), links.end(), wasConnectedToTheNode);
        const bool linkRemoved = removeIt != links.end();

        links.erase(removeIt, links.end());

        if (linkRemoved)
            OnLinkDeleted();
    }

    NodeEditorModel::LinkQueryResult NodeEditorModel::CanCreateLink(Pin* startPin, Pin* endPin)
    {
        if (!startPin)                                  return LinkQueryResult::InvalidStartPin;
        else if (!endPin)                               return LinkQueryResult::InvalidEndPin;
        else if (endPin == startPin)                    return LinkQueryResult::SamePin;
        else if (endPin->Node == startPin->Node)        return LinkQueryResult::SameNode;
        else if (endPin->Kind == startPin->Kind)        return LinkQueryResult::IncompatiblePinKind;
        else if (endPin->Storage != startPin->Storage)  return LinkQueryResult::IncompatibleStorageKind;
        else if (endPin->Type != startPin->Type)        return LinkQueryResult::IncompatibleType;

        return LinkQueryResult::CanConnect;
    }

    void NodeEditorModel::CreateLink(Pin* startPin, Pin* endPin)
    {
        auto& links = GetLinks();

        links.emplace_back(Link(UUID(), startPin->ID, endPin->ID));
        links.back().Color = GetIconColor(startPin->Type);

        OnLinkCreated();
    }

    void NodeEditorModel::RemoveLink(UUID linkId)
    {
        auto& links = GetLinks();

        auto id = std::find_if(links.begin(), links.end(), [linkId](auto& link) { return link.ID == linkId; });
        if (id != links.end())
        {
            links.erase(id);

            OnLinkDeleted();
        }
    }

#pragma endregion Links


    //=============================================================================
    /// Nodes

#pragma region Nodes

    void NodeEditorModel::RemoveNode(UUID nodeId)
    {
        auto& nodes = GetNodes();

        auto id = std::find_if(nodes.begin(), nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
        if (id != nodes.end())
        {
            DeleteDeadLinks(nodeId);
            nodes.erase(id);

            OnNodeDeleted();
        }
    }

    void NodeEditorModel::BuildNode(Node* node)
    {
        for (auto& input : node->Inputs)
        {
            input.Node = node;
            input.Kind = PinKind::Input;
        }

        for (auto& output : node->Outputs)
        {
            output.Node = node;
            output.Kind = PinKind::Output;
        }
    }

    void NodeEditorModel::BuildNodes()
    {
        for (auto& node : GetNodes())
            BuildNode(&node);
    }

    Node* NodeEditorModel::CreateNode(const std::string& category, const std::string& typeID)
    {
        std::string safeTypeName = choc::text::replace(typeID, " ", "_");
        std::string safeCategory = choc::text::replace(category, " ", "_");

        OnNodeSpawned(GetNodeFactory()->SpawnNode(safeCategory, safeTypeName));
        BuildNode(&GetNodes().back());

        OnNodeCreated();

        return &GetNodes().back();
    }

#pragma endregion Nodes


    //=============================================================================
    /// Serialization

#pragma region Serialization

    void NodeEditorModel::SaveAll()
    {
        Serialize();
    }

    void NodeEditorModel::LoadAll()
    {
        Deserialize();
        BuildNodes();
    }

#pragma endregion Serialization

    //=============================================================================
    /// Model Interface

    void NodeEditorModel::CompileGraph()
    {
        OnCompileGraph();
    }

} // namespace