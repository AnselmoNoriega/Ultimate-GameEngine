#include <nrpch.h>
#include "Nodes.h"

#include "choc/text/choc_StringUtilities.h"

#define NODE_CATEGORY(name) std::string { #name }
#define DECLARE_NODE(category, name) {#name, category::name}

namespace NR::Nodes
{
	//==========================================================================
	/// Base Nodes
	class Base
	{
	public:
		static Node* Input_Action()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(255, 128, 128));
			node->Category = "Base";

			node->Outputs.emplace_back(0, "Play", PinType::Function);
			node->Outputs.emplace_back(0, "Stop", PinType::Function);

			return node;
		}

		static Node* Output_Audio()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(255, 128, 128));
			node->Category = "Base";

			node->Inputs.emplace_back(0, "Left", PinType::Audio);
			node->Inputs.emplace_back(0, "Right", PinType::Audio);

			return node;
		}

		static Node* Wave_Player()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(54, 207, 145));
			node->Category = "Base";

			node->Inputs.emplace_back(0, "Play", PinType::Function);
			node->Inputs.emplace_back(0, "Stop", PinType::Function);
			node->Inputs.emplace_back(0, "Wave Asset", PinType::Object);
			node->Inputs.emplace_back(0, "Start Time", PinType::Float, StorageKind::Value, choc::value::createFloat32(0.0f));
			node->Inputs.emplace_back(0, "Loop", PinType::Bool, StorageKind::Value, choc::value::createBool(false));
			node->Inputs.emplace_back(0, "Number of Loops", PinType::Int, StorageKind::Value, choc::value::createFloat32(-1.0f));

			node->Outputs.emplace_back(0, "On Play", PinType::Function);
			node->Outputs.emplace_back(0, "On Finish", PinType::Function);
			node->Outputs.emplace_back(0, "On Looped", PinType::Function);
			node->Outputs.emplace_back(0, "Playback Position", PinType::Float);
			node->Outputs.emplace_back(0, "Out Left", PinType::Audio);
			node->Outputs.emplace_back(0, "Out Right", PinType::Audio);

			return node;
		}

		static Node* LessThan()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(128, 195, 248));
			node->Category = "Base";

			node->Type = NodeType::Simple;
			node->Inputs.emplace_back(0, "Left", PinType::Float, StorageKind::Value, choc::value::createFloat32(0.0f));
			node->Inputs.emplace_back(0, "Right", PinType::Float, StorageKind::Value, choc::value::createFloat32(0.0f));
			node->Outputs.emplace_back(0, "Result", PinType::Float);

			return node;
		}
	};

	//==========================================================================
	// Math Nodes
	class Math
	{
	public:
		//static Node* Add()
		//{
		//	const std::string nodeName = choc::text::replace(__func__, "_", " ");

		//	auto* node = new Node();
		//	return node;
		//}

		//static Node* Subtract()
		//{
		//	const std::string nodeName = choc::text::replace(__func__, "_", " ");

		//	auto* node = new Node();
		//	return node;
		//}

		static Node* Multiply_Audio_Float()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(128, 195, 248));
			node->Category = "Math";

			node->Type = NodeType::Simple;
			node->Inputs.emplace_back(0, "Audio In", PinType::Audio, StorageKind::Value);
			node->Inputs.emplace_back(0, "Float In", PinType::Float, StorageKind::Value, choc::value::createFloat32(1.0f));
			node->Outputs.emplace_back(0, "Audio Out", PinType::Audio);

			return node;
		}

		static Node* Multiply_Audio_Audio()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(128, 195, 248));
			node->Category = "Math";

			node->Type = NodeType::Simple;
			node->Inputs.emplace_back(0, "Audio In", PinType::Audio, StorageKind::Value);
			node->Inputs.emplace_back(0, "Audio In 2", PinType::Audio, StorageKind::Value);
			node->Outputs.emplace_back(0, "Audio Out", PinType::Audio);

			return node;
		}

		static Node* Increment_Int()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(128, 195, 248));
			node->Category = "Math";

			node->Type = NodeType::Blueprint;
			node->Inputs.emplace_back(0, "Trigger", PinType::Function);
			node->Inputs.emplace_back(0, "Reset", PinType::Function);
			node->Inputs.emplace_back(0, "In", PinType::Int);
			node->Outputs.emplace_back(0, "On Trigger", PinType::Function);
			node->Outputs.emplace_back(0, "Out", PinType::Int);

			return node;
		}
	};


	//==========================================================================
	/// Array Nodes
	class Array
	{
	public:
		static Node* Get_Float()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(147, 226, 74));
			node->Category = "Array";

			node->Inputs.emplace_back(0, "Trigger", PinType::Function, StorageKind::Value);
			node->Inputs.emplace_back(0, "In Array", PinType::Float, StorageKind::Array);
			node->Inputs.emplace_back(0, "Index", PinType::Int, StorageKind::Value, choc::value::createInt32(0));
			node->Outputs.emplace_back(0, "Element", PinType::Float);

			return node;
		}

		static Node* Get_String()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(194, 75, 227));
			node->Category = "Array";

			node->Inputs.emplace_back(0, "Trigger", PinType::Function, StorageKind::Value);
			node->Inputs.emplace_back(0, "In Array", PinType::String, StorageKind::Array);
			node->Inputs.emplace_back(0, "Index", PinType::Int, StorageKind::Value, choc::value::createInt32(0));
			node->Outputs.emplace_back(0, "On Trigger", PinType::Function);
			node->Outputs.emplace_back(0, "Element", PinType::String);

			return node;
		}

		static Node* Get_Random_Float()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(147, 226, 74));
			node->Category = "Array";

			node->Inputs.emplace_back(0, "Generate", PinType::Function, StorageKind::Value);
			node->Inputs.emplace_back(0, "In Array", PinType::Float, StorageKind::Array);
			node->Outputs.emplace_back(0, "On Generate", PinType::Function);
			node->Outputs.emplace_back(0, "Out", PinType::Float);

			return node;
		}

		static Node* Get_Random_String()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new Node(0, nodeName.c_str(), ImColor(194, 75, 227));
			node->Category = "Array";

			node->Inputs.emplace_back(0, "Generate", PinType::Function, StorageKind::Value);
			node->Inputs.emplace_back(0, "In Array", PinType::String, StorageKind::Array);
			node->Outputs.emplace_back(0, "On Generate", PinType::Function);
			node->Outputs.emplace_back(0, "Out", PinType::String);

			return node;
		}
	};

	Node* SOULFactory::SpawnNodeStatic(const std::string& category, const std::string& type)
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

	const std::map<std::string, std::map<std::string, std::function<Node* ()>>> SOULFactory::Registry =
	{
				{ NODE_CATEGORY(Base), {
											DECLARE_NODE(Base, Input_Action),
											DECLARE_NODE(Base, Output_Audio),
											DECLARE_NODE(Base, Wave_Player),
											DECLARE_NODE(Base, LessThan)
										}},
				{ NODE_CATEGORY(Math), {
			//DECLARE_NODE(Math, Add),
			//DECLARE_NODE(Math, Subtract),
			DECLARE_NODE(Math, Multiply_Audio_Float),
			DECLARE_NODE(Math, Multiply_Audio_Audio),
			DECLARE_NODE(Math, Increment_Int)
		}},
{ NODE_CATEGORY(Array), {
							DECLARE_NODE(Array, Get_Float),
							DECLARE_NODE(Array, Get_String),
							DECLARE_NODE(Array, Get_Random_Float),
							DECLARE_NODE(Array, Get_Random_String)
						}}
	};

}