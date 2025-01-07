#pragma once

#include "CommandID.h"
#include "AudioCommands.h"

namespace NR
{
	class AudioCommandRegistry
	{
	public:
		template<typename Type>
		using Registry = std::unordered_map<Audio::CommandID, Type>;

		static void Init();
		static void Shutdown();

		template <class Type>
		static bool AddNewCommand(const char* uniqueName) = delete;
		template<> static bool AddNewCommand<Audio::TriggerCommand>(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::Trigger, uniqueName); }
		template<> static bool AddNewCommand<Audio::SwitchCommand>(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::Switch, uniqueName); }
		template<> static bool AddNewCommand<Audio::StateCommand>(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::State, uniqueName); }
		template<> static bool AddNewCommand<Audio::ParameterCommand>(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::Parameter, uniqueName); }

		template <class Type>
		static bool DoesCommandExist(const Audio::CommandID& commandID) = delete;
		template<> static bool DoesCommandExist<Audio::TriggerCommand>(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::Trigger, commandID); }
		template<> static bool DoesCommandExist<Audio::SwitchCommand>(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::Switch, commandID); }
		template<> static bool DoesCommandExist<Audio::StateCommand>(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::State, commandID); }
		template<> static bool DoesCommandExist<Audio::ParameterCommand>(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::Parameter, commandID); }

		template <class Type>
		static bool RemoveCommand(const Audio::CommandID& ID) = delete;
		template<> static bool RemoveCommand<Audio::TriggerCommand>(const Audio::CommandID& ID) { bool result = sTriggers.erase(ID);		OnRegistryChagne(); return result; }
		template<> static bool RemoveCommand<Audio::SwitchCommand>(const Audio::CommandID& ID) { bool result = sSwitches.erase(ID);		OnRegistryChagne(); return result; }
		template<> static bool RemoveCommand<Audio::StateCommand>(const Audio::CommandID& ID) { bool result = sStates.erase(ID);		OnRegistryChagne(); return result; }
		template<> static bool RemoveCommand<Audio::ParameterCommand>(const Audio::CommandID& ID) { bool result = sParameters.erase(ID);	OnRegistryChagne(); return result; }

		template <class Type>
		static Type& GetCommand(const Audio::CommandID& ID) = delete;
		template<> static Audio::TriggerCommand& GetCommand<Audio::TriggerCommand>(const Audio::CommandID& ID) { return sTriggers.at(ID); }
		template<> static Audio::SwitchCommand& GetCommand<Audio::SwitchCommand>(const Audio::CommandID& ID) { return sSwitches.at(ID); }
		template<> static Audio::StateCommand& GetCommand<Audio::StateCommand>(const Audio::CommandID& ID) { return sStates.at(ID); }
		template<> static Audio::ParameterCommand& GetCommand<Audio::ParameterCommand>(const Audio::CommandID& ID) { return sParameters.at(ID); }

		template <class Type>
		static const Registry<Type>& GetRegistry() {}
		template<> static const Registry<Audio::TriggerCommand>& GetRegistry<Audio::TriggerCommand>() { return sTriggers; }
		template<> static const Registry<Audio::SwitchCommand>& GetRegistry<Audio::SwitchCommand>() { return sSwitches; }
		template<> static const Registry<Audio::StateCommand>& GetRegistry<Audio::StateCommand>() { return sStates; }
		template<> static const Registry<Audio::ParameterCommand>& GetRegistry<Audio::ParameterCommand>() { return sParameters; }

		static void WriteRegistryToFile();

	private:
		static bool DoesCommandExist(Audio::ECommandType type, const Audio::CommandID& commandID);
		static bool AddNewCommand(Audio::ECommandType type, const char* uniqueName);

		friend class AudioEventsEditor;
		static void LoadCommandRegistry();

		static void OnRegistryChagne();

	public:
		static std::function<void()> onRegistryChange;

	private:
		static Registry<Audio::TriggerCommand> sTriggers;
		static Registry<Audio::SwitchCommand> sSwitches;
		static Registry<Audio::StateCommand> sStates;
		static Registry<Audio::ParameterCommand> sParameters;
	};
} // namespace Hazel
