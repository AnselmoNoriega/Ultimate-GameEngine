#pragma once

#include "NotRed/Core/Events/KeyEvent.h"
#include "NotRed/Audio/Editor/OrderedVector.h"
#include "TreeModel.h"

#include "NotRed/Audio/AudioEvents/CommandID.h"

struct ImGuiWindow;

namespace NR
{
	class Texture2D;
}
namespace YAML
{
	class Emitter;
	class Node;
}

namespace NR
{
	namespace Audio
	{
		struct TriggerCommand;
	}

	class AudioEventsEditor
	{
	public:
		using CommandsTree = TreeModel<Audio::CommandID>;

		static void OnImGuiRender(bool& show);
		static bool OnKeyPressedEvent(KeyPressedEvent& e);

		static void SerializeTree(YAML::Emitter& out);
		static void DeserializeTree(std::vector<YAML::Node>& data);

	private:
		static void OnOpennesChange(bool opened);

		static void DrawEventsList();
		static void DrawEventDetails();
		static void DrawFolderDetails();
		static void DrawSelectionDetails();

		static void DrawTreeNode(CommandsTree::Node* treeNode, bool isRootNode = false);

		static void OnSelectionChanged(Audio::CommandID commandID);
		static void OnDeleteNode(CommandsTree::Node* treeNode);

	private:
		static std::string GetUniqueName();
		static std::string GetUniqueFolderName(CommandsTree::Node* parentNode, const std::string& nameBase);

		static void ActivateRename(CommandsTree::Node* treeNode);
		static void OnEntryRenamed(CommandsTree::Node* treeNode, const char* newName);
		static std::optional<std::string> CheckNameConflict(const std::string& name, CommandsTree::Node* parentNode,
			std::function<void()> onReplaceDestination, std::function<void()> onCancel = nullptr);
		static bool CheckAndResolveNameConflict(CommandsTree::Node* newParent, CommandsTree::Node* node, CommandsTree::Node* oldParent);

		static void OnRegistryChanged();

	private:
		static bool sopen;
		static CommandsTree::Node* sNewlyCreatedNode;
		static CommandsTree::Node* sRenamingEntry;
		static OrderedVector<CommandsTree::Node*> sSelection;

		static std::vector<CommandsTree::Node*> sMarkedForDelete;

		struct ReorderData
		{
			Ref<CommandsTree::Node> Node;
			CommandsTree::Node* NewParent;
		};
		static std::vector<ReorderData> sMarkedForReorder;

		static CommandsTree sTree;

		static ImGuiWindow* sWindowHandle;

		// Flags
		static bool f_Renaming;
		static std::string sRenameConflict;

		static Ref<Texture2D> sFolderIcon;
		static Ref<Texture2D> sMoveIcon;
		static Ref<Texture2D> sDeleteIcon;
	};


} // namespace Hazel