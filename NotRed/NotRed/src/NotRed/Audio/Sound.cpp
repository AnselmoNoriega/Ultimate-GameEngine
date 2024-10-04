#include "nrpch.h"
#include "Sound.h"

#include "AudioEngine.h"

namespace NR::Audio
{
	const std::string Sound::StringFromState(Sound::ESoundPlayState state)
	{
		std::string str;

		switch (state)
		{
		case ESoundPlayState::Stopped:
			str = "Stopped";
			break;
		case ESoundPlayState::Starting:
			str = "Starting";
			break;
		case ESoundPlayState::Playing:
			str = "Playing";
			break;
		case ESoundPlayState::Pausing:
			str = "Pausing";
			break;
		case ESoundPlayState::Paused:
			str = "Paused";
			break;
		case ESoundPlayState::Stopping:
			str = "Stopping";
			break;
		case ESoundPlayState::FadingOut:
			str = "FadingOut";
			break;
		case ESoundPlayState::FadingIn:
			str = "FadingIn";
			break;
		default:
			break;
		}
		return str;
	}

	Sound::~Sound()
	{
		if (mSound.engineNode.baseNode.pCachedData != NULL && mSound.pDataSource != NULL)
		{
			ma_sound_uninit(&mSound);
		}
	}

	bool Sound::InitializeDataSource(const SoundConfig& config, AudioEngine* audioEngine)
	{
		if (mIsReadyToPlay)
		{
			if (IsPlaying())
			{
				ma_sound_stop(&mSound);
			}

			if (mSound.engineNode.baseNode.pCachedData != NULL || mSound.pDataSource != NULL)
			{
				ma_sound_uninit(&mSound);
			}

			ma_node_uninit(&mMasterSplitter, &mSound.engineNode.pEngine->pResourceManager->config.allocationCallbacks);
		}

		ma_result result;
		result = ma_sound_init_from_file(&audioEngine->mEngine, config.FileAsset->FilePath.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, &mSound);

		if (result != MA_SUCCESS)
		{
			return false;
		}

		InitializeEffects();

		const bool isSpatializationEnabled = config.SpatializationEnabled;

		auto& spatializer = mSound.engineNode.spatializer;
		mSound.engineNode.isSpatializationDisabled = !isSpatializationEnabled;

		if (isSpatializationEnabled)
		{
			const auto& spatialization = config.Spatialization;
			ma_attenuation_model attMod{ ma_attenuation_model_inverse };

			switch (spatialization.AttenuationMod)
			{
			case AttenuationModel::None:
				attMod = ma_attenuation_model_none;
				break;
			case AttenuationModel::Inverse:
				attMod = ma_attenuation_model_inverse;
				break;
			case AttenuationModel::Linear:
				attMod = ma_attenuation_model_linear;
				break;
			case AttenuationModel::Exponential:
				attMod = ma_attenuation_model_exponential;
				break;
			}

			spatializer.attenuationModel = attMod;
			spatializer.minGain = spatialization.MinGain;
			spatializer.maxGain = spatialization.MaxGain;
			spatializer.minDistance = spatialization.MinDistance;
			spatializer.maxDistance = spatialization.MaxDistance;
			spatializer.coneInnerAngleInRadians = spatialization.ConeInnerAngleInRadians;
			spatializer.coneOuterAngleInRadians = spatialization.ConeOuterAngleInRadians;
			spatializer.coneOuterGain = spatialization.ConeOuterGain;
			spatializer.dopplerFactor = spatialization.DopplerFactor;
			spatializer.rolloff = spatialization.Rolloff;
		}

		SetLooping(config.Looping);
		SetLocation(config.SpawnLocation);

		mIsReadyToPlay = result == MA_SUCCESS;
		return result == MA_SUCCESS;
	}

