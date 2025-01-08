#include <nrpch.h>
#include "GraphSerializer.h"

#include "NotRed/Editor/NodeGraphEditor/NodeGraphAsset.h"
#include "NotRed/Asset/AssetManager.h"

#include "NotRed/Util/YAMLSerializationHelpers.h"
#include "NotRed/Util/SerializationMacros.h"

#include "choc/text/choc_JSON.h"
#include "yaml-cpp/yaml.h"

namespace NR
{
	static std::string PinTypeToString(PinType pinType)
	{
		switch (pinType)
		{
		default:
		case PinType::Flow:				return "Flow";
		case PinType::Bool:				return "Bool";
		case PinType::Int:				return "Int";
		case PinType::Float:			return "Float";
		case PinType::Audio:			return "Audio";
		case PinType::String:			return "String";
		case PinType::Object:			return "Object";
		case PinType::Function:			return "Function";
		case PinType::Delegate:			return "Delegate";
		}
	}

	static PinType PinTypeFromString(const std::string_view& pinTypeStr)
	{
		if (pinTypeStr == "Flow")		return PinType::Flow;
		if (pinTypeStr == "Bool")		return PinType::Bool;
		if (pinTypeStr == "Int")		return PinType::Int;
		if (pinTypeStr == "Float")		return PinType::Float;
		if (pinTypeStr == "Audio")		return PinType::Audio;
		if (pinTypeStr == "String")		return PinType::String;
		if (pinTypeStr == "Object")		return PinType::Object;
		if (pinTypeStr == "Function")	return PinType::Function;
		if (pinTypeStr == "Delegate")	return PinType::Delegate;
	}

	static std::string StorageKindToString(StorageKind storageKind)
	{
		switch (storageKind)
		{
		default:
		case StorageKind::Value:    return "Value";
		case StorageKind::Array:    return "Array";
		}
	}

	static StorageKind StorageKindFromString(const std::string_view& nodeTypeStr)
	{
		if (nodeTypeStr == "Value") return StorageKind::Value;
		if (nodeTypeStr == "Array") return StorageKind::Array;
	}

	// Defines how a node is drawn
	static std::string NodeTypeToString(NodeType nodeType)
	{
		switch (nodeType)
		{
		default:
		case NodeType::Simple:    return "Simple";
		case NodeType::Blueprint: return "Blueprint";
		case NodeType::Comment:   return "Comment";
		}
	}

	static NodeType NodeTypeFromString(const std::string_view& nodeTypeStr)
	{
		if (nodeTypeStr == "Simple")    return NodeType::Simple;
		if (nodeTypeStr == "Blueprint") return NodeType::Blueprint;
		if (nodeTypeStr == "Comment")   return NodeType::Comment;
	}

	void DefaultGraphSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<DemoGraph> graph = asset.As<DemoGraph>();

		YAML::Emitter out;

		//============================================================
		/// Nodes

		out << YAML::BeginMap; // Nodes & Links

		out << YAML::Key << "Nodes" << YAML::Value;
		out << YAML::BeginSeq;
		for (auto& node : graph->Nodes)
		{
			out << YAML::BeginMap; // node

			const ImVec4& nodeCol = node.Color.Value;
			const ImVec2& nodeSize = node.Size;
			const glm::vec4 nodeColOut(nodeCol.x, nodeCol.y, nodeCol.z, nodeCol.w);
			const glm::vec2 nodeSizeOut(nodeSize.x, nodeSize.y);

			NR_SERIALIZE_PROPERTY(ID, node.ID, out);
			NR_SERIALIZE_PROPERTY(Category, node.Category, out);
			NR_SERIALIZE_PROPERTY(Name, node.Name, out);
			NR_SERIALIZE_PROPERTY(Colour, nodeColOut, out);
			NR_SERIALIZE_PROPERTY(Type, NodeTypeToString(node.Type), out);
			NR_SERIALIZE_PROPERTY(Size, nodeSizeOut, out);
			NR_SERIALIZE_PROPERTY(Location, node.State, out);

			out << YAML::Key << "Inputs" << YAML::BeginSeq;
			for (auto& in : node.Inputs)
			{
				out << YAML::BeginMap; // in
				NR_SERIALIZE_PROPERTY(ID, in.ID, out);
				NR_SERIALIZE_PROPERTY(Name, in.Name, out);
				NR_SERIALIZE_PROPERTY(Type, PinTypeToString(in.Type), out);
				NR_SERIALIZE_PROPERTY(Storage, StorageKindToString(in.Storage), out);
				NR_SERIALIZE_PROPERTY(Value, choc::json::toString(in.Value), out);
				out << YAML::EndMap; // in
			}
			out << YAML::EndSeq; // Inputs

			out << YAML::Key << "Outputs" << YAML::BeginSeq;
			for (auto& outp : node.Outputs)
			{
				out << YAML::BeginMap; // outp
				NR_SERIALIZE_PROPERTY(ID, outp.ID, out);
				NR_SERIALIZE_PROPERTY(Name, outp.Name, out);
				NR_SERIALIZE_PROPERTY(Type, PinTypeToString(outp.Type), out);
				NR_SERIALIZE_PROPERTY(Storage, StorageKindToString(outp.Storage), out);
				NR_SERIALIZE_PROPERTY(Value, choc::json::toString(outp.Value), out);
				out << YAML::EndMap; // outp
			}
			out << YAML::EndSeq; // Outputs

			out << YAML::EndMap; // node
		}
		out << YAML::EndSeq; // Nodes

		//============================================================
		/// Links

