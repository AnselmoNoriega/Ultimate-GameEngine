#pragma once

#include <functional>

#include "Nodes.h"
#include "NodeGraphAsset.h"

#include "choc/containers/choc_Value.h"

#include "NotRed/Asset/AssetImporter.h"

namespace NR
{
    class NodeEditorModel
    {
    public:
        NodeEditorModel(const NodeEditorModel&) = delete;
        NodeEditorModel operator=(const NodeEditorModel&) = delete;

        NodeEditorModel() = default;
        virtual ~NodeEditorModel() = default;

        //==================================================================================
        /// Graph Model Interface
        virtual std::vector<Node>& GetNodes() = 0;
        virtual std::vector<Link>& GetLinks() = 0;
        virtual const std::vector<Node>& GetNodes() const = 0;
        virtual const std::vector<Link>& GetLinks() const = 0;

        virtual const Nodes::Registry& GetNodeTypes() const = 0;

        //==================================================================================
        /// Graph Model
        uintptr_t GetIDFromString(const std::string& idString) const;

        Node* FindNode(UUID id);
        Link* FindLink(UUID id);
        Pin* FindPin(UUID id);
        const Node* FindNode(UUID id) const;
        const Link* FindLink(UUID id) const;
        const Pin* FindPin(UUID id)   const;

        Link* GetLinkConnectedToPin(UUID id);
        const Link* GetLinkConnectedToPin(UUID id) const;
        bool IsPinLinked(UUID id);

        struct LinkQueryResult
        {
            enum
            {
                CanConnect,
                InvalidStartPin,
                InvalidEndPin,
                IncompatiblePinKind,
                IncompatibleStorageKind,
                IncompatibleType,
                SamePin,
                SameNode
            } Reason;

            LinkQueryResult(decltype(Reason) value) : Reason(value) {}

            explicit operator bool()
            {
                return Reason == LinkQueryResult::CanConnect;
            }
        };
        LinkQueryResult CanCreateLink(Pin* startPin, Pin* endPin);
        void CreateLink(Pin* startPin, Pin* endPin);
        void RemoveLink(UUID linkId);

        Node* CreateNode(const std::string& category, const std::string& typeID);
        void  RemoveNode(UUID nodeId);

        void SaveAll();
        void LoadAll();

        void CompileGraph();

    private:
        //==================================================================================
        /// Internal
        void AssignIds(Node* node);
        void OnNodeSpawned(Node* node);
        void DeleteDeadLinks(UUID nodeId);

    protected:
        // Assigns relationship between pins and nodes
        void BuildNode(Node* node);
        void BuildNodes();

        //==================================================================================
        /// Graph Model Interface
    public:
        std::function<void()> onNodeCreated = nullptr, onNodeDeleted = nullptr;
        std::function<void()> onLinkCreated = nullptr, onLinkDeleted = nullptr;

        std::function<bool(const Ref<Asset>& graphAsset)> onCompile = nullptr;
        std::function<void()> onPlay = nullptr, onStop = nullptr;

    protected:
        virtual void OnNodeCreated() { if (onNodeCreated) onNodeCreated(); }
        virtual void OnNodeDeleted() { if (onNodeDeleted) onNodeDeleted(); }
        virtual void OnLinkCreated() { if (onLinkCreated) onLinkCreated(); }
        virtual void OnLinkDeleted() { if (onLinkDeleted) onLinkDeleted(); }

        virtual void OnCompileGraph() {};

    private:
        virtual void Serialize() = 0;
        virtual void Deserialize() = 0;

        virtual Nodes::AbstractFactory* GetNodeFactory() = 0;

    };

    inline bool operator==(NodeEditorModel::LinkQueryResult a, NodeEditorModel::LinkQueryResult b)
    {
        return a.Reason == b.Reason;
    }

    inline bool operator!=(NodeEditorModel::LinkQueryResult a, NodeEditorModel::LinkQueryResult b)
    {
        return !(a == b);
    }


    //==================================================================================
    /// Demo Model
    class DemoNodeEditorModel : public NodeEditorModel
    {
    public:
        explicit DemoNodeEditorModel(Ref<DemoGraph> graphAsset) : mGraphAsset(graphAsset) { Deserialize(); }
        ~DemoNodeEditorModel() = default;

        std::vector<Node>& GetNodes() override { return mGraphAsset->Nodes; }
        std::vector<Link>& GetLinks() override { return mGraphAsset->Links; }
        const std::vector<Node>& GetNodes() const override { return mGraphAsset->Nodes; }
        const std::vector<Link>& GetLinks() const override { return mGraphAsset->Links; }

        const Nodes::Registry& GetNodeTypes() const override { return mNodeFactory.Registry; }

    private:
        void Serialize() override { AssetImporter::Serialize(mGraphAsset); };
        void Deserialize() override { BuildNodes(); };

        Nodes::AbstractFactory* GetNodeFactory() override { return &mNodeFactory; }

        void OnCompileGraph() override;

    private:
        Nodes::DemoFactory mNodeFactory;
        Ref<DemoGraph> mGraphAsset = nullptr;
    };

} // namespace