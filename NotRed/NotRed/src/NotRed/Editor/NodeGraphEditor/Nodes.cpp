#include <nrpch.h>
#include "Nodes.h"

#include "choc/text/choc_StringUtilities.h"

#define NODE_CATEGORY(name) std::string { #name }
#define DECLARE_NODE(category, name) {#name, category::name}

namespace NR::Nodes
{
	//==========================================================================
	/// Demo Nodes
	class DemoNodes
	{
	public:
		static Node* Some_Float()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str());
			node->Category = "DemoNodes";

			node->Type = NodeType::Blueprint;
			node->Size = ImVec2(300, 200);

			node->Inputs.emplace_back(0, "In", PinType::Float);
			node->Inputs.emplace_back(0, "In", PinType::Float);
			node->Outputs.emplace_back(0, "Result", PinType::Float);

			return node;
		}

		static Node* All_Pin_Types()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str());
			node->Category = "DemoNodes";

			node->Type = NodeType::Blueprint;
			node->Size = ImVec2(300, 200);

			node->Inputs.emplace_back(0, "Flow In", PinType::Flow);
			node->Inputs.emplace_back(0, "Function In", PinType::Function);
			node->Inputs.emplace_back(0, "Float In", PinType::Float);
			node->Inputs.emplace_back(0, "Int In", PinType::Int);
			node->Inputs.emplace_back(0, "Bool In", PinType::Bool);
			node->Inputs.emplace_back(0, "String In", PinType::String);
			node->Inputs.emplace_back(0, "Object In", PinType::Object);

			node->Outputs.emplace_back(0, "Flow Out", PinType::Flow);
			node->Outputs.emplace_back(0, "Function Out", PinType::Function);
			node->Outputs.emplace_back(0, "Float Out", PinType::Float);
			node->Outputs.emplace_back(0, "Int Out", PinType::Int);
			node->Outputs.emplace_back(0, "Bool Out", PinType::Bool);
			node->Outputs.emplace_back(0, "String Out", PinType::String);
			node->Outputs.emplace_back(0, "Object Out", PinType::Object);

			return node;
		}

		static Node* All_Pin_Type_Arrays()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str());
			node->Category = "DemoNodes";

			node->Type = NodeType::Blueprint;
			node->Size = ImVec2(300, 200);

			node->Inputs.emplace_back(0, "Flow In", PinType::Flow, StorageKind::Array);
			node->Inputs.emplace_back(0, "Function In", PinType::Function, StorageKind::Array);
			node->Inputs.emplace_back(0, "Float In", PinType::Float, StorageKind::Array);
			node->Inputs.emplace_back(0, "Int In", PinType::Int, StorageKind::Array);
			node->Inputs.emplace_back(0, "Bool In", PinType::Bool, StorageKind::Array);
			node->Inputs.emplace_back(0, "String In", PinType::String, StorageKind::Array);
			node->Inputs.emplace_back(0, "Object In", PinType::Object, StorageKind::Array);

			node->Outputs.emplace_back(0, "Flow Out", PinType::Flow, StorageKind::Array);
			node->Outputs.emplace_back(0, "Function Out", PinType::Function, StorageKind::Array);
			node->Outputs.emplace_back(0, "Float Out", PinType::Float, StorageKind::Array);
			node->Outputs.emplace_back(0, "Int Out", PinType::Int, StorageKind::Array);
			node->Outputs.emplace_back(0, "Bool Out", PinType::Bool, StorageKind::Array);
			node->Outputs.emplace_back(0, "String Out", PinType::String, StorageKind::Array);
			node->Outputs.emplace_back(0, "Object Out", PinType::Object, StorageKind::Array);

			return node;
		}

		static Node* Simple_All_Pin_Types()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str());
			node->Category = "DemoNodes";

			node->Type = NodeType::Simple;
			node->Size = ImVec2(300, 200);

			node->Inputs.emplace_back(0, "Flow In", PinType::Flow);
			node->Inputs.emplace_back(0, "Function In", PinType::Function);
			node->Inputs.emplace_back(0, "Float In", PinType::Float);
			node->Inputs.emplace_back(0, "Int In", PinType::Int);
			node->Inputs.emplace_back(0, "Bool In", PinType::Bool);
			node->Inputs.emplace_back(0, "String In", PinType::String);
			node->Inputs.emplace_back(0, "Object In", PinType::Object);

			node->Outputs.emplace_back(0, "Flow Out", PinType::Flow);
			node->Outputs.emplace_back(0, "Function Out", PinType::Function);
			node->Outputs.emplace_back(0, "Float Out", PinType::Float);
			node->Outputs.emplace_back(0, "Int Out", PinType::Int);
			node->Outputs.emplace_back(0, "Bool Out", PinType::Bool);
			node->Outputs.emplace_back(0, "String Out", PinType::String);
			node->Outputs.emplace_back(0, "Object Out", PinType::Object);

			return node;
		}

		static Node* Simple_All_Pin_Type_Arrays()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str());
			node->Category = "DemoNodes";

			node->Type = NodeType::Simple;
			node->Size = ImVec2(300, 200);

			node->Inputs.emplace_back(0, "Flow In", PinType::Flow, StorageKind::Array);
			node->Inputs.emplace_back(0, "Function In", PinType::Function, StorageKind::Array);
			node->Inputs.emplace_back(0, "Float In", PinType::Float, StorageKind::Array);
			node->Inputs.emplace_back(0, "Int In", PinType::Int, StorageKind::Array);
			node->Inputs.emplace_back(0, "Bool In", PinType::Bool, StorageKind::Array);
			node->Inputs.emplace_back(0, "String In", PinType::String, StorageKind::Array);
			node->Inputs.emplace_back(0, "Object In", PinType::Object, StorageKind::Array);

			node->Outputs.emplace_back(0, "Flow Out", PinType::Flow, StorageKind::Array);
			node->Outputs.emplace_back(0, "Function Out", PinType::Function, StorageKind::Array);
			node->Outputs.emplace_back(0, "Float Out", PinType::Float, StorageKind::Array);
			node->Outputs.emplace_back(0, "Int Out", PinType::Int, StorageKind::Array);
			node->Outputs.emplace_back(0, "Bool Out", PinType::Bool, StorageKind::Array);
			node->Outputs.emplace_back(0, "String Out", PinType::String, StorageKind::Array);
			node->Outputs.emplace_back(0, "Object Out", PinType::Object, StorageKind::Array);

			return node;
		}
	};

	//==========================================================================
	/// Utility Nodes
	class Utility
	{
	public:
		static Node* Comment()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str());
			node->Category = "Utility";

			node->Type = NodeType::Comment;
			node->Size = ImVec2(300, 200);

			return node;
		}

		static Node* Message()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(128, 195, 248));
			node->Category = "Utility";
			node->Type = NodeType::Simple;
			node->Outputs.emplace_back(0, "Message", PinType::String);

			return node;
		}

		static Node* Dummy_Node()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(128, 195, 248));
			node->Category = "Utility";
			node->Type = NodeType::Blueprint;
			node->Inputs.emplace_back(0, "Message", PinType::String);
			node->Outputs.emplace_back(0, "Message", PinType::String);

			return node;
		}
	};

	Node* Nodes::DemoFactory::SpawnNodeStatic(const std::string& category, const std::string& type)
	{
		auto cat = Registry.find(category);
		if (cat != Registry.end())
		{
			auto nodes = cat->second.find(type);
			if (nodes != cat->second.end())
			{
				return nodes->second();
			}
			else
			{
				NR_CORE_ERROR("SpawnNodeStatic() - Category {0} does not contain node type {1}", category, type);
				return nullptr;
			}
		}

		NR_CORE_ERROR("SpawnNodeStatic() - Category {0} does not exist", category);
		return nullptr;
	}

	const std::map<std::string, std::map<std::string, std::function<Node* ()>>> DemoFactory::Registry = { {
			NODE_CATEGORY(Utility),	{
										DECLARE_NODE(Utility, Comment),
										DECLARE_NODE(Utility, Message),
										DECLARE_NODE(Utility, Dummy_Node)
									}}, {
			NODE_CATEGORY(DemoNodes), {
										DECLARE_NODE(DemoNodes, Some_Float),
										DECLARE_NODE(DemoNodes, All_Pin_Types),
										DECLARE_NODE(DemoNodes, All_Pin_Type_Arrays),
										DECLARE_NODE(DemoNodes, Simple_All_Pin_Types),
										DECLARE_NODE(DemoNodes, Simple_All_Pin_Type_Arrays)
								   }}
	};
}