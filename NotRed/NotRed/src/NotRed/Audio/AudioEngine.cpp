#include "nrpch.h"
#include "AudioEngine.h"

#include "DSP/Reverb/Reverb.h"

#include "yaml-cpp/yaml.h"

namespace NR::Audio
{
	AudioEngine* AudioEngine::sInstance = nullptr;
	Stats AudioEngine::sStats;

	void AudioEngine::ExecuteOnAudioThread(AudioThreadCallbackFunction func, const char* jobID/* = "NONE"*/)
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
			if (alData->isResourceManager) 
			{
				alData->Stats.MemResManager -= *sizeBox;
			}
			else                           
			{
				alData->Stats.MemEngine -= *sizeBox;
			}
		}

		std::free(buffer);
	}

	void* MemAllocCallback(size_t sz, void* pUserData)
	{
		constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

		char* buffer = (char*)malloc(sz + offset); //allocate offset extra bytes 
		if (buffer == NULL)
		{
			return NULL;
		}

		auto* alData = (AllocationCallbackData*)pUserData;
		{
			std::scoped_lock lock{ alData->Stats.mutex };
			if (alData->isResourceManager) 
			{
				alData->Stats.MemResManager += sz;
			}
			else                           
			{
				alData->Stats.MemEngine += sz;
			}
		}

		int* sizeBox = (int*)buffer;
		*sizeBox = sz; //store the size in first sizeof(int) bytes!

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
			if (alData->isResourceManager) 
			{
				alData->Stats.MemResManager += sz - *sizeBox;
			}
			else                           
			{
				alData->Stats.MemEngine += sz - *sizeBox;
			}
		}

		*sizeBox = sz;

		auto* newBuffer = (char*)ma_realloc(buffer, sz, NULL);
		if (newBuffer == NULL)
		{
			return NULL;
		}

		return newBuffer + offset;
	}

	void MALogCallback(ma_context* pContext, ma_device* pDevice, ma_uint32 logLevel, const char* message)
	{
		NR_CORE_INFO(message);
	}

	AudioEngine::AudioEngine()
	{
		AudioThread::BindUpdateFunction([this](TimeFrame dt) { Update(dt); });
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
		return;
		NR_CORE_ASSERT(sInstance, "Audio Engine was not initialized.");
		sInstance->Uninitialize();
		delete sInstance;
		sInstance = nullptr;
	}

	bool AudioEngine::Initialize()
	{
		if (true || bInitialized)
		{
			return true;
		}

		ma_result result;
		ma_engine_config engineConfig = ma_engine_config_init();

		ma_allocation_callbacks allocationCallbacks{ &mEngineCallbackData, &MemAllocCallback, &MemReallocCallback, &MemFreeCallback };
		engineConfig.allocationCallbacks = allocationCallbacks;

		result = ma_engine_init(&engineConfig, &mEngine);
		if (result != MA_SUCCESS)
		{
			NR_CORE_ASSERT(false, "Failed to initialize audio engine.");
			return false;
		}

		allocationCallbacks.pUserData = &mRMCallbackData;
		mEngine.pResourceManager->config.allocationCallbacks = allocationCallbacks;

		mMasterReverb = CreateScope<DSP::Reverb>();
		mMasterReverb->Initialize(&mEngine, &mEngine.nodeGraph.endpoint);

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
		mComponentSoundMap.Clear(0);
		mAudioComponentRegistry.Clear(0);

		AudioThread::Stop();

		mMasterReverb.reset();

		ma_engine_uninit(&mEngine);

		for (auto& s : mSoundSources)
			delete s;

		bInitialized = false;

		NR_CORE_INFO("Audio Engine: engine un-initialized.");

		return true;
	}

	void AudioEngine::CreateSources()
	{
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

					if (a < b)
					{
						return sourceToCheck;
					}
					else if (a > b)
					{
						return sourceLowest;
					}
					else if (checkPlaybackPos)
					{
						return sourceToCheck->GetPlaybackPercentage() > sourceLowest->GetPlaybackPercentage() ? sourceToCheck : sourceLowest;
					}
					else
					{
						return sourceLowest;
					}
				}
			};

		// Run through all of the active sources and find the lowest priority source that can be stopped
		for (SoundObject* so : mActiveSounds)
		{
			if (auto* source = dynamic_cast<Sound*> (so))
			{
				if (source->IsStopping())
				{
					lowestPriStoppingSource = getLowerPriority(source, lowestPriStoppingSource);
				}
				else
				{
					if (!source->mLooping)
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
		ReleaseFinishedSources();

		NR_CORE_ASSERT(!mSourceManager.mFreeSourcIDs.empty());

		return releasedSoundSource;
	}

	Sound* AudioEngine::GetSoundForAudioComponent(uint64_t audioComponentID)
	{
		auto sceneID = sInstance->mCurrentSceneID;
		return mComponentSoundMap.Get(sceneID, audioComponentID).value_or(nullptr);
	}

	Sound* AudioEngine::GetSoundForAudioComponent(uint64_t audioComponentID, const SoundConfig& sourceConfig)
	{
		NR_CORE_ASSERT(AudioThread::IsAudioThread());

		auto sceneID = sInstance->mCurrentSceneID;

		// If component already has an active sound, return reference to the active sound
		if (auto* s = mComponentSoundMap.Get(sceneID, audioComponentID).value_or(nullptr))
		{
			return s;
		}

		Sound* sound = nullptr;

		int freeID;
		if (!mSourceManager.GetFreeSourceId(freeID))
		{
			// Stop lowest priority source
			FreeLowestPrioritySource();
			mSourceManager.GetFreeSourceId(freeID);
		}

		sound = mSoundSources.at(freeID);

		// Associate Sound object with the AudioComponent
		sound->mAudioComponentID = audioComponentID;

		sound->mSceneID = sceneID;

		if (!mSourceManager.InitializeSource(freeID, sourceConfig))
		{
			return nullptr;
		}

		mComponentSoundMap.Add(sceneID, audioComponentID, sound);
		mActiveSounds.push_back(sound);

		return sound;
	}

	void AudioEngine::SubmitSoundToPlay(uint64_t audioComponentID, const SoundConfig& sourceConfig)
	{
		auto startSound = [this, audioComponentID, sourceConfig]
			{
				if (Sound* sound = GetSoundForAudioComponent(audioComponentID, sourceConfig))
				{
					sound->mPlaybackComplete = [this, audioComponentID]
						{
							if (auto* ac = GetAudioComponentFromID(mCurrentSceneID, audioComponentID))
							{
								ac->MarkedForDestroy = true;
							}
						};

					sound->SetVolume(sourceConfig.VolumeMultiplier);
					sound->SetPitch(sourceConfig.PitchMultiplier);
					mSoundsToStart.push_back(sound);
				}
			};
		AudioThread::IsAudioThread() ? startSound() : ExecuteOnAudioThread(startSound, "StartSound");
	}

	bool AudioEngine::SubmitSoundToPlay(uint64_t audioComponentID)
	{
		auto* ac = GetAudioComponentFromID(mCurrentSceneID, audioComponentID);
		if (ac == nullptr)
		{
			NR_CORE_ASSERT(ac, "AudioComponent was not found in registry!");
			return false;
		}
		/* TODO: this is a little bit backwards at the moment.
				  Technically AC doesn't need to have these multipliers,
				  but conceptually it does.
		 */
		ac->VolumeMultiplier = ac->SoundConfig.VolumeMultiplier;
		ac->PitchMultiplier = ac->SoundConfig.PitchMultiplier;
		SubmitSoundToPlay(audioComponentID, ac->SoundConfig);

		return true;
	}

	bool AudioEngine::StopActiveSoundSource(uint64_t audioComponentID)
	{
		if (!mComponentSoundMap.Get(mCurrentSceneID, audioComponentID).has_value())
		{
			return false;
		}

		auto stopSound = [this, audioComponentID]
			{
				if (auto* sound = mComponentSoundMap.Get(mCurrentSceneID, audioComponentID).value_or(nullptr))
				{
					sound->Stop();
				}
			};

		AudioThread::IsAudioThread() ? stopSound() : ExecuteOnAudioThread(stopSound, "StopSound");

		return true;
	}

	bool AudioEngine::PauseActiveSoundSource(uint64_t audioComponentID)
	{
		if (!mComponentSoundMap.Get(mCurrentSceneID, audioComponentID).has_value())
		{
			return false;
		}

		auto pauseSound = [this, audioComponentID]
			{
				if (auto* sound = mComponentSoundMap.Get(mCurrentSceneID, audioComponentID).value_or(nullptr))
					sound->Pause();
			};

		AudioThread::IsAudioThread() ? pauseSound() : ExecuteOnAudioThread(pauseSound, "PauseSound");

		return true;
	}

	bool AudioEngine::IsSoundForComponentPlaying(uint64_t audioComponentID)
	{
		return mComponentSoundMap.Get(mCurrentSceneID, audioComponentID).has_value();
	}

	void AudioEngine::UpdateSources()
	{
		// Bulk update all of the sources
		std::scoped_lock lock{ mUpdateSourcesLock };
		for (auto& data : mSourceUpdateData)
		{
			if (auto* sound = GetSoundForAudioComponent(data.entityID))
			{
				sound->SetVolume(data.VolumeMultiplier);
				sound->SetPitch(data.PitchMultiplier);
				sound->SetLocation(data.Position);
				sound->SetVelocity(data.Velocity);
			}
		}
	}

	void AudioEngine::SubmitSourceUpdateData(std::vector<SoundSourceUpdateData> updateData)
	{
		std::scoped_lock lock{ mUpdateSourcesLock };
		mSourceUpdateData.swap(updateData);
	}

	void AudioEngine::UpdateListener()
	{
		if (mAudioListener.HasChanged(true))
		{
			glm::vec3 pos, dir, vel;
			mAudioListener.GetPositionDirection(pos, dir);
			mAudioListener.GetVelocity(vel);
			ma_engine_listener_set_position(&mEngine, 0, pos.x, pos.y, pos.z);
			ma_engine_listener_set_direction(&mEngine, 0, dir.x, dir.y, dir.z);
			ma_engine_listener_set_velocity(&mEngine, 0, vel.x, vel.y, vel.z);
		}
	}

	void AudioEngine::UpdateListenerPosition(const glm::vec3& newTranslation, const glm::vec3& newDirection)
	{
		if (mAudioListener.PositionNeedsUpdate(newTranslation, newDirection))
		{
			mAudioListener.SetPositionDirection(newTranslation, newDirection);
		}
	}

	void AudioEngine::UpdateListenerVelocity(const glm::vec3& newVelocity)
	{
		mAudioListener.SetVelocity(newVelocity);
	}

	void AudioEngine::ReleaseFinishedSources()
	{
		for (int i = mActiveSounds.size() - 1; i >= 0; --i)
		{
			if (Sound* source = dynamic_cast<Sound*>(mActiveSounds[i]))
			{
				if (source->IsFinished())
				{
					mActiveSounds.erase(mActiveSounds.begin() + i);
					mComponentSoundMap.Remove(source->mSceneID, source->mAudioComponentID);

					// Return Sound Source for reuse
					mSourceManager.mFreeSourcIDs.push(source->mSoundSourceID);
					{
						std::scoped_lock lock{ sStats.mutex };
						sStats.NumActiveSounds = mActiveSounds.size();
					}
				}
			}
		}
	}

	void AudioEngine::Update(TimeFrame dt)
	{
		UpdateListener();

		UpdateSources();

		// Start sounds requested to start
		for (auto* sound : mSoundsToStart)
		{
			sound->Play();
			{
				std::scoped_lock lock{ sStats.mutex };
				sStats.NumActiveSounds = mActiveSounds.size();
			}
		}
		mSoundsToStart.clear();

		for (auto* sound : mActiveSounds)
		{
			sound->Update(dt);
		}

		ReleaseFinishedSources();
	}

	void AudioEngine::RegisterNewListener(AudioListenerComponent& listenerComponent)
	{
	}

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
					{
						dynamic_cast<Sound*>(sound)->StopNow(true, true);
					}
					else
					{
						sound->Stop();
					}
				}
			};

		AudioThread::IsAudioThread() ? stopAll() : ExecuteOnAudioThread(stopAll, "StopAll");
	}

	void AudioEngine::SetSceneContext(const Ref<Scene>& scene)
	{
		auto& audioEngine = Get();

		audioEngine.StopAll();

		audioEngine.mSceneContext = scene;
		auto* newScene = audioEngine.mSceneContext.Raw();
		const auto newSceneID = newScene->GetID();
		audioEngine.mCurrentSceneID = newSceneID;

		{
			std::scoped_lock lock{ sStats.mutex };
			sStats.NumAudioComps = sInstance->mAudioComponentRegistry.Count(newSceneID);
		}

		auto view = newScene->GetAllEntitiesWith<Audio::AudioComponent>();
		for (auto entity : view)
		{
			Entity e = { entity, newScene };
			auto& audioComp = e.GetComponent<Audio::AudioComponent>();

			sInstance->RegisterAudioComponent(e);
		}
	}

	void Audio::AudioEngine::RuntimePlaying(UUID sceneID)
	{
		auto& audioEngine = Get();

		const auto currentSceneID = audioEngine.mCurrentSceneID;
		NR_CORE_ASSERT(currentSceneID == sceneID);

		auto newScene = Scene::GetScene(currentSceneID);

		auto view = newScene->GetAllEntitiesWith<Audio::AudioComponent>();
		for (auto entity : view)
		{
			Entity audioEntity = { entity, newScene.Raw() };

			auto& ac = audioEntity.GetComponent<Audio::AudioComponent>();
			if (ac.PlayOnAwake)
			{
				auto newScene = Scene::GetScene(currentSceneID);
				if (!newScene->IsEditorScene() && newScene->IsPlaying())
				{
					auto translation = newScene->GetWorldSpaceTransform(audioEntity).Translation;
					ac.SourcePosition = translation;
					ac.SoundConfig.SpawnLocation = translation;
					audioEngine.SubmitSoundToPlay(audioEntity.GetID());
				}
			}
		}
	}

	void AudioEngine::SceneDestruct(UUID sceneID)
	{
		Get().mAudioComponentRegistry.Clear(sceneID);
	}

	Ref<Scene>& AudioEngine::GetCurrentSceneContext()
	{
		return Get().mSceneContext;
	}

	AudioComponent* Audio::AudioEngine::GetAudioComponentFromID(UUID sceneID, uint64_t audioComponentID)
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

		uint64_t entityID = audioEntity.GetID();
		auto* ac = GetAudioComponentFromID(sceneID, entityID);
		if (ac->PlayOnAwake)
		{
			auto newScene = Scene::GetScene(sceneID);

			if (!newScene->IsEditorScene() && newScene->IsPlaying())
			{
				auto translation = newScene->GetWorldSpaceTransform(audioEntity).Translation;
				ac->SourcePosition = translation;
				ac->SoundConfig.SpawnLocation = translation;
				SubmitSoundToPlay(entityID);
			}
		}
	}

	void AudioEngine::UnregisterAudioComponent(UUID sceneID, UUID entityID)
	{
		mAudioComponentRegistry.Remove(sceneID, entityID);
		{
			std::scoped_lock lock{ sStats.mutex };
			sStats.NumAudioComps = mAudioComponentRegistry.Count(sceneID);
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
}