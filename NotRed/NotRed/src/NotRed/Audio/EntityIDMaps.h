#pragma once

#include <optional>
#include <shared_mutex>
#include <numeric>
#include <variant>

#include "NotRed/Core/UUID.h"
#include "NotRed/Scene/Entity.h"
#include "AudioEvents/AudioCommands.h"
#include "AudioEvents/CommandID.h"
#include "AudioComponent.h"

namespace NR
{
    /* ---------------------------------------------------------------
        Structure that provides thread safe access to map of ScenesIDs,
        their EntityIDs and associated to the Entities objects.
        --------------------------------------------------------------
    */
    template<typename T>
    struct EntityIDMap
    {
        void Add(UUID sceneID, UUID entityID, T object)
        {
            std::scoped_lock lock{ mMutex };
            mEntityIDMap[sceneID][entityID] = object;
        }

        void Remove(UUID sceneID, UUID entityID)
        {
            std::scoped_lock lock{ mMutex };
            NR_CORE_ASSERT(mEntityIDMap.at(sceneID).find(entityID) != mEntityIDMap.at(sceneID).end(),
                "Could not find entityID in the registry to remove.");

            mEntityIDMap.at(sceneID).erase(entityID);
            if (mEntityIDMap.at(sceneID).empty())
                mEntityIDMap.erase(sceneID);
        }

        std::optional<T> Get(UUID sceneID, UUID entityID) const
        {
            std::shared_lock lock{ mMutex };
            if (mEntityIDMap.find(sceneID) != mEntityIDMap.end())
            {
                if (mEntityIDMap.at(sceneID).find(entityID) != mEntityIDMap.at(sceneID).end())
                {
                    return std::optional<T>(mEntityIDMap.at(sceneID).at(entityID));
                }
            }
            return std::optional<T>();
        }

        std::optional<std::unordered_map<UUID, T>> Get(UUID sceneID) const
        {
            std::shared_lock lock{ mMutex };
            if (mEntityIDMap.find(sceneID) != mEntityIDMap.end())
            {
                return std::optional<std::unordered_map<UUID, T>>(mEntityIDMap.at(sceneID));
            }
            return std::optional<std::unordered_map<UUID, T>>();
        }

        void Clear(UUID sceneID)
        {
            std::scoped_lock lock{ mMutex };
            if (sceneID == 0)
            {
                mEntityIDMap.clear();
            }
            else if (mEntityIDMap.find(sceneID) != mEntityIDMap.end())
            {
                mEntityIDMap.at(sceneID).clear();
                mEntityIDMap.erase(sceneID);
            }
        }

        uint64_t Count(UUID sceneID) const
        {
            std::shared_lock lock{ mMutex };
            if (mEntityIDMap.find(sceneID) != mEntityIDMap.end())
                return mEntityIDMap.at(sceneID).size();
            else
                return 0;
        }
    protected:
        mutable std::shared_mutex mMutex;
    private:
        std::unordered_map<UUID, std::unordered_map<UUID, T>> mEntityIDMap;
    };


    /*  =================================================================
        Registry to keep track of all Audio Components in the game engine

        Provides easy access to Audio Components from AudioEngine
        -----------------------------------------------------------------
    */
    struct AudioComponentRegistry : public EntityIDMap<Entity>
    {
        AudioComponent* GetAudioComponent(UUID sceneID, uint64_t audioComponentID) const;
        Entity GetEntity(UUID sceneID, uint64_t audioComponentID) const;
    };

