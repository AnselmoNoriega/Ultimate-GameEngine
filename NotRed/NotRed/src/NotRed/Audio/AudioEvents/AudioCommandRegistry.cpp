#include <nrpch.h>
#include "AudioCommandRegistry.h"

#include "yaml-cpp/yaml.h"
#include "NotRed/Util/SerializationMacros.h"

#include "NotRed/Audio/Editor/AudioEventsEditor.h"

#include "NotRed/Util/FileSystem.h"
#include "NotRed/Util/StringUtils.h"
#include "NotRed/Asset/AssetManager.h"

namespace NR
{
	using namespace Audio;

	AudioCommandRegistry::Registry<TriggerCommand>		AudioCommandRegistry::sTriggers;
	AudioCommandRegistry::Registry<SwitchCommand>		AudioCommandRegistry::sSwitches;
	AudioCommandRegistry::Registry<StateCommand>		AudioCommandRegistry::sStates;
	AudioCommandRegistry::Registry<ParameterCommand>	AudioCommandRegistry::sParameters;

	std::function<void()> AudioCommandRegistry::onRegistryChange = nullptr;

	void AudioCommandRegistry::Init()
	{
		LoadCommandRegistry();
		//WriteRegistryToFile();
	}

	void AudioCommandRegistry::Shutdown()
	{
		if (Project::GetActive())
		{
			WriteRegistryToFile();
		}

		sTriggers.clear();
		sSwitches.clear();
		sStates.clear();
		sParameters.clear();
	}

	bool AudioCommandRegistry::AddNewCommand(ECommandType type, const char* uniqueName)
	{
		CommandID id{ uniqueName };

		bool result = false;

		switch (type)
		{
		case ECommandType::Trigger:
			if (DoesCommandExist(type, id))	result = false;
			else
			{
				auto com = TriggerCommand();
				com.DebugName = uniqueName;
				sTriggers.emplace(id, std::move(com));
				result = true;
			}
			break;
		case ECommandType::Switch:
			if (DoesCommandExist(type, id))	result = false;
			else
			{
				auto com = SwitchCommand();
				com.DebugName = uniqueName;
				sSwitches.emplace(id, std::move(com));
				result = true;
			}
			break;
		case ECommandType::State:
			if (DoesCommandExist(type, id))	result = false;
			else
			{
				auto com = StateCommand();
				com.DebugName = uniqueName;
				sStates.emplace(id, std::move(com));
				result = true;
			}
			break;
		case ECommandType::Parameter:
			if (DoesCommandExist(type, id))	result = false;
			else
			{
				auto com = ParameterCommand();
				com.DebugName = uniqueName;
				sParameters.emplace(id, std::move(com));
				result = true;
			}
			break;
		default: result = false;
		}

		if (result)
		{
			OnRegistryChagne();
		}

		return result;
	}

	bool AudioCommandRegistry::DoesCommandExist(ECommandType type, const CommandID& commandID)
	{
		auto isInRegistry = [](auto& registry, const CommandID& id) -> bool
			{
				if (registry.find(id) != registry.end())
				{
					return true;
				}

				return false;
			};

		switch (type)
		{
		case ECommandType::Trigger: return isInRegistry(sTriggers, commandID); break;
		case ECommandType::Switch: return isInRegistry(sSwitches, commandID); break;
		case ECommandType::State: return isInRegistry(sStates, commandID); break;
		case ECommandType::Parameter: return isInRegistry(sParameters, commandID); break;
		default:
			return false;
		}
	}

	void AudioCommandRegistry::LoadCommandRegistry()
	{
		const std::string& audioCommandsRegistryPath = Project::GetAudioCommandsRegistryPath().string();
		if (!FileSystem::Exists(audioCommandsRegistryPath) && !FileSystem::IsDirectory(audioCommandsRegistryPath))
		{
			return;
		}

		std::ifstream stream(audioCommandsRegistryPath);
		NR_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		std::vector<YAML::Node> documents = YAML::LoadAll(strStream.str());
		YAML::Node data = documents[0];

		auto triggers = data["Triggers"];

		if (!triggers)
		{
			NR_CORE_ERROR("CommandRegistry appears to be corrupted!");
			return;
		}

		sTriggers.reserve(triggers.size());

		for (auto entry : triggers)
		{
			TriggerCommand trigger;

			NR_DESERIALIZE_PROPERTY(DebugName, trigger.DebugName, entry, std::string(""));

			if (trigger.DebugName.empty())
			{
				NR_CORE_ERROR("Tried to load invalid command. Command must have a DebugName, or an ID!");
				continue;
			}

			auto actions = entry["Actions"];
			trigger.Actions.GetVector().reserve(actions.size());

			for (auto actionEntry : actions)
			{
				TriggerAction action;
				std::string type;
				std::string context;

				NR_DESERIALIZE_PROPERTY(Type, type, actionEntry, std::string(""));
				NR_DESERIALIZE_PROPERTY_ASSET(Target, action.Target, actionEntry, SoundConfig);
				NR_DESERIALIZE_PROPERTY(Context, context, actionEntry, std::string(""));

				action.Type = Utils::AudioActionTypeFromString(type);
				action.Context = Utils::AudioActionContextFromString(context);

				trigger.Actions.PushBack(action);
			}
			sTriggers[CommandID(trigger.DebugName.c_str())] = trigger;
		}

		OnRegistryChagne();

		AudioEventsEditor::DeserializeTree(documents);
	}

	void AudioCommandRegistry::WriteRegistryToFile()
	{
		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Triggers" << YAML::BeginSeq;
		for (auto& [commandID, trigger] : sTriggers)
		{
			out << YAML::BeginMap; // Trigger entry
			NR_SERIALIZE_PROPERTY(DebugName, trigger.DebugName, out);

			out << YAML::Key << "Actions" << YAML::BeginSeq;
			for (auto& action : trigger.Actions.GetVector())
			{
				out << YAML::BeginMap;
				NR_SERIALIZE_PROPERTY(Type, Utils::AudioActionTypeToString(action.Type), out);
				NR_SERIALIZE_PROPERTY_ASSET(Target, action.Target, out);
				NR_SERIALIZE_PROPERTY(Context, Utils::AudioActionContextToString(action.Context), out);
				out << YAML::EndMap;
			}
			out << YAML::EndSeq; // Actions
			out << YAML::EndMap; // Trigger entry
		}
		out << YAML::EndSeq;// Triggers
		out << YAML::EndMap;

		AudioEventsEditor::SerializeTree(out);

		const std::string& audioCommandsRegistryPath = Project::GetAudioCommandsRegistryPath().string();
		std::ofstream fout(audioCommandsRegistryPath);
		fout << out.c_str();
	}

	void AudioCommandRegistry::OnRegistryChagne()
	{
		if (onRegistryChange)
		{
			onRegistryChange();
		}
	}
} // namespace