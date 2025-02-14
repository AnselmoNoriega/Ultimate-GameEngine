#pragma once

#include "NotRed/Editor/AssetEditorPanel.h"

#include "Nodes.h"

namespace ax::NodeEditor
{
	struct Style;
	struct EditorContext;

	namespace Utilities
	{
		struct BlueprintNodeBuilder;
	}
}

namespace NR
{
	class NodeEditorModel;

	class NodeGraphEditorBase : public AssetEditor
	{
	public:
		NodeGraphEditorBase();
		virtual ~NodeGraphEditorBase() = default;

		// If you override this, make sure to call base class method from subclass.
		void SetAsset(const Ref<Asset>& asset) override;
		Ref<Asset> GetAsset() { return mGraphAsset; }

		void Update(float dt) override {}
		void OnEvent(Event& e) override {}

	private:
		bool InitializeEditor();

		void Render() override final;

		// Called after drawing main Canvas
		virtual void OnRender() {};

		virtual void DrawPinIcon(const Pin& pin, bool connected, int alpha);
		virtual void DrawNodes();

		virtual void DrawNodeContextMenu(Node* node);
		virtual void DrawPinContextMenu(Pin* pin);
		virtual void DrawLinkContextMenu(Link* link);

		virtual NodeEditorModel* GetModel() = 0;
		virtual const char* GetIconForSimpleNode(const std::string& simpleNodeIdentifier) const { return nullptr; }

	protected:
		// If you override this, make sure to call base class method from subclass.
		void Close() override;

		virtual void InitializeEditorStyle(ax::NodeEditor::Style* style);
		virtual int GetPinIconSize() { return 24; }

		bool IsPinAcceptingLink(const Pin& pin) { return mAcceptingLinkPins.first == &pin || mAcceptingLinkPins.second == &pin; }

		ImGuiWindowClass* GetWindowClass() { return &mWindowClass; }
		void EnsureWindowIsDocked(ImGuiWindow* childWindow);

	protected:
		ax::NodeEditor::EditorContext* mEditor = nullptr;
		bool mInitialized = false;

		Ref<Asset> mGraphAsset = nullptr;

		std::function<Node* (bool searching, std::string_view searchedString)> onNodeListPopup = nullptr;

		// A pair of nodes that are accepting a link connection in this frame
		std::pair<Pin*, Pin*> mAcceptingLinkPins{ nullptr, nullptr };

		// Render local states
		struct ContextState;
		std::unique_ptr<ContextState> mState = nullptr;

		Ref<Texture2D> mHeaderBackground;
		Ref<Texture2D> mShadow;
		Ref<Texture2D> mSaveIcon;
		Ref<Texture2D> mCompileIcon;

		Ref<Texture2D> mSearchIcon;

		Ref<Texture2D> mIconPin_V_C;
		Ref<Texture2D> mIconPin_V_D;
		Ref<Texture2D> mIconPin_F_C;
		Ref<Texture2D> mIconPin_F_D;
		Ref<Texture2D> mIconPin_A_C;
		Ref<Texture2D> mIconPin_A_D;

		// For debugging
		bool mShowNodeIDs = false;

	private:
		ImGuiWindowClass mWindowClass;
		ImGuiID mMainDockID;
		std::unordered_map<ImGuiID, ImGuiID> mDockIDs;

		std::string mGraphStatePath;
	};


	//==================================================================================
	/// Demo Node Editor
	class DemoNodeEditorModel;

	class DemoNodeGraphEditor final : public NodeGraphEditorBase
	{
	public:
		DemoNodeGraphEditor();
		~DemoNodeGraphEditor();

		void SetAsset(const Ref<Asset>& asset) override;

	private:
		ImGuiWindowFlags GetWindowFlags() override;
		void OnRender() override;

		const char* GetIconForSimpleNode(const std::string& simpleNodeIdentifier) const override;

		void DrawToolbar();
		void DrawDetailsPanel();

		NodeEditorModel* GetModel() override;

	private:
		std::unique_ptr<DemoNodeEditorModel> mModel = nullptr;
	};
}
