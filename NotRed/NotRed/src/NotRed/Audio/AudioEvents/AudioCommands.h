#pragma once

#include "NotRed/Audio/Sound.h"
#include "NotRed/Audio/Editor/OrderedVector.h"

namespace NR::Audio
{
	enum EActionType : int
	{
		Play, Stop, StopAll,
		Pause, PauseAll, Resume, ResumeAll,
		Break,
		Seek, SeekAll,
		PostTrigger
	};

	enum EActionContext : int
	{
		GameObject, Global
	};


	enum class ECommandType
	{
		Trigger, Switch, State, Parameter
	};

	struct TriggerAction
	{
		EActionType Type;
		Ref<SoundConfig> Target;
		EActionContext Context;

		bool Handled = false;

		bool operator==(const TriggerAction& other) const
		{
			return Type == other.Type && Target == other.Target && Context == other.Context && Handled == other.Handled;
		}
	};

	struct CommandBase
	{
		std::string DebugName;
		bool Handled = false;
		virtual ECommandType GetType() const = 0;
	};

	struct TriggerCommand : public CommandBase
	{
		virtual ECommandType GetType() const override { return ECommandType::Trigger; }
		OrderedVector<TriggerAction> Actions;

		/* This is used when need to skip executing the Event to the next Update, e.g. waiting for "stop-fade"
		   It might be useful to have in a base class, or in some other, more generic form. */
		bool DelayExecution = false;

		TriggerCommand& operator=(const TriggerCommand& other)
		{
			if (this == &other)
			{
				return *this;
			}

			Actions.SetValues(other.Actions.GetVector());
			DebugName = other.DebugName;
			return *this;
		}
	};

	struct SwitchCommand : public CommandBase
	{
		ECommandType GetType() const override { return ECommandType::Switch; }
	};

	struct StateCommand : public CommandBase
	{
		ECommandType GetType() const override { return ECommandType::State; }
	};

	struct ParameterCommand : public CommandBase
	{
		ECommandType GetType() const override { return ECommandType::Parameter; }
	};
} // namespace Audio

namespace NR::Utils
{
	inline NR::Audio::EActionType AudioActionTypeFromString(const std::string& actionType)
	{
		if (actionType == "Play")		return NR::Audio::EActionType::Play;
		if (actionType == "Stop")		return NR::Audio::EActionType::Stop;
		if (actionType == "StopAll")	return NR::Audio::EActionType::StopAll;
		if (actionType == "Pause")		return NR::Audio::EActionType::Pause;
		if (actionType == "PauseAll")	return NR::Audio::EActionType::PauseAll;
		if (actionType == "Resume")		return NR::Audio::EActionType::Resume;
		if (actionType == "ResumeAll")	return NR::Audio::EActionType::ResumeAll;
		if (actionType == "Break")		return NR::Audio::EActionType::Break;
		if (actionType == "Seek")		return NR::Audio::EActionType::Seek;
		if (actionType == "SeekAll")	return NR::Audio::EActionType::SeekAll;
		if (actionType == "PostTrigger")return NR::Audio::EActionType::PostTrigger;

		NR_CORE_ASSERT(false, "Unknown Action Type");
		return NR::Audio::EActionType::Play;
	}

	inline const char* AudioActionTypeToString(NR::Audio::EActionType actionType)
	{
		switch (actionType)
		{
		case NR::Audio::EActionType::Play:			return "Play";
		case NR::Audio::EActionType::Stop:			return "Stop";
		case NR::Audio::EActionType::StopAll:		return "StopAll";
		case NR::Audio::EActionType::Pause:			return "Pause";
		case NR::Audio::EActionType::PauseAll:		return "PauseAll";
		case NR::Audio::EActionType::Resume:		return "Resume";
		case NR::Audio::EActionType::ResumeAll:		return "ResumeAll";
		case NR::Audio::EActionType::Break:			return "Break";
		case NR::Audio::EActionType::Seek:			return "Seek";
		case NR::Audio::EActionType::SeekAll:		return "SeekAll";
		case NR::Audio::EActionType::PostTrigger:	return "PostTrigger";
		default:
		{
			NR_CORE_ASSERT(false, "Unknown Action Type");
			return "None";
		}
		}
	}

	inline NR::Audio::EActionContext AudioActionContextFromString(const std::string& context)
	{
		if (context == "GameObject")	return NR::Audio::EActionContext::GameObject;
		if (context == "Global")		return NR::Audio::EActionContext::Global;

		NR_CORE_ASSERT(false, "Unknown Context Type");
		return NR::Audio::EActionContext::GameObject;
	}

	inline const char* AudioActionContextToString(NR::Audio::EActionContext context)
	{
		switch (context)
		{
		case NR::Audio::EActionContext::GameObject:	return "GameObject";
		case NR::Audio::EActionContext::Global:		return "Global";
		default:
		{
			NR_CORE_ASSERT(false, "Unknown Context Type");
			return "None";
		}
		}
	}

} // namespace Utils