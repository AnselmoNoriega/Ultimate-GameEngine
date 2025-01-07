#include "nrpch.h"
#include "AudioEngine.h"

#include <algorithm>
#include <execution>

#include "AudioEvents/AudioCommandRegistry.h"

#include "NotRed/Debug/Profiler.h"
#include "NotRed/Core/Timer.h"

#include "DSP/Reverb/Reverb.h"
#include "DSP/Spatializer/Spatializer.h"
#include "DSP/Spatializer/Spatializer.h"

#include "yaml-cpp/yaml.h"

#ifdef DBG_VOICES // Enable logging of Voice handling
#define LOG_VOICES(...) NR_CORE_INFO(__VA_ARGS__)
#else
#define LOG_VOICES(...)
#endif

#if 0 // Enable logging for Events handling
#define LOG_EVENTS(...) NR_CORE_INFO(__VA_ARGS__)
#else
#define LOG_EVENTS(...)
#endif

namespace NR
{
	using namespace Audio;

	AudioEngine* AudioEngine::sInstance = nullptr;

	Audio::Stats AudioEngine::sStats;

	void AudioEngine::ExecuteOnAudioThread(AudioThreadCallbackFunction func, const char* jobID)
	{
		AudioThread::AddTask(new AudioFunctionCallback(std::move(func), jobID));
	}

	void MemFreeCallback(void* p, void* pUserData)
	{
		if (p == NULL)
		{
			return;
		}

		constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

		char* buffer = (char*)p - offset; //get the start of the buffer
		int* sizeBox = (int*)buffer;

		auto* alData = (AllocationCallbackData*)pUserData;
		{
			std::scoped_lock lock{ alData->Stats.mutex };
			if (alData->isResourceManager)      alData->Stats.MemResManager -= *sizeBox;
			else                                alData->Stats.MemEngine -= *sizeBox;
		}

#ifdef LOG_MEMORY
		NR_CORE_INFO("Freed mem KB: {0}", *sizeBox / 1000.0f);
#endif
		std::free(buffer);
	}

	void* MemAllocCallback(size_t sz, void* pUserData)
	{
		constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

		char* buffer = (char*)malloc(sz + offset); //allocate offset extra bytes 
		if (buffer == NULL)
		{
			return NULL; // no memory! 
		}

		auto* alData = (AllocationCallbackData*)pUserData;
		{
			std::scoped_lock lock{ alData->Stats.mutex };
			if (alData->isResourceManager)      alData->Stats.MemResManager += sz;
			else                                alData->Stats.MemEngine += sz;
		}


		int* sizeBox = (int*)buffer;
		*sizeBox = (int)sz; //store the size in first sizeof(int) bytes!

#ifdef LOG_MEMORY
		NR_CORE_INFO("Allocated mem KB: {0}", *sizeBox / 1000.0f);
#endif
		return buffer + offset; //return buffer after offset bytes!
	}

	void* MemReallocCallback(void* p, size_t sz, void* pUserData)
	{
		constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

		auto* buffer = (char*)p - offset; //get the start of the buffer
		int* sizeBox = (int*)buffer;

		auto* alData = (AllocationCallbackData*)pUserData;
		{
			std::scoped_lock lock{ alData->Stats.mutex };
			if (alData->isResourceManager)      alData->Stats.MemResManager += sz - *sizeBox;
			else                                alData->Stats.MemEngine += sz - *sizeBox;
		}

		*sizeBox = (int)sz;

		auto* newBuffer = (char*)ma_realloc(buffer, sz, NULL);
		if (newBuffer == NULL)
			return NULL;

#ifdef LOG_MEMORY
		NR_CORE_INFO("Reallocated mem KB: {0}", sz / 1000.0f);
#endif
		return newBuffer + offset;
	}

	void MALogCallback(void* pUserData, ma_uint32 level, const char* pMessage)
	{
		std::string message = "[miniaudio] - " + std::string(ma_log_level_to_string(level)) + ": " + pMessage;
		message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());

