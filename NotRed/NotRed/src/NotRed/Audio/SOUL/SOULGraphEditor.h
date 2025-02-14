#pragma once

#include "NotRed/Editor/NodeGraphEditor/NodeGraphEditor.h"

namespace NR
{
	//==================================================================================
	/// Demo Node Editor
	class SOULNodeEditorModel;

	class SOULNodeGraphEditor final : public NodeGraphEditorBase
	{
	public:
		SOULNodeGraphEditor();
		~SOULNodeGraphEditor();

		void SetAsset(const Ref<Asset>& asset) override;

		static ImColor GetTypeColor(const choc::value::Value& v);
		static std::pair<PinType, StorageKind> GetPinTypeAndStorageKindForValue(const choc::value::Value& v);

	private:
		ImGuiWindowFlags GetWindowFlags() override;
		void OnRender() override;

		const char* GetIconForSimpleNode(const std::string& simpleNodeIdentifier) const override;

		void DrawToolbar();
		void DrawDetailsPanel();
		void DrawGraphIO();

		NodeEditorModel* GetModel() override;

	private:
		std::unique_ptr<SOULNodeEditorModel> mModel = nullptr;

		Ref<Texture2D> mIconPlay;
		Ref<Texture2D> mIconStop;

		bool mGraphDirty = false;
		std::string mSelectedInputOutput;
	};
}