    /*
    template<class T>
    struct ActiveCommandsMap
    {
    public:
        ActiveCommandsMap() {};
        ActiveCommandsMap(const ActiveCommandsMap&) = delete;

        using CommandScoped = Scope<T>;

        //===============================================================================================

        uint64_t GetCommandCount(UUID sceneID) const
        {
            return mActiveCommands.Count(sceneID);
        }

        std::optional<std::vector<CommandScoped>*> GetCommands(UUID sceneID, UUID objectID) const
        {
            return mActiveCommands.Get(sceneID, objectID);
        }

        virtual void AddCommand(UUID sceneID, UUID objectID, T* command)
        {
            if (auto* commands = mActiveCommands.Get(sceneID, objectID).value_or(nullptr))
            {
                commands->push_back(CommandScoped(command));
            }
            else
            {
                auto newCommandsList = new std::vector<CommandScoped>();
                newCommandsList->push_back(CommandScoped(command));
                mActiveCommands.Add(sceneID, objectID, newCommandsList);
            }
        }

        // @returns true - if `command` was found and removed from the registry.
        virtual bool RemoveCommand(UUID sceneID, UUID objectID, T* command)
        {
            if (auto* commands = mActiveCommands.Get(sceneID, objectID).value_or(nullptr))
            {
                commands->erase(std::find_if(commands->begin(), commands->end(), [command](CommandScoped& itCommand) {return itCommand.get() == command; }));

                if (commands->empty())
                {
                    delete* mActiveCommands.Get(sceneID, objectID);
                    mActiveCommands.Remove(sceneID, objectID);
                }

                return true;
            }

            return false;
        }

    protected:
        EntityIDMap<std::vector<CommandScoped>*> mActiveCommands;
    };


    struct ActiveTriggersRegistry : private ActiveCommandsMap<TriggerCommand>
    {
    public:
        ActiveTriggersRegistry();
        ActiveTriggersRegistry(const ActiveTriggersRegistry&) = delete;

        using TriggerScoped = Scope<TriggerCommand>;

        //===============================================================================================

        uint64_t Count(UUID sceneID);

        std::optional<std::vector<TriggerScoped>*> GetCommands(UUID sceneID, UUID objectID) const;
        void AddTrigger(UUID sceneID, UUID objectID, TriggerCommand* trigger);

        // @returns true - if `trigger` was found and removed from the registry./
        bool RemoveTrigger(UUID sceneID, UUID objectID, TriggerCommand* trigger);

        // @returns true - if Trigger was fully handled.
        bool OnTriggerActionHandled(UUID sceneID, UUID objectID, TriggerCommand* trigger);

        // @returns true - if Trigger owning Play action of the `soundSource` was fully handled.
        bool OnPlaybackActionFinished(UUID sceneID, UUID objectID, Ref<SoundConfig> soundSource);

    private:
        bool RefreshActions(TriggerCommand* trigger);
        bool RefreshTriggers(UUID sceneID, UUID objectID);

    private:
        mutable std::shared_mutex mMutex;
    };
    */

    namespace Audio
    {
        using EventID = UUID32;          // Event is an active execution instance of a Command 
        using SourceID = UUID32;         // ID of the sound source

        struct EventInfo
        {
            using StateObject = std::variant<std::monostate,
                TriggerCommand,
                SwitchCommand,
                StateCommand,
                ParameterCommand>;

            EventInfo(const CommandID& commandID, UUID objectID);
            EventInfo()
                : commandID("")
            {};

            EventID EventID;                                      // ID of the Event instance
            CommandID commandID;                                  // calling Command, used to retrieve instructions and list of Actions to perform
            UUID ObjectID;                                 // context Object of the Event
            std::vector<SourceID> ActiveSources;                  // Active Sources associated with the Playback Instance

            std::shared_ptr<StateObject> CommandState{ nullptr }; // Object to track execution of the Actions
        };

        // Events and associated info
        struct EventRegistry
        {
        public:
            uint32_t Count() const;
            // @returns number of Sources associated to an Event
            uint32_t GetNumberOfSources(EventID eventID) const;

            EventID Add(EventInfo& eventInfo);
            bool Remove(EventID eventID);

            // Associate Sound Source with the Event
            bool AddSource(EventID eventID, SourceID sourceID);
            // @returns true - if eventID doesn't have any sourceIDs left and was removed from the registry
            bool RemoveSource(EventID eventID, SourceID sourceID);

            EventInfo Get(EventID eventID) const;

        private:
            mutable std::shared_mutex mMutex;
            std::unordered_map<EventID, EventInfo> mPlaybackInstances;
        };


        // Objects and associated Events
        struct ObjectEventRegistry
        {
        public:
            uint32_t Count() const;
            uint32_t GetTotalPlaybackCount() const;

            bool Add(UUID objectID, EventID eventID);
            // @returns true - if objectID doesn't have any eventIDs left and was removed from the registry
            bool Remove(UUID objectID, EventID eventID);
            bool RemoveObject(UUID objectID);

            uint32_t GetNumberOfEvents(UUID objectID) const;
            std::vector<EventID> GetEvents(UUID objectID)  const;

        private:
            mutable std::shared_mutex mMutex;
            std::unordered_map<UUID, std::vector<EventID>> mObjects;
        };


        // Objects and associated active Sounds
        struct ObjectSourceRegistry
        {
        public:
            uint32_t Count() const;
            uint32_t GetTotalPlaybackCount() const;

            bool Add(UUID objectID, SourceID sourceID);
            // @returns true - if objectID doesn't have any sourceIDs left and was removed from the registry
            bool Remove(UUID objectID, SourceID sourceID);
            bool RemoveObject(UUID objectID);

            uint32_t GetNumberOfActiveSounds(UUID objectID) const;
            std::optional<std::vector<SourceID>> GetActiveSounds(UUID objectID) const;

        private:
            mutable std::shared_mutex mMutex;
            std::unordered_map<UUID, std::vector<SourceID>> mObjects;
        };

    } // namespace Audio

}