		switch (level)
		{
		case MA_LOG_LEVEL_INFO:
			NR_CORE_TRACE(message);
			break;
		case MA_LOG_LEVEL_WARNING:
			NR_CORE_INFO(message);
			break;
		case MA_LOG_LEVEL_ERROR:
			NR_CORE_ERROR(message);
			break;
		default:
			NR_CORE_TRACE(message);
		}
	}

	//==========================================================================
	AudioEngine::AudioEngine()
	{
		//AudioThread::BindUpdateFunction<AudioEngine, &AudioEngine::Update>(this);
		AudioThread::BindUpdateFunction([this](float dt) { Update(dt); });
		AudioThread::Start();

		AudioEngine::ExecuteOnAudioThread([this] { Initialize(); }, "InitializeAudioEngine");
	}

	AudioEngine::~AudioEngine()
	{
		if (bInitialized)
		{
			Uninitialize();
		}
	}

	void AudioEngine::Init()
	{
		NR_CORE_ASSERT(sInstance == nullptr, "Audio Engine already initialized.");
		AudioEngine::sInstance = new AudioEngine();
	}

	void AudioEngine::Shutdown()
	{
		NR_CORE_ASSERT(sInstance, "Audio Engine was not initialized.");
		sInstance->Uninitialize();
		delete sInstance;
		sInstance = nullptr;
	}

	bool AudioEngine::Initialize()
	{
		if (bInitialized)
			return true;

		NR_PROFILE_FUNC();

		ma_result result;
		ma_engine_config engineConfig = ma_engine_config_init();
		// TODO: for now splitter node and custom node don't work toghether if custom periodSizeInFrames is set
		//engineConfig.periodSizeInFrames = PCM_FRAME_CHUNK_SIZE;

		ma_allocation_callbacks allocationCallbacks{ &mEngineCallbackData, &MemAllocCallback, &MemReallocCallback, &MemFreeCallback };

		result = ma_log_init(&allocationCallbacks, &mmaLog);
		NR_CORE_ASSERT(result == MA_SUCCESS, "Failed to initialize miniaudio logger.");
		ma_log_callback logCallback = ma_log_callback_init(&MALogCallback, nullptr);
		result = ma_log_register_callback(&mmaLog, logCallback);
		NR_CORE_ASSERT(result == MA_SUCCESS, "Failed to register miniaudio log callback.");

		engineConfig.allocationCallbacks = allocationCallbacks;
		engineConfig.pLog = &mmaLog;

		result = ma_engine_init(&engineConfig, &mEngine);
		if (result != MA_SUCCESS)
		{
			NR_CORE_ASSERT(false, "Failed to initialize audio engine.");
			return false;
		}


		allocationCallbacks.pUserData = &mRMCallbackData;
		mEngine.pResourceManager->config.allocationCallbacks = allocationCallbacks;

		mSourceManager.Initialize();
		mMasterReverb = CreateScope<DSP::Reverb>();
		mMasterReverb->Initialize(&mEngine, &mEngine.nodeGraph.endpoint);

		// Play Test sound
		/*ma_sound_init_fromfile(&mEngine, "assets/audio/LR test.wav", MA_DATA_SOURCE_FLAG_DECODE, NULL, &mTestSound);
		ma_sound_set_spatialization_enabled(&mTestSound, MA_FALSE);
		ma_sound_start(&mTestSound);
		ma_sound_set_fade_in_milliseconds(&mTestSound, 0.0f, 1.0f, 200);*/

		// TODO: get max number of sources from platform interface, or user settings
		mNumSources = 32;
		CreateSources();

		NR_CORE_INFO(R"(Audio Engine: engine initialized.
                    -----------------------------
                    Endpoint Device Name:   {0}
                    Sample Rate:            {1}
                    Channels:               {2}
                    -----------------------------
                    Callback Buffer Size:   {3}
                    Number of Sources:      {4}
                    -----------------------------)",
			mEngine.pDevice->playback.name,
			mEngine.pDevice->sampleRate,
			mEngine.pDevice->playback.channels,
			engineConfig.periodSizeInFrames,
			mNumSources);

		{
			std::scoped_lock lock{ sStats.mutex };
			sStats.TotalSources = mNumSources;
		}

		bInitialized = true;
		return true;
	}

	bool AudioEngine::Uninitialize()
	{
		StopAll(true);

		mSceneContext = nullptr;
		mAudioComponentRegistry.Clear(0);

		AudioThread::Stop();

		mSourceManager.UninitializeEffects();
		mMasterReverb.reset();

		ma_engine_uninit(&mEngine);

		for (auto& s : mSoundSources)
		{
			delete s;
		}

		bInitialized = false;

		NR_CORE_INFO("Audio Engine: engine un-initialized.");

		return true;
	}


	//==================================================================================

	void AudioEngine::CreateSources()
	{
		NR_PROFILE_FUNC();

		mSoundSources.reserve(mNumSources);
		for (int i = 0; i < mNumSources; i++)
		{
			Sound* soundSource = new Sound();
			soundSource->mSoundSourceID = i;
			mSoundSources.push_back(soundSource);
			mSourceManager.mFreeSourcIDs.push(i);
		}
	}

	Sound* AudioEngine::FreeLowestPrioritySource()
	{
		NR_PROFILE_FUNC();

		NR_CORE_ASSERT(mSourceManager.mFreeSourcIDs.empty());

		Sound* lowestPriStoppingSource = nullptr;
		Sound* lowestPriNonLoopingSource = nullptr;
		Sound* lowestPriSource = nullptr;

		// Compare and return lower priority Source of the two
		auto getLowerPriority = [](Sound* sourceToCheck, Sound* sourceLowest, bool checkPlaybackPos = false) -> Sound*
			{
				if (!sourceLowest)
				{
					return sourceToCheck;
				}
				else
				{
					float a = sourceToCheck->GetPriority();
					float b = sourceLowest->GetPriority();

					if (a < b)             return sourceToCheck;
					else if (a > b)             return sourceLowest;
					else if (checkPlaybackPos)  return sourceToCheck->GetPlaybackPercentage() > sourceLowest->GetPlaybackPercentage() ? sourceToCheck : sourceLowest;
					else                        return sourceLowest;
				}
			};

		// Run through all of the active sources and find the lowest priority source that can be stopped
		for (SoundObject* so : mActiveSounds)
		{
			/* TODO
				make sure only checking Sounds (Voices) (which could be the lowest level SoundWave),
				and handle, in some way, complex SoundObject graphs that may contain multiple Waves.
				Should probably keep track of Active Sources and Active Sounds separatelly (e.i. implement ActiveSound abstraction)
			*/
			if (auto* source = dynamic_cast<Sound*> (so))
			{
				if (source->IsStopping())
				{
					lowestPriStoppingSource = getLowerPriority(source, lowestPriStoppingSource);
				}
				else
				{
					if (!source->bLooping)
					{
						// Checking playback percentage here, in case the volume weighted prioprity is the same
						lowestPriNonLoopingSource = getLowerPriority(source, lowestPriNonLoopingSource, true);
					}
					else
					{
						lowestPriSource = getLowerPriority(source, lowestPriSource);
					}
				}
			}
		}

		Sound* releasedSoundSource = nullptr;

		if (lowestPriStoppingSource)   releasedSoundSource = lowestPriNonLoopingSource;
		else if (lowestPriNonLoopingSource) releasedSoundSource = lowestPriNonLoopingSource;
		else                                releasedSoundSource = lowestPriSource;

		NR_CORE_ASSERT(releasedSoundSource);

		releasedSoundSource->StopNow();

		// Need to make sure we remove released source from "starting sounds"
		// in case user tries to play more than max number of sounds in one call
		const auto startingSound = std::find_if(mSoundsToStart.begin(), mSoundsToStart.end(), [releasedSoundSource](const Sound* sound)
			{ return sound == releasedSoundSource; });

		if (startingSound != mSoundsToStart.end())
		{
			mSoundsToStart.erase(startingSound);

			LOG_VOICES("Voice released, ID {0}", releasedSoundSource->mSoundSourceID);
		}

		ReleaseFinishedSources();

#if DBG_VOICES
		NR_CORE_ASSERT(releasedSoundSource->mPlayState == Sound::ESoundPlayState::Stopped);
#endif
		NR_CORE_ASSERT(!mSourceManager.mFreeSourcIDs.empty());

		/* Other possible way:
		   1. check if any of the sources already Stopping
		   2. check the lowest priority
		   3. check lowes volume
		   4. check the farthest distance from the listener
		   5. check playback time left
		*/

		return releasedSoundSource;
	}

	bool AudioEngine::SubmitSoundToPlay(uint64_t audioComponentID)
	{
		NR_PROFILE_FUNC();

		auto* ac = GetAudioComponentFromID(mCurrentSceneID, audioComponentID);

		if (ac)
		{
			if (auto objectID = mComponentObjectMap.Get(mCurrentSceneID, audioComponentID))
			{
				return PostTrigger(ac->StartCommandID, *objectID);
			}
			else
			{
				NR_CORE_ASSERT(false, "AudioComponent doesn't have associated AudioObject!");
				return false;
			}
		}

		NR_CORE_ERROR("AudioComponent with ID {0} doesn't exist!", audioComponentID);
		return false;
	}

	bool AudioEngine::StopActiveSoundSource(uint64_t audioComponentID)
	{
		if (!mComponentObjectMap.Get(mCurrentSceneID, audioComponentID).has_value())
		{
			return false;
		}

		auto stopSound = [this, audioComponentID]
			{
				// Stop Active Sound Sources associated to the AudioComponent's AudioObject
				if (auto objectID = mComponentObjectMap.Get(mCurrentSceneID, audioComponentID))
				{
					if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(*objectID))
					{
						for (auto& soundID : *activeSoundIDs)
						{
							mSoundSources[soundID]->Stop();
						}
					}
				}
			};

		AudioThread::IsAudioThread() ? stopSound() : ExecuteOnAudioThread(stopSound, "StopSound");

		return true;
	}

	bool AudioEngine::PauseActiveSoundSource(uint64_t audioComponentID)
	{
		if (!mComponentObjectMap.Get(mCurrentSceneID, audioComponentID).has_value())
		{
			return false;
		}

		auto pauseSound = [this, audioComponentID]
			{
				//=== Audio Objects API

				// Pause Active Sound Sources associated to the AudioComponent's AudioObject

				if (auto objectID = mComponentObjectMap.Get(mCurrentSceneID, audioComponentID))
				{
					if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(*objectID))
					{
						for (auto& soundID : *activeSoundIDs)
							mSoundSources[soundID]->Pause();
					}
				}
			};

		AudioThread::IsAudioThread() ? pauseSound() : ExecuteOnAudioThread(pauseSound, "PauseSound");

		return true;
	}

	bool AudioEngine::ResumeActiveSoundSource(uint64_t audioComponentID)
	{
		if (!mComponentObjectMap.Get(mCurrentSceneID, audioComponentID).has_value())
			return false;


		auto resumeSound = [this, audioComponentID]
			{
				//=== Audio Objects API

				// Resume Active Sound Sources associated to the AudioComponent's AudioObject

				if (auto objectID = mComponentObjectMap.Get(mCurrentSceneID, audioComponentID))
				{
					if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(*objectID))
					{
						for (auto& soundID : *activeSoundIDs)
						{
							Sound* sound = mSoundSources[soundID];

							// Only re-start sound if it wasn explicitly paused
							if (sound->mPlayState == Sound::ESoundPlayState::Paused)
							{
								sound->Play();
							}
						}
					}
				}
			};

		AudioThread::IsAudioThread() ? resumeSound() : ExecuteOnAudioThread(resumeSound, "PauseSound");

		return true;
	}

	bool AudioEngine::StopEventID(EventID playingEvent)
	{
		if (mEventRegistry.GetNumberOfSources(playingEvent) <= 0)
		{
			// This might be unnecesary, but for the time being is useful for debugging.
			NR_CONSOLE_LOG_WARN("Audio. Trying to stop event that's not in the active events registry.");
			return false;
		}

		auto stopEventSources = [this, playingEvent]
			{
				for (auto& sourceID : mEventRegistry.Get(playingEvent).ActiveSources)
				{
					mSoundSources[sourceID]->Stop();
				}
			};

		AudioThread::IsAudioThread() ? stopEventSources() : ExecuteOnAudioThread(stopEventSources, "StopEventID");

		return true;
	}

	bool AudioEngine::PauseEventID(EventID playingEvent)
	{
		if (mEventRegistry.GetNumberOfSources(playingEvent) <= 0)
		{
			return false;
		}

		auto pauseEventSources = [this, playingEvent]
			{
				for (auto& sourceID : mEventRegistry.Get(playingEvent).ActiveSources)
				{
					mSoundSources[sourceID]->Pause();
				}
			};

		AudioThread::IsAudioThread() ? pauseEventSources() : ExecuteOnAudioThread(pauseEventSources, "PauseEventID");

		return true;
	}

	bool AudioEngine::ResumeEventID(EventID playingEvent)
	{
		if (mEventRegistry.GetNumberOfSources(playingEvent) <= 0)
		{
			return false;
		}

		auto resumeEventSources = [this, playingEvent]
			{
				for (auto& sourceID : mEventRegistry.Get(playingEvent).ActiveSources)
				{
					Sound* sound = mSoundSources[sourceID];

					// Only re-start sound if it wasn explicitly paused
					if (sound->mPlayState == Sound::ESoundPlayState::Paused)
					{
						sound->Play();
					}
				}
			};

		AudioThread::IsAudioThread() ? resumeEventSources() : ExecuteOnAudioThread(resumeEventSources, "ResumeEventID");

		return true;
	}

	bool AudioEngine::IsSoundForComponentPlaying(uint64_t audioComponentID)
	{
		//=== Audio Objects API
		if (auto objectID = mComponentObjectMap.Get(mCurrentSceneID, audioComponentID))
		{
			return mObjectEventsRegistry.GetNumberOfEvents(*objectID);
		}

		return false;
	}


	//==================================================================================

	// TODO: move this to SourceManager
	void AudioEngine::UpdateSources()
	{
		NR_PROFILE_FUNC();

		{
			// Bulk update all of the sources
			std::scoped_lock lock{ mUpdateSourcesLock };

			// 1. Update positioning of the AudioObjects.

			// TODO: test performance of this being in a separate loop vs merged with the 2.

			for (auto& data : mSourceUpdateData)
			{
				if (auto objectID = mComponentObjectMap.Get(mCurrentSceneID, data.entityID))
				{
					auto& audioObject = mAudioObjects.at(*objectID);
					audioObject.SetTransform(data.Transform); //? this sometimes out of range when switching scenes
					audioObject.SetVelocity(data.Velocity);
				}
			}

			// 2. Update Volume, Velocity and Pitch

			for (auto& data : mSourceUpdateData)
			{
				// TODO: update attached objects with an offset

				if (auto objectID = mComponentObjectMap.Get(mCurrentSceneID, data.entityID))
				{
					if (auto activeSources = mObjectSourceMap.GetActiveSounds(*objectID))
					{
						for (SourceID sourceID : *activeSources)
						{
							auto* sound = mSoundSources.at(sourceID);

							// TODO: instead of setting all of this info here, set update it from Events or Parameter controls?

							/*auto isVelocityValid = [](const glm::vec3& velocity) {
								auto inRange = [](auto x, auto low, auto high) {
									return ((x - high) * (x - low) <= 0);
								};

								constexpr float speedOfSound = 343.0f;
								return !(!inRange(velocity.x, -speedOfSound, speedOfSound)
									|| !inRange(velocity.y, -speedOfSound, speedOfSound)
									|| !inRange(velocity.z, -speedOfSound, speedOfSound));
							};

							if(!isVelocityValid(data.Velocity))
							{
								//NR_CORE_ASSERT(false);
								NR_CORE_ERROR("[AudioEngine] Invalid Velocity from Physics actor. ({0})", sound->mDebugName);
								//NR_CORE_ERROR("Velocity: {0}, {1}, {2}", data.Velocity.x, data.Velocity.y, data.Velocity.z);
							}*/

							// order for updating the values somewhat matching order of corresponding values of their backend data source
							sound->SetVolume(data.VolumeMultiplier);
							sound->SetPitch(data.PitchMultiplier);
							//sound->SetLocation(data.Position);
						}
					}
					else
					{
						//NR_CORE_ASSERT(false, "AudioObject associated to given AudioComponent doesn't have any Active Sources.");
					}
				}
				else
				{
					//NR_CORE_ASSERT(false, "There's no AudioObject associated to given AudioComponent.");
					//NR_CONSOLE_LOG_WARN("[AudioEngine] UpdateSources: There's no AudioObject associated to updating AudioComponent.");
				}
			}
		}

		// 3. Update position of the sound sources from associated AudioObjects, which can originate from AudioComponent or not.

		//ScopedTimer timer("UpdateSources");

		// PARALLEL
		NR_PROFILE_FUNC("AudioEngine::UpdateSources - USP Loop");

		std::for_each(std::execution::par, mAudioObjects.begin(), mAudioObjects.end(), [&](std::pair<const UUID, AudioObject>& IDObjectPair)
			{
				auto& [objectID, audioObject] = IDObjectPair;
				if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(objectID))
				{
					// TODO: don't update sources that do not requre update? Although still need to update all of the sources if Listener position has changed
					for (auto& sourceID : *activeSoundIDs)
					{
						mSourceManager.mSpatializer->UpdateSourcePosition(sourceID, audioObject.GetTransform(), audioObject.GetVelocity());
					}
				}

			});
	}

	void AudioEngine::SubmitSourceUpdateData(std::vector<SoundSourceUpdateData> updateData)
	{
		{
			std::scoped_lock lock{ mUpdateSourcesLock };
			mSourceUpdateData.swap(updateData);
		}
	}

	void AudioEngine::UpdateListener()
	{
		if (mAudioListener.HasChanged(true))
		{
			Audio::Transform transform = mAudioListener.GetPositionDirection();
			glm::vec3 vel;
			mAudioListener.GetVelocity(vel);
			ma_engine_listener_set_position(&mEngine, 0, transform.Position.x, transform.Position.y, transform.Position.z);
			ma_engine_listener_set_direction(&mEngine, 0, transform.Orientation.x, transform.Orientation.y, transform.Orientation.z);
			ma_engine_listener_set_world_up(&mEngine, 0, transform.Up.x, transform.Up.y, transform.Up.z);
			ma_engine_listener_set_velocity(&mEngine, 0, vel.x, vel.y, vel.z);

			auto [innerAngle, outerAngle] = mAudioListener.GetConeInnerOuterAngles();
			float outerGain = mAudioListener.GetConeOuterGain();
			ma_engine_listener_set_cone(&mEngine, 0, innerAngle, outerAngle, outerGain); //? this is not thread safe internally


			// Need to update all Spatializer sources when listener position changes
			mSourceManager.mSpatializer->UpdateListener(transform, vel);
		}
	}

	void AudioEngine::UpdateListenerPosition(const Audio::Transform& newTransform)
	{
		if (mAudioListener.PositionNeedsUpdate(newTransform))
		{
			mAudioListener.SetNewPositionDirection(newTransform);
		}
	}

	void AudioEngine::UpdateListenerVelocity(const glm::vec3& newVelocity)
	{
		auto isVelocityValid = [](const glm::vec3& velocity) {
			auto inRange = [](auto x, auto low, auto high) {
				return ((x - high) * (x - low) <= 0);
				};

			constexpr float speedOfSound = 343.0f;
			return !(!inRange(velocity.x, -speedOfSound, speedOfSound)
				|| !inRange(velocity.y, -speedOfSound, speedOfSound)
				|| !inRange(velocity.z, -speedOfSound, speedOfSound)); };

		//if (isVelocityValid(newVelocity))

		NR_CORE_ASSERT(isVelocityValid(newVelocity));

		mAudioListener.SetNewVelocity(newVelocity);
	}

	void AudioEngine::UpdateListenerConeAttenuation(float innerAngle, float outerAngle, float outerGain)
	{
		mAudioListener.SetNewConeAngles(innerAngle, outerAngle, outerGain);
	}

	void AudioEngine::ReleaseFinishedSources()
	{
		NR_PROFILE_FUNC();

		for (int i = (int)mActiveSounds.size() - 1; i >= 0; --i)
		{
			if (Sound* source = dynamic_cast<Sound*>(mActiveSounds[i]))
			{
				if (source->IsFinished())
				{
					mActiveSounds.erase(mActiveSounds.begin() + i);

					// If AudioComponentID is 0, this sound is associated to an AudioObject from the new API, not to an AudioComponent
					const uint64_t objectID = source->mAudioObjectID;
					if (objectID != 0)
					{
						mObjectSourceMap.Remove(objectID, source->mSoundSourceID);

						// If this EventID doesn't have any more Sources playing, remove SourceID from EventsRegistry
						if (mEventRegistry.RemoveSource(source->mEventID, source->mSoundSourceID))
						{
							// If all Actions of the Event were handled (e.g. this Play Action was the last one),
							// remove Event from the object registry.

							auto info = mEventRegistry.Get(source->mEventID);
							TriggerCommand& trigger = std::get<TriggerCommand>(*info.CommandState);
							auto& actions = trigger.Actions.GetVector();
							bool allActionsHandled = std::all_of(actions.begin(), actions.end(),
								[](const TriggerAction& action) {return action.Handled; });

							if (allActionsHandled)
							{
								mObjectEventsRegistry.Remove(objectID, source->mEventID);
								mEventRegistry.Remove(source->mEventID);

								std::scoped_lock lock{ sStats.mutex };
								sStats.ActiveEvents = mEventRegistry.Count();
							}
						}

						// If the audio object was released by the user while associated sound was playing,
						// we can release the object now.
						{
							std::scoped_lock lock{ mObjectsLock };
							if (auto* object = GetAudioObject(objectID).value_or(nullptr))
							{
								if (object->mReleased)
									ReleaseAudioObject(objectID);
							}
						}
					}
					else
					{
						NR_CORE_ASSERT(false, "Sound Source must have had associated ObjectID assigned!");
					}

					// Return Sound Source for reuse
					mSourceManager.ReleaseSource(source->mSoundSourceID);

					LOG_VOICES("FreeSourceIDs pushed ID {0}", source->mSoundSourceID);

					{
						std::scoped_lock lock{ sStats.mutex };
						sStats.NumActiveSounds = (uint32_t)mActiveSounds.size();
					}
				}
			}
		}
	}

	void AudioEngine::Update(float dt)
	{
		// AudioThread tasks handled before this Update function

		if (mPlaybackState == EPlaybackState::Playing)
		{

			//---  Execute queued commands ---
			//--------------------------------

			for (int i = (int)mCommandQueue.size() - 1; i >= 0; i--)
			{
				auto& [type, info] = mCommandQueue.front();

				// Flag to add a Command back to the queue and process/complete later
				// Play Action is exception, because it's handled when the source playback is complete.
				bool commandHandled = true;

				switch (type)
				{
				case ECommandType::Trigger:
				{
					// Get persistent state object for this Event
					TriggerCommand& trigger = std::get<TriggerCommand>(*info.CommandState);

					//? should this be for the whole Event, or per Action?
					if (trigger.DelayExecution)
					{
						trigger.DelayExecution = false;
					}

					for (auto& action : trigger.Actions.GetVector())
					{
						if (!action.Target && action.Type != EActionType::StopAll
							&& action.Type != EActionType::PauseAll
							&& action.Type != EActionType::ResumeAll
							&& action.Type != EActionType::SeekAll)
						{
							NR_CORE_ERROR("One of the actions of audio trigger ({0}) doesn't have a target assigned!", trigger.DebugName);
							action.Handled = true;
							continue;
						}

						//! in the future migh want some Actions to be able to ignore this flag
						if (trigger.DelayExecution)
						{
							break;
						}

						// If an Action is already handled, this means we have delayed the execution of some other Action of this Event
						// We need to process only unhandled Actions.
						if (action.Handled)
						{
							continue;
						}


						/* TODO:  Handle Global context type - action.Context
									 If the context is Global, execute this Action to this target on all of the AudioObjects
						   */
						switch (action.Type)
						{
						case EActionType::Play:
							// Play action.Target on the Context object
							//NR_CORE_ASSERT(mActiveSounds.size() < mNumSources && mSoundsToStart.size() < mNumSources);
							if (auto* sound = InitiateNewVoice(info.ObjectID, info.EventID, action.Target))
							{
								Audio::Transform spawnLocation;
								{
									//? not ideal
									std::shared_lock lock{ mObjectsLock };
									spawnLocation = mAudioObjects.at(info.ObjectID).GetTransform();
								}

								// Set spawn location
								//sound->SetLocation(spawnLocation.Position, spawnLocation.Orientation);
								mSourceManager.mSpatializer->UpdateSourcePosition(sound->mSoundSourceID, spawnLocation);

								mEventRegistry.AddSource(info.EventID, sound->mSoundSourceID);
								mSoundsToStart.push_back(sound);

								LOG_EVENTS("GameObject: Source PLAY");
							}
							else NR_CORE_ERROR("Failed to initialize new Voice for audio object");

							action.Handled = true;

							break;
						case EActionType::Pause:

							if (action.Context == EActionContext::GameObject)
							{
								// Pause target on the context object
								if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(info.ObjectID))
								{
									for (auto& sourceID : *activeSoundIDs)
									{
										auto* sound = mSoundSources.at(sourceID);
										if (sound->mSoundConfig == action.Target)
										{
											// If Play was called on this sound withing this Event (or within this Update),
											// need to remove it from Starting Sounds
											// TODO: wrap in a function?
											auto it = std::find(mSoundsToStart.begin(), mSoundsToStart.end(), sound);
											if (it != mSoundsToStart.end())
												mSoundsToStart.erase(it);

											sound->Pause();
										}
										LOG_EVENTS("GameObject: Source PAUSE");
									}
								}
								else NR_CORE_ERROR("No Active Sources for object to Pause");
							}
							else // --- Global context
							{
								for (auto* sound : mActiveSounds)
								{
									if (sound->mSoundConfig == action.Target)
									{
										// If Play was called on this sound withing this Event (or within this Update),
										// need to remove it from Starting Sounds
										// TODO: wrap in a function?
										auto it = std::find(mSoundsToStart.begin(), mSoundsToStart.end(), sound);
										if (it != mSoundsToStart.end())
											mSoundsToStart.erase(it);

										sound->Pause();
									}

									LOG_EVENTS("Global: Source PAUSE");
								}
							}

							action.Handled = true;
							break;
						case EActionType::Resume:

							if (action.Context == EActionContext::GameObject)
							{
								// Resume target on the context object
								if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(info.ObjectID))
								{
									action.Handled = true;

									for (auto& sourceID : *activeSoundIDs)
									{
										auto* sound = mSoundSources.at(sourceID);
										if (sound->mSoundConfig == action.Target)
										{
											// Making sure we only re-start sound if it wasn explicitly paused
											if (sound->mPlayState == Sound::ESoundPlayState::Paused)
											{
												sound->Play();
											}
											else if (sound->mPlayState == Sound::ESoundPlayState::Pausing)
											{
												//? This is for testing for now, but should probably be handled in Sound object.
												//? In fact, the sound could just reverse fade back to its normal volume level.
												//? Or straight up cancel the pausing.

												// If the sound is Pausing, delay the Resume Action and let its stop-fade to finish
												action.Handled = false;
												commandHandled = false;
												trigger.DelayExecution = true;
											}
										}
										LOG_EVENTS("GameObject: Source RESUME");
									}
								}
								else
								{
									NR_CORE_ERROR("No Active Sources for object to Resume");
									action.Handled = true;
								}
							}
							else // --- Global context
							{
								action.Handled = true;

								for (auto* sound : mActiveSounds)
								{
									if (sound->mSoundConfig == action.Target)
									{
										// Making sure we only re-start sound if it wasn explicitly paused
										if (sound->mPlayState == Sound::ESoundPlayState::Paused)
										{
											sound->Play();
										}
										else if (sound->mPlayState == Sound::ESoundPlayState::Pausing)
										{
											//? This is for testing for now, but should probably be handled in Sound object.

											//? This might not work for resuming multiple sources
											action.Handled = false;
											commandHandled = false;
											trigger.DelayExecution = true;
										}
									}

									LOG_EVENTS("Global: Source RESUME");
								}
							}

							break;
						case EActionType::Stop:

							if (action.Context == EActionContext::GameObject)
							{
								// Stop target on the context object
								if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(info.ObjectID))
								{
									for (auto& sourceID : *activeSoundIDs)
									{
										auto* sound = mSoundSources.at(sourceID);
										if (sound->mSoundConfig == action.Target)
										{
											// If Play was called on this sound withing this Event (or within this Update),
											// need to remove it from Starting Sounds
											// TODO: wrap in a function?
											auto it = std::find(mSoundsToStart.begin(), mSoundsToStart.end(), sound);
											if (it != mSoundsToStart.end())
												mSoundsToStart.erase(it);

											// Still calling Stop to flag it "Finished" and free resources / put it back in pool
											sound->Stop();
										}
										LOG_EVENTS("GameObject: Source STOP");
									}
								}
								else NR_CORE_ERROR("No Active Sources for object to Stop");
							}
							else // --- Global context
							{
								for (auto* sound : mActiveSounds)
								{
									if (sound->mSoundConfig == action.Target)
									{
										// If Play was called on this sound withing this Event (or within this Update),
										// need to remove it from Starting Sounds
										// TODO: wrap in a function?
										auto it = std::find(mSoundsToStart.begin(), mSoundsToStart.end(), sound);
										if (it != mSoundsToStart.end())
											mSoundsToStart.erase(it);

										// Still calling Stop to flag it "Finished" and free resources / put it back in pool
										sound->Stop();
									}
									LOG_EVENTS("Global: Source STOP");
								}
							}

							action.Handled = true;
							break;
						case EActionType::StopAll:

							if (action.Context == EActionContext::GameObject)
							{
								if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(info.ObjectID))
								{
									for (auto& sourceID : *activeSoundIDs)
									{
										auto* sound = mSoundSources.at(sourceID);

										auto it = std::find(mSoundsToStart.begin(), mSoundsToStart.end(), sound);
										if (it != mSoundsToStart.end())
											mSoundsToStart.erase(it);

										// Still calling Stop to flag it "Finished" and free resources / put it back in pool
										sound->Stop();
									}
								}

								LOG_EVENTS("GameObject: StopAll");
							}
							else
							{
								mSoundsToStart.clear();
								for (auto* sound : mActiveSounds)
								{
									sound->Stop();
								}
								LOG_EVENTS("Global: StopAll");
							}

							action.Handled = true;
							break;
						case EActionType::PauseAll:

							if (action.Context == EActionContext::GameObject)
							{
								if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(info.ObjectID))
								{
									for (auto& sourceID : *activeSoundIDs)
									{
										auto* sound = mSoundSources.at(sourceID);

										// If Play was called on this sound withing this Event (or within this Update),
										// need to remove it from Starting Sounds
										auto it = std::find(mSoundsToStart.begin(), mSoundsToStart.end(), sound);
										if (it != mSoundsToStart.end())
											mSoundsToStart.erase(it);

										sound->Pause();
									}
								}

								LOG_EVENTS("GameObject: PauseAll");
							}
							else
							{
								mSoundsToStart.clear();
								for (auto* sound : mActiveSounds)
								{
									sound->Pause();
								}
								LOG_EVENTS("Global: PauseAll");
							}

							action.Handled = true;
							break;
						case EActionType::ResumeAll:

							if (action.Context == EActionContext::GameObject)
							{
								if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(info.ObjectID))
								{
									action.Handled = true;

									for (auto& sourceID : *activeSoundIDs)
									{
										auto* sound = mSoundSources.at(sourceID);

										// Making sure we only re-start sound if it wasn explicitly paused
										if (sound->mPlayState == Sound::ESoundPlayState::Paused)
										{
											sound->Play();
										}
										else if (sound->mPlayState == Sound::ESoundPlayState::Pausing)
										{
											//? This is for testing for now, but should probably be handled in Sound object.

											//? This might not work for resuming multiple sources
											action.Handled = false;
											commandHandled = false;
											trigger.DelayExecution = true;
										}
									}
								}

								LOG_EVENTS("GameObject: ResumeAll");
							}
							else
							{
								action.Handled = true;

								for (auto* sound : mActiveSounds)
								{
									// Making sure we only re-start sound if it wasn explicitly paused
									if (sound->mPlayState == Sound::ESoundPlayState::Paused)
									{
										sound->Play();
									}
									else if (sound->mPlayState == Sound::ESoundPlayState::Pausing)
									{
										//? This is for testing for now, but should probably be handled in Sound object.

										//? This might not work for resuming multiple sources
										action.Handled = false;
										commandHandled = false;
										trigger.DelayExecution = true;
									}
								}
								LOG_EVENTS("Global: ResumeAll");
							}

							break;
						default:
							action.Handled = true;
							break;
						}

						/* TODO: if action.Handled == false (e.g. delayed execution, or interpolating Action)
									tick timer and, or push back to the queue
							*/
					}

					// Checking if != Play allows to push the Command back to the queue to handle later
					// and prevent calling other Play Actions again if their sources are still playing.
					auto& actions = trigger.Actions.GetVector();
					bool allActionsHandled = std::all_of(actions.begin(), actions.end(),
						[](const TriggerAction& action) {return action.Handled && action.Type != EActionType::Play; });

					// If all Actions have been handled, remove Event from the registry
					if (allActionsHandled)
					{
						mObjectEventsRegistry.Remove(info.ObjectID, info.EventID);
						mEventRegistry.Remove(info.EventID);

						std::scoped_lock lock{ sStats.mutex };
						sStats.ActiveEvents = mEventRegistry.Count();
					}

					break;
				}
				case ECommandType::Switch:
					break;
				case ECommandType::State:
					break;
				case ECommandType::Parameter:
					break;
				default:
					break;
				}

				// TODO: Events might require timestep for interpolation of some kind,
				//       or some other time tracking mechanism.
				if (!commandHandled)
				{
					mCommandQueue.push(mCommandQueue.front());
				}

				mCommandQueue.pop();
			}


			//--- AudioEngine Update ---
			//--------------------------
			UpdateListener();

			UpdateSources();

			// Start sounds requested to start
			for (auto* sound : mSoundsToStart)
			{
				sound->Play();
				{
					std::scoped_lock lock{ sStats.mutex };
					sStats.NumActiveSounds = (uint32_t)mActiveSounds.size();
				}
			}
			mSoundsToStart.clear();

			for (auto* sound : mActiveSounds)
				sound->Update(dt);

		}
		else if (mPlaybackState == EPlaybackState::Paused)
		{
			const bool pausingOrStopping = std::any_of(mActiveSounds.begin(), mActiveSounds.end(), [](const Sound* sound)
				{
					return (sound->mPlayState == Sound::ESoundPlayState::Pausing)
						|| (sound->mPlayState == Sound::ESoundPlayState::Stopping);
				});

			// If any sound is still stopping, or pausing, continue to Update
			if (pausingOrStopping)
			{
				for (auto* sound : mActiveSounds)
					sound->Update(dt);
			}
		}

		ReleaseFinishedSources();
	}

	void AudioEngine::RegisterNewListener(AudioListenerComponent& listenerComponent)
	{
		// TODO
	}

	//==================================================================================

	Sound* AudioEngine::InitiateNewVoice(UUID objectID, EventID eventID, const Ref<SoundConfig>& sourceConfig)
	{
		NR_CORE_ASSERT(AudioThread::IsAudioThread());

		auto sceneID = mCurrentSceneID;

		// TODO: For now just handling basic Sounds, more complex SoundObjects later
		Sound* sound = nullptr;

		int freeID;
		if (!mSourceManager.GetFreeSourceId(freeID))
		{
			// Stop lowest priority source
			FreeLowestPrioritySource();
			mSourceManager.GetFreeSourceId(freeID);
			LOG_VOICES("Got released voice ID {0}", freeID);
		}

		sound = mSoundSources.at(freeID);

		// Associate Voice with the Objcet
		sound->mAudioObjectID = objectID;
		sound->mEventID = eventID;

		//? it is not ideal to associate sound with scene this way, but it works like a charm
		sound->mSceneID = sceneID;

		//? this is weird for now, because SourceManager calls back AudioEngine to init get the source by ID
		// TODO: move mSoundSources to SourceManager
		if (!mSourceManager.InitializeSource(freeID, sourceConfig))
		{
			// TODO: release resource if failed to initialize
			NR_CORE_ASSERT(false, "Failed to initialize sound source!");
			return nullptr;
		}

		//? not ideal, but needed to clear the active event triggered playback of this sound
		sound->mSoundConfig = sourceConfig;

		mObjectSourceMap.Add(objectID, freeID);
		mActiveSounds.push_back(sound);

		return sound;
	}

	UUID AudioEngine::InitializeAudioObject(UUID objectID, const std::string& debugName, const Audio::Transform& objectPosition /*= Audio::Transform()*/)
	{
		size_t numObjects;
		{
			//? not ideal because at the start of a scene game might request a large number of objects to be initialized,
			//? which would casue this lock to be repeatedly called over and over
			//? 200 objects will take around 5 milliseconds just locking

			std::scoped_lock lock{ mObjectsLock };

			auto* object = &mAudioObjects.emplace(std::make_pair(objectID, AudioObject(objectID, debugName))).first->second;
			object->SetTransform(objectPosition);
			numObjects = mAudioObjects.size();
		}

		{
			std::scoped_lock lock{ sStats.mutex };
			sStats.AudioObjects = (uint32_t)numObjects;
		}

		return objectID;
	}

	void AudioEngine::ReleaseAudioObject(UUID objectID)
	{
		auto releaseObject = [this, objectID]
			{
				// If there is an active Event referencing object, mark the object to be release later
				if (mObjectEventsRegistry.GetNumberOfEvents(objectID) > 0)
				{
					mAudioObjects.at(objectID).mReleased = true;
				}
				else
				{
					mAudioObjects.erase(objectID);
					LOG_EVENTS("Object with ID {0} released.", objectID);
					{
						std::scoped_lock lock{ sStats.mutex };
						sStats.AudioObjects = (uint32_t)mAudioObjects.size();
					}
				}
			};

		AudioThread::IsAudioThread() ? releaseObject() : ExecuteOnAudioThread(releaseObject, "Release Object");
	}

	//? make sure this is called thread safely
	std::optional<AudioObject*> AudioEngine::GetAudioObject(uint64_t objectID)
	{
		if (mAudioObjects.find(objectID) == mAudioObjects.end())
		{
			NR_CORE_ASSERT(false, "Object with ID " + std::to_string(objectID) + " was not found.");
			return {};
		}
		else
		{
			return &mAudioObjects.at(objectID); // TODO: replace with safe Registry interface
		}
	}

	bool AudioEngine::SetTransform(uint64_t objectID, const Audio::Transform& objectPosition)
	{
		std::scoped_lock lock{ mObjectsLock };

		if (mAudioObjects.find(objectID) == mAudioObjects.end())
		{
			return false;
		}

		mAudioObjects.at(objectID).SetTransform(objectPosition);

		return true;
	}

	std::optional<Audio::Transform> AudioEngine::GetTransform(uint64_t objectID)
	{
		std::shared_lock lock{ mObjectsLock };
		if (mAudioObjects.find(objectID) == mAudioObjects.end())
		{
			return {};
		}

		return mAudioObjects.at(objectID).GetTransform();
	}

	std::string AudioEngine::GetAudioObjectInfo(uint64_t objectID)
	{
		std::shared_lock lock{ mObjectsLock };

		if (mAudioObjects.find(objectID) == mAudioObjects.end())
		{
			return "";
		}

		return mAudioObjects.at(objectID).GetDebugName();
	}

	UUID AudioEngine::GetAudioObjectOfComponent(uint64_t audioComponentID)
	{
		if (auto id = mComponentObjectMap.Get(mCurrentSceneID, audioComponentID))
		{
			return *id;
		}

		return 0;
	}

	void AudioEngine::AttachObject(uint64_t audioComponentID, uint64_t audioObjectID, const glm::vec3& PositionOffset)
	{

	}

	EventID AudioEngine::PostTrigger(CommandID triggerCommandID, UUID objectID)
	{
		if (triggerCommandID == 0)
		{
			NR_CORE_ERROR("Audio: PostTrigger with empty triggerCommandID.");
			return 0;
		}

		auto& command = AudioCommandRegistry::GetCommand<TriggerCommand>(triggerCommandID);
		LOG_EVENTS("Posting audio trigger event: {0}", command.DebugName);

		{
			std::shared_lock lock{ mObjectsLock };

			if (!GetAudioObject(objectID))
			{
				NR_CORE_ERROR("Audio: PostTrigger. Object with ID {0} does not exist.", objectID);
				return 0;
			}
		}

		EventInfo eventInfo(triggerCommandID, objectID);

		// Add state object to the EventInfo before passing it to the registry
		eventInfo.CommandState = std::make_shared<EventInfo::StateObject>(AudioCommandRegistry::GetCommand<TriggerCommand>(triggerCommandID));

		EventID eventID = mEventRegistry.Add(eventInfo);

		auto postTrigger = [this, triggerCommandID, objectID, eventID, eventInfo]() mutable
			{
				if (!mObjectEventsRegistry.Add(objectID, eventID))
				{
					// Posting Event on object that already has associated Event playing
				}

				{
					std::scoped_lock lock{ sStats.mutex };
					sStats.ActiveEvents = mEventRegistry.Count();
				}

				// Push command to the queue.
				mCommandQueue.push({ ECommandType::Trigger, eventInfo }); // TODO: alternative option is to push playbackID and retrieve EventInfo in the Update()
			};

		AudioThread::IsAudioThread() ? postTrigger() : ExecuteOnAudioThread(postTrigger, "Post Trigger");

		return eventID;
	}

	//==================================================================================

	Stats AudioEngine::GetStats()
	{
		{
			std::scoped_lock lock{ sStats.mutex };
			sStats.FrameTime = AudioThread::GetFrameTime();
		}
		return AudioEngine::sStats;
	}

	void AudioEngine::StopAll(bool stopNow /*= false*/)
	{
		auto stopAll = [&, stopNow]
			{
				mSoundsToStart.clear();

				for (auto* sound : mActiveSounds)
				{
					if (stopNow)
						dynamic_cast<Sound*>(sound)->StopNow(true, true);
					else
						sound->Stop();
				}
			};

		AudioThread::IsAudioThread() ? stopAll() : ExecuteOnAudioThread(stopAll, "StopAll");
	}

	void AudioEngine::PauseAll()
	{
		if (mPlaybackState != EPlaybackState::Playing)
		{
			NR_CORE_ASSERT(false, "Audio Engine is already paused.");
			return;
		}

		auto pauseAll = [&]
			{
				mPlaybackState = EPlaybackState::Paused;

				for (auto* sound : mActiveSounds)
				{
					sound->Pause();
				}
			};

		AudioThread::IsAudioThread() ? pauseAll() : ExecuteOnAudioThread(pauseAll, "PauseAudioEngine");
	}

	void AudioEngine::ResumeAll()
	{
		if (mPlaybackState != EPlaybackState::Paused)
		{
			NR_CORE_ASSERT(false, "Audio Engine is already active.");
			return;
		}

		auto resumeAll = [&]
			{
				mPlaybackState = EPlaybackState::Playing;

				for (auto* sound : mActiveSounds)
				{
					// Making sure we only re-start sound if it wasn explicitly paused
					if (sound->mPlayState == Sound::ESoundPlayState::Paused)
					{
						sound->Play();
					}
					else if (sound->mPlayState == Sound::ESoundPlayState::Pausing)
					{
						sound->Play();
					}
				}
			};

		AudioThread::IsAudioThread() ? resumeAll() : ExecuteOnAudioThread(resumeAll, "ResumeAudioEngine");
	}

	void AudioEngine::SetSceneContext(const Ref<Scene>& scene)
	{
		auto& audioEngine = Get();

		audioEngine.StopAll();

		//? A bit of a hack to handle changing scenes in Editor context.
		//? OnSceneDestruct is not called when creating new scene, or loading a different scene.
		OnSceneDestruct(audioEngine.mCurrentSceneID);

		audioEngine.mSceneContext = scene;
		if (scene) {
			auto* newScene = audioEngine.mSceneContext.Raw();
			const auto newSceneID = newScene->GetID();
			audioEngine.mCurrentSceneID = newSceneID;

			{
				std::scoped_lock lock{ sStats.mutex };
				sStats.NumAudioComps = sInstance->mAudioComponentRegistry.Count(newSceneID);
			}

			auto view = newScene->GetAllEntitiesWith<AudioComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, newScene };
				auto& audioComp = e.GetComponent<AudioComponent>();

				sInstance->RegisterAudioComponent(e);
			}
		}
		else
		{
			NR_CORE_ASSERT(false);
		}

		//NR_CORE_INFO("SET SCENE CONTEXT");
	}

	void AudioEngine::OnRuntimePlaying(UUID sceneID)
	{
		auto& audioEngine = Get();

		const auto currentSceneID = audioEngine.mCurrentSceneID;
		NR_CORE_ASSERT(currentSceneID == sceneID);

		auto newScene = Scene::GetScene(currentSceneID);

		auto view = newScene->GetAllEntitiesWith<AudioComponent>();
		for (auto entity : view)
		{
			Entity audioEntity = { entity, newScene.Raw() };

			auto& ac = audioEntity.GetComponent<AudioComponent>();
			if (ac.PlayOnAwake)
			{
				auto newScene = Scene::GetScene(currentSceneID);
				// TODO: Play on awake! Duck-taped.
				//       Proper implementation requires enablement state for components.
				//       For now just checking if this component was created in Runtime scene.
				if (!newScene->IsEditorScene() && newScene->IsPlaying())
				{
					auto translation = newScene->GetWorldSpaceTransform(audioEntity).Translation;
					////ac.SourcePosition = translation;
					////ac.SoundConfig.SpawnLocation = translation;
					audioEngine.SubmitSoundToPlay(audioEntity.GetID());
				}
			}
		}

		//NR_CORE_INFO("ON RUNTIME PLAYING");
	}

	void AudioEngine::OnSceneDestruct(UUID sceneID)
	{
		auto& instance = Get();

		if (auto sceneObjects = instance.mComponentObjectMap.Get(sceneID))
		{
			for (auto& [comp, objectID] : *sceneObjects)
			{
				instance.ReleaseAudioObject(objectID);
			}
		}
		instance.mComponentObjectMap.Clear(sceneID);
		instance.mAudioComponentRegistry.Clear(sceneID);

		//NR_CORE_INFO("ON SCENE DESTRUCT");
	}

	Ref<Scene>& AudioEngine::GetCurrentSceneContext()
	{
		return Get().mSceneContext;
	}

	//==================================================================================

	AudioComponent* AudioEngine::GetAudioComponentFromID(UUID sceneID, uint64_t audioComponentID)
	{
		return mAudioComponentRegistry.GetAudioComponent(sceneID, audioComponentID);
	}

	void AudioEngine::RegisterAudioComponent(Entity audioEntity)
	{
		const auto sceneID = audioEntity.GetSceneID();
		mAudioComponentRegistry.Add(sceneID, audioEntity.GetID(), audioEntity);
		{
			std::scoped_lock lock{ sStats.mutex };
			sStats.NumAudioComps = mAudioComponentRegistry.Count(sceneID);
		}

		//====================================================
		//=== Audio Objects API
		const auto entityID = audioEntity.GetID();
		const auto objectID = UUID();

		if (mComponentObjectMap.Get(sceneID, entityID))
		{
			//NR_CORE_ASSERT(false, "AudioComponent already has AudioObject!");
			return;
		}

		//? GetWorldSpaceTransform doesn't work here, because it can't find any Entities by ID,
		//? which it tries to do to traverse parent hierarchy
		//auto transform = mSceneContext->GetWorldSpaceTransform(audioEntity);
		auto& transform = audioEntity.Transform();
		InitializeAudioObject(objectID, "AC Object", Audio::Transform{ transform.Translation, transform.Rotation });

		mComponentObjectMap.Add(sceneID, entityID, objectID);

		//====================================================

		//=== Handle Play on Awake
		// TODO: refactor to work with Audio Objects API

		////uint64_t entityID = audioEntity.GetID();
		auto* ac = GetAudioComponentFromID(sceneID, entityID);
		if (ac->PlayOnAwake)
		{
			auto newScene = Scene::GetScene(sceneID);
			// TODO: Play on awake! Duck-taped.
			//       Proper implementation requires enablement state for components.
			//       For now just checking if this component was created in Runtime scene.

			// This is going to be 'true' only when new AucioComponents added during play of a scene,
			// skipped when scenes are copied.
			if (!newScene->IsEditorScene() && newScene->IsPlaying())
			{
				auto translation = newScene->GetWorldSpaceTransform(audioEntity).Translation;
				////ac->SourcePosition = translation;
				////ac->SoundConfig.SpawnLocation = translation;
				SubmitSoundToPlay(entityID);
			}
		}
	}

	void AudioEngine::UnregisterAudioComponent(UUID sceneID, UUID entityID)
	{
		bool stopPlayback = true;
		if (auto ac = mAudioComponentRegistry.Get(sceneID, entityID))
		{
			stopPlayback = ac->GetComponent<AudioComponent>().bStopIfEntityDestroyed;
			mAudioComponentRegistry.Remove(sceneID, entityID);
		}
		{
			std::scoped_lock lock{ sStats.mutex };
			sStats.NumAudioComps = mAudioComponentRegistry.Count(sceneID);
		}

		if (auto objectID = mComponentObjectMap.Get(sceneID, entityID))
		{
			if (stopPlayback)
			{
				auto stopSources = [this, objectID]
					{
						if (auto activeSoundIDs = mObjectSourceMap.GetActiveSounds(*objectID))
						{
							for (auto& soundID : *activeSoundIDs)
								mSoundSources[soundID]->Stop();
						}
					};

				AudioThread::IsAudioThread() ? stopSources() : ExecuteOnAudioThread(stopSources, "StopSources");
			}

			ReleaseAudioObject(*objectID);
			mComponentObjectMap.Remove(sceneID, entityID);
		}
	}

	void AudioEngine::SerializeSceneAudio(YAML::Emitter& out, const Ref<Scene>& scene)
	{
		out << YAML::Key << "SceneAudio";
		out << YAML::BeginMap; // SceneAudio
		if (mMasterReverb)
		{
			out << YAML::Key << "MasterReverb";
			out << YAML::BeginMap; // MasterReverb
			auto storeParameter = [this, &out](DSP::EReverbParameters type)
				{
					out << YAML::Key << mMasterReverb->GetParameterName(type) << YAML::Value << mMasterReverb->GetParameter(type);
				};

			storeParameter(DSP::EReverbParameters::PreDelay);
			storeParameter(DSP::EReverbParameters::RoomSize);
			storeParameter(DSP::EReverbParameters::Damp);
			storeParameter(DSP::EReverbParameters::Width);

			out << YAML::EndMap; // MasterReverb
		}
		out << YAML::EndMap; // SceneAudio
	}

	void AudioEngine::DeserializeSceneAudio(YAML::Node& data)
	{
		auto masterReverb = data["MasterReverb"];

		if (masterReverb && mMasterReverb)
		{
			auto setParam = [this, masterReverb](DSP::EReverbParameters type)
				{
					mMasterReverb->SetParameter(type, masterReverb[mMasterReverb->GetParameterName(type)].as<float>());
				};

			setParam(DSP::EReverbParameters::PreDelay);
			setParam(DSP::EReverbParameters::RoomSize);
			setParam(DSP::EReverbParameters::Damp);
			setParam(DSP::EReverbParameters::Width);

		}
	}

} // namespace