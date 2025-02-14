#pragma once

#include <map>
#include <imgui.h>

#include "NotRed/Core/UUID.h"

#include "choc/containers/choc_Value.h"

namespace NR
{
	enum class ValueType
	{
		Bool,
		Int,
		Float,
		Audio,
		String,
		WaveRef,
		Function
	};

	//! Don't forget to add to/from string functions in GraphSerializer.cpp
	enum class PinType
	{
		Flow,
		Bool,
		Int,
		Float,
		Audio,
		String,
		Object,
		Function,
		Delegate
	};

	enum class PinKind
	{
		Output,
		Input
	};

	enum class StorageKind
	{
		Value,
		Array
	};

	//! Don't forget to add to/from string functions in GraphSerializer.cpp
	enum class NodeType
	{
		Blueprint,
		Simple,
		Comment
	};

	//! Set colour for your pin / value types
	inline ImColor GetIconColor(PinType type)
	{
		switch (type)
		{
		default:
		case PinType::Flow:     return ImColor(200, 200, 200);
		case PinType::Bool:     return ImColor(220, 48, 48);
		case PinType::Int:      return ImColor(68, 201, 156);
		case PinType::Float:    return ImColor(147, 226, 74);
		case PinType::Audio:    return ImColor(102, 204, 163);
		case PinType::String:   return ImColor(194, 75, 227);
		case PinType::Object:   return ImColor(51, 150, 215);
		case PinType::Function: return ImColor(200, 200, 200);
		case PinType::Delegate: return ImColor(255, 48, 48);
		}
	};

	static choc::value::Value CreateAssetRefObject(uint64_t handle = 0)
	{
		return choc::value::createObject("AssetHandle",
			"TypeName", "AssetHandle",
			"Value", std::to_string(handle));
	}

	//=================================================================
	struct Node;

	struct Pin
	{
		UUID ID;
		Node* Node;
		std::string Name;
		PinType Type;
		PinKind Kind;
		StorageKind Storage;

		choc::value::Value Value;

		Pin(UUID id, const char* name, PinType type, StorageKind storageKind = StorageKind::Value, choc::value::Value defaultValue = choc::value::Value()) :
			ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input), Storage(storageKind), Value(defaultValue)
		{ }
	};

	struct Node
	{
		UUID ID;
		std::string Category, Name;
		std::vector<Pin> Inputs;
		std::vector<Pin> Outputs;
		ImColor Color;
		NodeType Type;
		ImVec2 Size;

		std::string State;
		std::string SavedState;

		Node(UUID id, const char* name, ImColor color = ImColor(255, 255, 255)) :
			ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
		{ }
	};

	struct Link
	{
		UUID ID;

		UUID StartPinID;
		UUID EndPinID;

		ImColor Color;

		Link(UUID id, UUID startPinId, UUID endPinId) :
			ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
		{
		}
	};

	//=================================================================
	/// Node factories

	namespace Nodes
	{
		using Registry = std::map<std::string, std::map<std::string, std::function<Node* ()>>>;

		class AbstractFactory
		{
		public:
			virtual ~AbstractFactory() = default;

			virtual Node* SpawnNode(const std::string& category, const std::string& type) = 0;
		};

		template<class T>
		class Factory : public AbstractFactory
		{
		public:
			static const Registry Registry;

			template<class T>
			[[nodiscard]] static Node* SpawnNodeStatic(const std::string& category, const std::string& type)
			{
				return T::SpawnNodeStatic();
			};
		};

		//! Create your own Factory subclass if you need a different selection of nodes for your custom graph
		//! Create and register custom node spawn functions in .cpp file
		class DemoFactory : public Factory<DemoFactory>
		{
			[[nodiscard]] static Node* SpawnNodeStatic(const std::string& category, const std::string& type);

			[[nodiscard]] Node* SpawnNode(const std::string& category, const std::string& type) override { return SpawnNodeStatic(category, type); }
		};

		class SOULFactory : public Factory<SOULFactory>
		{
			[[nodiscard]] static Node* SpawnNodeStatic(const std::string& category, const std::string& type);
			[[nodiscard]] Node* SpawnNode(const std::string& category, const std::string& type) override { return SpawnNodeStatic(category, type); }
		};

	} // namespace Nodes

} // namespac NR