	void Sound::InitializeEffects()
	{
		ma_result result = MA_SUCCESS;
		ma_splitter_node_config splitterConfig = ma_splitter_node_config_init(mSound.engineNode.baseNode.pOutputBuses[0].channels);

		result = ma_splitter_node_init(
			mSound.engineNode.baseNode.pNodeGraph,
			&splitterConfig,
			&mSound.engineNode.pEngine->pResourceManager->config.allocationCallbacks,
			&mMasterSplitter);

		NR_CORE_ASSERT(result == MA_SUCCESS);

		// Store the node the sound was connected to
		auto* oldOutput = mSound.engineNode.baseNode.pOutputBuses[0].pInputNode;

		// Attach splitter node to the old output of the sound
		result = ma_node_attach_output_bus(&mMasterSplitter, 0, oldOutput, 0);
		NR_CORE_ASSERT(result == MA_SUCCESS);

		// Attach sound node to splitter node
		result = ma_node_attach_output_bus(&mSound, 0, &mMasterSplitter, 0);
		NR_CORE_ASSERT(result == MA_SUCCESS);

		//result = ma_node_attach_output_bus(&m_MasterSplitter, 0, oldOutput, 0);
		// Set volume of the main pass-through output of the splitter to 1.0
		result = ma_node_set_output_bus_volume(&mMasterSplitter, 0, 1.0f);
		NR_CORE_ASSERT(result == MA_SUCCESS);

		// Mute the "FX send" output of the splitter
		result = ma_node_set_output_bus_volume(&mMasterSplitter, 1, 0.0f);
		NR_CORE_ASSERT(result == MA_SUCCESS);
	}

	bool Sound::Play()
	{
		if (!IsReadyToPlay())
		{
			return false;
		}

		ma_result result = MA_ERROR;

		switch (mPlayState)
		{
		case ESoundPlayState::Stopped:
		{
			mFinished = false;
			result = ma_sound_start(&mSound);
			mPlayState = ESoundPlayState::Starting;
			break;
		}
		case ESoundPlayState::Playing:
		{
			StopNow(false, true);
			mFinished = false;
			result = ma_sound_start(&mSound);
			mPlayState = ESoundPlayState::Starting;
			break;
		}
		case ESoundPlayState::Pausing:
		{
			StopNow(false, false);
			result = ma_sound_start(&mSound);
			mPlayState = ESoundPlayState::Starting;
			break;
		}
		case ESoundPlayState::Paused:
		{
			// Prepare fade-in for un-pause
			ma_sound_set_fade_in_milliseconds(&mSound, 0.0f, mStoredFaderValue, STOPPING_FADE_MS / 2);
			result = ma_sound_start(&mSound);
			mPlayState = ESoundPlayState::Starting;
			break;
		}
		case ESoundPlayState::Stopping:
		{
			StopNow(true, true);
			result = ma_sound_start(&mSound);
			mPlayState = ESoundPlayState::Starting;
			break;
		}
		case ESoundPlayState::Starting:
		case ESoundPlayState::FadingOut:
		case ESoundPlayState::FadingIn:
		default:
			break;
		}

		NR_CORE_ASSERT(result == MA_SUCCESS);
		return result == MA_SUCCESS;
	}

	bool Sound::Stop()
	{
		bool result = true;
		switch (mPlayState)
		{
		case ESoundPlayState::Stopped:
			result = false;
			break;
		case ESoundPlayState::Starting:
		{
			StopNow(false, true); // consider stop-fading
			mPlayState = ESoundPlayState::Stopping;
			break;
		}
		case ESoundPlayState::Playing:
		{
			result = StopFade(STOPPING_FADE_MS);
			mPlayState = ESoundPlayState::Stopping;
			break;
		}
		case ESoundPlayState::Pausing:
		{
			StopNow(true, true);
			mPlayState = ESoundPlayState::Stopping;
			break;
		}
		case ESoundPlayState::Paused:
		{
			StopNow(true, true);
			mPlayState = ESoundPlayState::Stopping;
			break;
		}
		case ESoundPlayState::Stopping:
			StopNow(true, true);
			break;
		case ESoundPlayState::FadingOut:
		case ESoundPlayState::FadingIn:
		default:
			break;
		}
		mLastFadeOutDuration = (float)STOPPING_FADE_MS / 1000.0f;

		return result;
	}

	bool Sound::Pause()
	{
		bool result = true;

		switch (mPlayState)
		{
		case ESoundPlayState::Playing:
		{
			result = StopFade(STOPPING_FADE_MS);
			mPlayState = ESoundPlayState::Pausing;
			break;
		}
		case ESoundPlayState::FadingOut:
		case ESoundPlayState::FadingIn:
			break;
		default:
			result = true;
			break;
		}

		return result;
	}

	bool Sound::StopFade(uint64_t numSamples)
	{
		constexpr double stopFadeTime = (double)STOPPING_FADE_MS * 1.1 / 1000.0;
		mStopFadeTime = stopFadeTime;

		ma_sound_set_fade_in_pcm_frames(&mSound, -1.0f, 0.0f, numSamples);
		
		return true;
	}

	bool Sound::StopFade(int milliseconds)
	{
		NR_CORE_ASSERT(milliseconds > 0);

		const uint64_t fadeInFrames = (milliseconds * mSound.engineNode.fader.config.sampleRate) / 1000;

		return StopFade(fadeInFrames);
	}

