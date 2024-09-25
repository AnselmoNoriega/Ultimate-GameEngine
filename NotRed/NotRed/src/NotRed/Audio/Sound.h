#pragma once

#include <glm/glm.hpp>

#include "SoundObject.h"
#include "miniaudioInc.h"
#include "NotRed/Asset/Asset.h"

namespace NR::Audio
{
	class AudioEngine;
	enum class AttenuationModel
	{
		None,          // No distance attenuation and no spatialization.
		Inverse,       // Equivalent to OpenAL's AL_INVERSE_DISTANCE_CLAMPED.
		Linear,        // Linear attenuation. Equivalent to OpenAL's AL_LINEAR_DISTANCE_CLAMPED.
		Exponential    // Exponential attenuation. Equivalent to OpenAL's AL_EXPONENT_DISTANCE_CLAMPED.
	};

	struct SpatializationConfig : public Asset
	{
		AttenuationModel AttenuationMod{ AttenuationModel::Inverse };     // Distance attenuation function
		float MinGain{ 0.0f };                                            // Minumum volume muliplier
		float MaxGain{ 1.0f };                                            // Maximum volume multiplier
		float MinDistance{ 1.0f };                                        // Distance where to start attenuation
		float MaxDistance{ 1000.0f };                                     // Distance where to end attenuation
		float ConeInnerAngleInRadians{ 6.283185f };                       // Defines the angle where no directional attenuation occurs 
		float ConeOuterAngleInRadians{ 6.283185f };                       // Defines the angle where directional attenuation is at max value (lowest multiplier)
		float ConeOuterGain{ 0.0f };                                      // Attenuation multiplier when direction of the emmiter falls outside of the ConeOuterAngle
		float DopplerFactor{ 1.0f };                                      // The amount of doppler effect to apply. Set to 0 to disables doppler effect. 
		float Rolloff{ 0.6f };                                            // Affects steepness of the attenuation curve. At 1.0 Inverse model is the same as Exponential
		bool AirAbsorptionEnabled{ true };                               // Enable Air Absorption filter
	};

	struct SoundConfig : public Asset
	{
		Ref<Asset> FileAsset;
		bool Looping = false;
		float VolumeMultiplier = 1.0f;
		float PitchMultiplier = 1.0f;
		bool SpatializationEnabled = false;
		SpatializationConfig Spatialization;
		glm::vec3 SpawnLocation{ 0.0f, 0.0f, 0.0f };
	};

	class Sound : public SoundObject
	{
	public:
		Sound() = default;
		~Sound();

		bool Play() override;
		bool Stop() override;
		bool Pause() override;

		bool IsPlaying() const override;

		void SetVolume(float newVolume) override;
		void SetPitch(float newPitch) override;
		void SetLooping(bool looping);

		float GetVolume() override;
		float GetPitch() override;

		bool FadeIn(const float duration, const float targetVolume) override;
		bool FadeOut(const float duration, const float targetVolume) override;

		/* Initialize data source from sound configuration.
			@param soundConfig - configuration to initialized data source with
			@param audioEngine - reference to AudioEngine

			@returns true - if successfully initialized data source
		 */
		bool InitializeDataSource(const SoundConfig& soundConfig, AudioEngine* audioEngine);

		void SetLocation(const glm::vec3& location);
		void SetVelocity(const glm::vec3& velocity = { 0.0f, 0.0f, 0.0f });

		/* @return true - if has a valid data source */
		bool IsReadyToPlay() const { return mIsReadyToPlay; }

		void Update(TimeFrame dt) override;

		/* @returns true - if playback complete. E.g. reached the end of data, or has been hard-stopped. */
		bool IsFinished() const;

		/* @returns true - if currently "stop-fading". */
		bool IsStopping() const;

		/* Get current level of fader performing fade operations.
		   Such operations performed when FadeIn(), or FadeOut() are called,
		   as well as "stop-fade" and "resume from pause" fade.

		   @returns current fader level
		*/
		float GetCurrentFadeVolume();

		/* Get current priority level based on current fade volume
		   and priority value set for this sound source.

		   @returns normalized priority value
		*/
		float GetPriority();

		/* @returns current playback percentage (read position) whithin data source */
		float GetPlaybackPercentage();

	private:
		std::function<void()> mPlaybackComplete;

		enum class ESoundPlayState
		{
			Stopped,
			Starting,
			Playing,
			Pausing,
			Paused,
			Stopping,
			FadingOut,
			FadingIn
		};

	private:
		/* Stop playback with short fade-out to prevent click.
		   @param numSamples - length of the fade-out in PCM frames

		   @returns true - if successfully initialized fade
		*/
		bool StopFade(uint64_t numSamples);

		/* Stop playback with short fade-out to prevent click.
		   @param milliseconds - length of the fade-out in milliseconds

		   @returns true - if successfully initialized fade
		*/
		bool StopFade(int milliseconds);

		/* "Hard-stop" playback without fade. This is called to immediatelly stop playback, has ended,
		   as well as to reset the play state when "stop-fade" has ended.
		   @param notifyPlaybackComplete - used when the sound has finished its playback
		   @param resetPlaybackPosition - set to 'false' when pausing

		   @returns ID of the sound source in pool
		*/
		int StopNow(bool notifyPlaybackComplete = true, bool resetPlaybackPosition = true);

		static const std::string StringFromState(Sound::ESoundPlayState state);

	private:
		ESoundPlayState mPlayState = ESoundPlayState::Stopped;

		/* ID of this sound source in pool of pre-initialized sources. */
		int mSoundSourceID = -1;

		/* Data source. For now all we use is Miniaudio,
			in the future SoundObject will access data source via SoundSource class.
		 */
		ma_sound mSound;
		bool mIsReadyToPlay = false;
		
		uint8_t mPriority = 64;
		bool mLooping = false;
		bool mFinished = false;

		/* Stored Fader "resting" value. Used to restore Fader before restarting playback if a fade has occured. */
		float mStoredFaderValue = 1.0f;
		float mLastFadeOutDuration = 0.0f;

		/* Stop-fade counter. Used to stop the sound after "stopping-fade" has finished. */
		double mStopFadeTime = 0.0;

		/* ID of the AudioComponent this voice was initialized from and is attached to */
		uint64_t mAudioComponentID = 0;

		/* ID of the scene this voice belongs to. */
		uint64_t mSceneID = 0;

	private:
		friend class AudioEngine;
	};
}