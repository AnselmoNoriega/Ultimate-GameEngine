#include <nrpch.h>
#include "EntityIDMaps.h"

#include "AudioEvents/CommandID.h"

namespace NR
{
	AudioComponent* AudioComponentRegistry::GetAudioComponent(UUID sceneID, uint64_t audioComponentID) const
	{
		std::optional<Entity> o = Get(sceneID, audioComponentID);
		if (o.has_value())
		{
			return &o->GetComponent<AudioComponent>();
		}
		else
		{
			NR_CORE_ASSERT("Component was not found in registry");
			return nullptr;
		}
	}

	Entity AudioComponentRegistry::GetEntity(UUID sceneID, uint64_t audioComponentID) const
	{
		return Get(sceneID, UUID(audioComponentID)).value_or(Entity());
	}

	//===============================================================================================
	namespace Audio
	{

		EventInfo::EventInfo(const CommandID& comdID, UUID objectID)
			: commandID(comdID), ObjectID(objectID)
		{
		}

		//===============================================================================================

		EventID EventRegistry::Add(EventInfo& eventInfo)
		{
			std::scoped_lock lock{ mMutex };

			EventID eventID;
			eventInfo.EventID = eventID;
			mPlaybackInstances[eventID] = eventInfo;
			return eventID;
		}

		bool EventRegistry::Remove(EventID eventID)
		{
			std::scoped_lock lock{ mMutex };

			NR_CORE_ASSERT(mPlaybackInstances.find(eventID) != mPlaybackInstances.end());

			return mPlaybackInstances.erase(eventID);
		}

		bool EventRegistry::AddSource(EventID eventID, SourceID sourceID)
		{
			std::scoped_lock lock{ mMutex };

			NR_CORE_ASSERT(mPlaybackInstances.find(eventID) != mPlaybackInstances.end());

			auto& sources = mPlaybackInstances.at(eventID).ActiveSources;
			sources.push_back(sourceID);
			return true;
		}

		uint32_t EventRegistry::Count() const
		{
			std::shared_lock lock{ mMutex };

			return (uint32_t)mPlaybackInstances.size();
		}

		uint32_t EventRegistry::GetNumberOfSources(EventID eventID) const
		{
			std::shared_lock lock{ mMutex };

			//NR_CORE_ASSERT(mPlaybackInstances.find(eventID) != mPlaybackInstances.end());

			if (mPlaybackInstances.find(eventID) != mPlaybackInstances.end())
				return (uint32_t)mPlaybackInstances.at(eventID).ActiveSources.size();
			else
				return 0;
		}

		bool EventRegistry::RemoveSource(EventID eventID, SourceID sourceID)
		{
			std::scoped_lock lock{ mMutex };

			NR_CORE_ASSERT(mPlaybackInstances.find(eventID) != mPlaybackInstances.end());

			auto& sources = mPlaybackInstances.at(eventID).ActiveSources;
			auto it = std::find(sources.begin(), sources.end(), sourceID);

			if (it == sources.end())
				NR_CORE_ERROR("Audio. EventRegistry. Attempted to remove source that's not associated to any Event in registry.");
			else
				sources.erase(it);

			return sources.empty();
			//? Should not remove EventInfo when last SourceID removed,
			//? there might still be active Events that are not referencing any Sound Source
			//mPlaybackInstances.erase(eventID);
		}

		EventInfo EventRegistry::Get(EventID eventID) const
		{
			std::shared_lock lock{ mMutex };

			NR_CORE_ASSERT(mPlaybackInstances.find(eventID) != mPlaybackInstances.end());

			return mPlaybackInstances.at(eventID);
		}


		//===============================================================================================

		uint32_t ObjectEventRegistry::Count() const
		{
			std::shared_lock lock{ mMutex };

			return (uint32_t)mObjects.size();
		}

		uint32_t ObjectEventRegistry::GetTotalPlaybackCount() const
		{
			std::shared_lock lock{ mMutex };

			return static_cast<uint32_t>(std::accumulate(mObjects.begin(), mObjects.end(), 0ULL,
				[](uint32_t prior, const std::pair<UUID, std::vector<EventID>>& item) -> uint32_t
				{
					return prior + static_cast<uint32_t>(item.second.size());
				}));
		}

		bool ObjectEventRegistry::Add(UUID objectID, EventID eventID)
		{
			std::scoped_lock lock{ mMutex };

			if (mObjects.find(objectID) != mObjects.end())
			{
				// If object already in the registy, associate eventID with it

				auto& playbacks = mObjects.at(objectID);

				if (std::find(playbacks.begin(), playbacks.end(), eventID) != playbacks.end())
				{
					NR_CORE_ASSERT(false, "EventID already associated to the object.");
					return false;
				}

				playbacks.push_back(eventID);
				return true;
			}
			else
			{
				// Add new object with associated eventID
				return mObjects.emplace(objectID, std::vector<EventID>{eventID}).second;
			}
		}

