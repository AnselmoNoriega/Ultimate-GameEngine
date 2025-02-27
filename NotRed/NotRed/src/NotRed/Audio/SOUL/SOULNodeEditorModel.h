#pragma once

#include "NotRed/Editor/NodeGraphEditor/NodeEditorModel.h"
#include "NotRed/Editor/NodeGraphEditor/PropertySet.h"

namespace NR
{
    //==================================================================================
    /// SOUL Graph Editor Model
    class SOULNodeEditorModel : public NodeEditorModel
    {
    public:
        explicit SOULNodeEditorModel(Ref<SOULSound> graphAsset) : mGraphAsset(graphAsset) { Deserialize(); }
        ~SOULNodeEditorModel() = default;

        std::vector<Node>& GetNodes() override { return mGraphAsset->Nodes; }
        std::vector<Link>& GetLinks() override { return mGraphAsset->Links; }
        const std::vector<Node>& GetNodes() const override { return mGraphAsset->Nodes; }
        const std::vector<Link>& GetLinks() const override { return mGraphAsset->Links; }

        Utils::PropertySet& GetInputs() { return mGraphAsset->GraphInputs; }
        Utils::PropertySet& GetOutputs() { return mGraphAsset->GraphOutputs; }

        const Nodes::Registry& GetNodeTypes() const override { return mNodeFactory.Registry; }

        void AddInputToGraph(const choc::value::ValueView& value);
        void RemoveInputFromGraph(const std::string& name);
        void OnGraphInputNameChanged(const std::string& oldName, const std::string& newName);
        void OnGraphInputTypeChanged(const std::string& inputName);
        void OnGraphInputValueChanged(const std::string& inputName);

        Node* SpawnGraphInputNode(const std::string& inputName);

    private:
        void Serialize() override { AssetImporter::Serialize(mGraphAsset); };
        void Deserialize() override { BuildNodes(); };

        bool SaveGraphState(const char* data, size_t size) override;
        const std::string& LoadGraphState() override;

        Nodes::AbstractFactory* GetNodeFactory() override { return &mNodeFactory; }

        void OnCompileGraph() override;

    public:
        std::function<void()> onCompiledSuccessfully = nullptr;

    private:
        Nodes::SOULFactory mNodeFactory;
        Ref<SOULSound> mGraphAsset = nullptr;
    };

} // namespace NR