	bool Sound::FadeIn(const float duration, const float targetVolume)
	{
		return ma_sound_start(&mSound) == MA_SUCCESS;
	}

	bool Sound::FadeOut(const float duration, const float targetVolume)
	{
		return true;
	}

	bool Sound::IsPlaying() const
	{
		return mPlayState != ESoundPlayState::Stopped && mPlayState != ESoundPlayState::Paused;
	}

	bool Sound::IsFinished() const
	{
		return mFinished;
	}

	bool Sound::IsStopping() const
	{
		return mPlayState == ESoundPlayState::Stopping;
	}

	void Sound::SetVolume(float newVolume)
	{
		ma_sound_set_volume(&mSound, newVolume);
	}

	void Sound::SetPitch(float newPitch)
	{
		ma_sound_set_pitch(&mSound, newPitch);
	}

	void Sound::SetLooping(bool looping)
	{
		mLooping = looping;
		ma_sound_set_looping(&mSound, mLooping);
	}

	float Sound::GetVolume()
	{
		return ma_node_get_output_bus_volume(&mSound, 0);
	}

	float Sound::GetPitch()
	{
		return mSound.engineNode.pitch;
	}

	void Sound::SetLocation(const glm::vec3& location)
	{
		ma_sound_set_position(&mSound, location.x, location.y, location.z);
	}

	void Sound::SetVelocity(const glm::vec3& velocity /*= { 0.0f, 0.0f, 0.0f }*/)
	{
		ma_sound_set_velocity(&mSound, velocity.x, velocity.y, velocity.z);
	}

	void Sound::Update(TimeFrame dt)
	{
		auto notifyIfFinished = [&]
			{
				if (ma_sound_at_end(&mSound) == MA_TRUE && mPlaybackComplete)
				{
					mPlaybackComplete();
				}
			};

		auto isNodePlaying = [&]
			{
				return ma_sound_is_playing(&mSound) == MA_TRUE;
			};

		auto isFadeFinished = [&]
			{
				return mStopFadeTime <= 0.0;
			};

		mStopFadeTime = std::max(0.0, mStopFadeTime - (double)dt.GetSeconds());

		switch (mPlayState)
		{
		case ESoundPlayState::Starting:
		{
			if (isNodePlaying())
			{
				mPlayState = ESoundPlayState::Playing;
			}
			break;
		}
		case ESoundPlayState::Playing:
		{
			if (ma_sound_is_playing(&mSound) == MA_FALSE)
			{
				mPlayState = ESoundPlayState::Stopped;
				mFinished = true;
				notifyIfFinished();
			}
			break;
		}
		case ESoundPlayState::Pausing:
		{
			if (isFadeFinished())
			{
				StopNow(false, false);
				mPlayState = ESoundPlayState::Paused;
			}
			break;
		}
		case ESoundPlayState::Paused:
			break;
		case ESoundPlayState::Stopping:
			if (isFadeFinished())
			{
				StopNow(true, true);
				mPlayState = ESoundPlayState::Stopped;
			}
			break;
		case ESoundPlayState::Stopped:
		case ESoundPlayState::FadingOut:
		case ESoundPlayState::FadingIn:
		default:
			break;
		}
	}

	int Sound::StopNow(bool notifyPlaybackComplete /*= true*/, bool resetPlaybackPosition /*= true*/)
	{
		// Stop reading the data source
		ma_sound_stop(&mSound);

		if (resetPlaybackPosition)
		{
			// Reset data source read position to the beginning of the data
			ma_sound_seek_to_pcm_frame(&mSound, 0);

			// Mark this voice to be released.
			mFinished = true;
		}
		mSound.engineNode.fader.volumeEnd = 1.0f;

		// Need to notify AudioEngine of completion,
		// if this is one shot, AudioComponent needs to be destroyed.
		if (notifyPlaybackComplete && mPlaybackComplete)
		{
			mPlaybackComplete();
		}

		return mSoundSourceID;
	}

	float Sound::GetCurrentFadeVolume()
	{
		return ma_sound_get_current_fade_volume(&mSound);
	}

	float Sound::GetPriority()
	{
		return GetCurrentFadeVolume() * ((float)mPriority / 255.0f);
	}

	float Sound::GetPlaybackPercentage()
	{
		ma_uint64 currentFrame;
		ma_uint64 totalFrames;
		ma_sound_get_cursor_in_pcm_frames(&mSound, &currentFrame);

		ma_sound_get_length_in_pcm_frames(&mSound, &totalFrames);

		return (float)currentFrame / (float)totalFrames;
	}
}