		bool ObjectEventRegistry::Remove(UUID objectID, EventID eventID)
		{
			std::scoped_lock lock{ mMutex };

			NR_CORE_ASSERT(mObjects.find(objectID) != mObjects.end());

			auto& events = mObjects.at(objectID);
			auto it = std::find(events.begin(), events.end(), eventID);

			if (it == events.end())
				NR_CORE_ERROR("Audio. ObjectEventRegistry. Attempted to remove Event that's not associated to any Object in registry.");
			else
				events.erase(it);

			if (events.empty())
			{
				mObjects.erase(objectID);
				return true;
			}

			return false;
		}

		bool ObjectEventRegistry::RemoveObject(UUID objectID)
		{
			std::scoped_lock lock{ mMutex };

			NR_CORE_ASSERT(mObjects.find(objectID) != mObjects.end());

			return mObjects.erase(objectID);
		}


		uint32_t ObjectEventRegistry::GetNumberOfEvents(UUID objectID) const
		{
			std::shared_lock lock{ mMutex };

			if (mObjects.find(objectID) != mObjects.end())
				return (uint32_t)mObjects.at(objectID).size();

			return 0;
		}

		std::vector<EventID> ObjectEventRegistry::GetEvents(UUID objectID) const
		{
			std::shared_lock lock{ mMutex };

			NR_CORE_ASSERT(mObjects.find(objectID) != mObjects.end());

			return mObjects.at(objectID);
		}


		//===============================================================================================

		uint32_t ObjectSourceRegistry::Count() const
		{
			std::shared_lock lock{ mMutex };

			return (uint32_t)mObjects.size();
		}

		uint32_t ObjectSourceRegistry::GetTotalPlaybackCount() const
		{
			std::shared_lock lock{ mMutex };

			return (uint32_t)std::accumulate(mObjects.begin(), mObjects.end(), 0,
				[](uint32_t prior, const std::pair<UUID, std::vector<SourceID>>& item) -> uint32_t
				{
					return prior + (uint32_t)item.second.size();
				});
		}

		bool ObjectSourceRegistry::Add(UUID objectID, SourceID sourceID)
		{
			std::scoped_lock lock{ mMutex };

			if (mObjects.find(objectID) != mObjects.end())
			{
				// If object already in the registy, associate sourceID with it

				auto& sounds = mObjects.at(objectID);

				if (std::find(sounds.begin(), sounds.end(), sourceID) != sounds.end())
				{
					NR_CORE_ASSERT(false, "Active Sound already associated to the object.");
					return false;
				}

				sounds.push_back(sourceID);
				return true;
			}
			else
			{
				// Add new object with associated sourceID
				return mObjects.emplace(objectID, std::vector<SourceID>{sourceID}).second;
			}
		}

		bool ObjectSourceRegistry::Remove(UUID objectID, SourceID sourceID)
		{
			std::scoped_lock lock{ mMutex };

			NR_CORE_ASSERT(mObjects.find(objectID) != mObjects.end());

			auto& sounds = mObjects.at(objectID);
			auto it = std::find(sounds.begin(), sounds.end(), sourceID);

			if (it == sounds.end())
				NR_CORE_ERROR("Audio. ObjectEventRegistry. Attempted to remove Source that's not associated to any Object in registry.");
			else
				sounds.erase(it);

			if (sounds.empty())
			{
				mObjects.erase(objectID);
				return true;
			}

			return false;
		}

		bool ObjectSourceRegistry::RemoveObject(UUID objectID)
		{
			std::scoped_lock lock{ mMutex };

			NR_CORE_ASSERT(mObjects.find(objectID) != mObjects.end());

			return mObjects.erase(objectID);
		}

		uint32_t ObjectSourceRegistry::GetNumberOfActiveSounds(UUID objectID) const
		{
			std::shared_lock lock{ mMutex };

			if (mObjects.find(objectID) != mObjects.end())
				return (uint32_t)mObjects.at(objectID).size();

			return 0;
		}

		std::optional<std::vector<SourceID>> ObjectSourceRegistry::GetActiveSounds(UUID objectID) const
		{
			std::shared_lock lock{ mMutex };

			if (mObjects.find(objectID) != mObjects.end())
				return mObjects.at(objectID);

			return {};
		}

	} // namespace Audio

} // namespace NotRed