		out << YAML::Key << "Links" << YAML::Value;
		out << YAML::BeginSeq;
		for (auto& link : graph->Links)
		{
			out << YAML::BeginMap; // link

			const auto& col = link.Color.Value;
			const glm::vec4 colOut(col.x, col.y, col.z, col.w);

			NR_SERIALIZE_PROPERTY(ID, link.ID, out);
			NR_SERIALIZE_PROPERTY(StartPinID, link.StartPinID, out);
			NR_SERIALIZE_PROPERTY(EndPinID, link.EndPinID, out);
			NR_SERIALIZE_PROPERTY(Colour, colOut, out);

			out << YAML::EndMap; // link
		}
		out << YAML::EndSeq; // Links

		out << YAML::EndMap; // Nodes & Links

		// Out
		std::ofstream fout(AssetManager::GetFileSystemPath(metadata));
		fout << out.c_str();
	}

	bool DefaultGraphSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(AssetManager::GetFileSystemPath(metadata));
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());

		auto graph = Ref<DemoGraph>::Create();

		for (const auto& node : data["Nodes"])
		{
			UUID nodeID;
			std::string nodeCategory;
			std::string nodeName;
			std::string location;
			std::string nodeType;
			glm::vec4 nodeCol;
			glm::vec2 nodeS;

			NR_DESERIALIZE_PROPERTY(ID, nodeID, node, uint64_t(0));
			NR_DESERIALIZE_PROPERTY(Category, nodeCategory, node, std::string());
			NR_DESERIALIZE_PROPERTY(Name, nodeName, node, std::string());
			NR_DESERIALIZE_PROPERTY(Colour, nodeCol, node, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			NR_DESERIALIZE_PROPERTY(Type, nodeType, node, std::string());
			NR_DESERIALIZE_PROPERTY(Size, nodeS, node, glm::vec2());
			NR_DESERIALIZE_PROPERTY(Location, location, node, std::string());

			auto& newNode = graph->Nodes.emplace_back(nodeID, nodeName.c_str());
			newNode.Category = nodeCategory;
			newNode.State = location;
			newNode.Color = ImColor(nodeCol.x, nodeCol.y, nodeCol.z, nodeCol.w);
			newNode.Type = NodeTypeFromString(nodeType);
			newNode.Size = ImVec2(nodeS.x, nodeS.y);

			if (node["Inputs"])
			{
				for (const auto& in : node["Inputs"])
				{
					UUID ID;
					std::string pinName;
					std::string valueStr;
					std::string pinType;
					std::string pinStorage;

					NR_DESERIALIZE_PROPERTY(ID, ID, in, uint64_t(0));
					NR_DESERIALIZE_PROPERTY(Name, pinName, in, std::string());
					NR_DESERIALIZE_PROPERTY(Type, pinType, in, std::string());
					NR_DESERIALIZE_PROPERTY(Storage, pinStorage, in, std::string());
					NR_DESERIALIZE_PROPERTY(Value, valueStr, in, std::string());

					bool isCustomValueType = choc::text::contains(valueStr, "Value");

					auto parseCustomValueType = [](const std::string& valueString) -> choc::value::Value
						{
							choc::value::Value value = choc::json::parse(valueString);

							if (value["TypeName"].isVoid())
							{
								NR_CORE_ASSERT(false, "Failed to deserialize custom value type, missing \"TypeName\" property.");
								return {};
							}

							choc::value::Value customObject = choc::value::createObject(value["TypeName"].get<std::string>());
							if (value.isObject())
							{
								for (uint32_t i = 0; i < value.size(); i++)
								{
									choc::value::MemberNameAndValue nameValue = value.getObjectMemberAt(i);
									customObject.addMember(nameValue.name, nameValue.value);
								}
							}
							else
							{
								NR_CORE_ASSERT(false, "Failed to load custom value type. It must be serialized as object.")
							}

							return customObject;
						};

					newNode.Inputs.emplace_back(ID, pinName.c_str(),
						PinTypeFromString(pinType),
						StorageKindFromString(pinStorage),
						isCustomValueType ? parseCustomValueType(valueStr) : choc::json::parseValue(valueStr))
						.Kind = PinKind::Input;
				}
			}

			if (node["Outputs"])
			{
				for (const auto& out : node["Outputs"])
				{
					UUID ID;
					std::string pinName;
					std::string valueStr;
					std::string pinType;
					std::string pinStorage;

					NR_DESERIALIZE_PROPERTY(ID, ID, out, uint64_t(0));
					NR_DESERIALIZE_PROPERTY(Name, pinName, out, std::string());
					NR_DESERIALIZE_PROPERTY(Type, pinType, out, std::string());
					NR_DESERIALIZE_PROPERTY(Storage, pinStorage, out, std::string());
					NR_DESERIALIZE_PROPERTY(Value, valueStr, out, std::string());

					newNode.Outputs.emplace_back(ID, pinName.c_str(),
						PinTypeFromString(pinType),
						StorageKindFromString(pinStorage),
						choc::json::parseValue(valueStr))
						.Kind = PinKind::Output;
				}
			}
		}

		for (const auto& link : data["Links"])
		{
			UUID ID;
			UUID StartPinID;
			UUID EndPinID;
			glm::vec4 colour;

			NR_DESERIALIZE_PROPERTY(ID, ID, link, uint64_t(0));
			NR_DESERIALIZE_PROPERTY(StartPinID, StartPinID, link, uint64_t(0));
			NR_DESERIALIZE_PROPERTY(EndPinID, EndPinID, link, uint64_t(0));
			NR_DESERIALIZE_PROPERTY(Colour, colour, link, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

			graph->Links.emplace_back(ID, StartPinID, EndPinID)
				.Color = ImColor(colour.x, colour.y, colour.z, colour.w);
		}

		asset = graph;
		asset->Handle = metadata.Handle;
		return true;
	}

} // namespace