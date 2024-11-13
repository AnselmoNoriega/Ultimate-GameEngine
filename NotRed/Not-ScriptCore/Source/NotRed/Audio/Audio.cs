using System;
using System.Runtime.CompilerServices;

namespace NR
{
    public class SimpleSound
    {
        internal IntPtr _unmanagedInstance;

        public SimpleSound(string filepath)
        {
            _unmanagedInstance = Constructor_Native(filepath);
        }
        internal SimpleSound(IntPtr unmanagedInstance)
        {
            _unmanagedInstance = unmanagedInstance;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr Constructor_Native(string filepath);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Destructor_Native(IntPtr unmanagedInstance);
    };

    public class Sound2D
    {
        protected Entity entity;
        protected AudioComponent _audioComponent;

        internal protected Sound2D(Entity _entity)
        {
            entity = _entity;
            _audioComponent = entity.GetComponent<AudioComponent>();
        }

        public float VolumeMultiplier
        {
            get { return _audioComponent.VolumeMultiplier; }
            set { _audioComponent.VolumeMultiplier = value; }
        }

        public float PitchMultiplier
        {
            get { return _audioComponent.PitchMultiplier; }
            set { _audioComponent.PitchMultiplier = value; }
        }

        public bool IsPlaying()
        {
            return _audioComponent.IsPlaying();
        }

        public bool Play(float startTime = 0.0f)
        {
            return _audioComponent.Play(startTime);
        }

        public bool Stop()
        {
            return _audioComponent.Stop();
        }

        public bool Pause()
        {
            return _audioComponent.Pause();
        }

        public bool Looping
        {
            get { return _audioComponent.Looping; }
            set { _audioComponent.Looping = value; }
        }
    }

    public class Sound3D : Sound2D
    {
        internal Sound3D(Entity _entity) : base(_entity) { }

        public Vector3 Location
        {
            get { return entity.Translation; }
            set { entity.Translation = value; }
        }
        public float MasterReverbSend
        {
            get { return _audioComponent.MasterReverbSend; }
            set { _audioComponent.MasterReverbSend = value; }
        }
    }

    public static class Audio
    {
        public static bool PlaySound2D(SimpleSound simpleSound, float volume = 1.0f, float pitch = 1.0f)
        {
            return PlaySound2DAsset_Native(simpleSound._unmanagedInstance, volume, pitch);
        }

        public static bool PlaySound2D(string assetPath, float volume = 1.0f, float pitch = 1.0f)
        {
            return PlaySound2DAssetPath_Native(assetPath, volume, pitch);
        }

        public static bool PlaySoundAtLocation(SimpleSound simpleSound, Vector3 location, float volume = 1.0f, float pitch = 1.0f)
        {
            return PlaySoundAtLocationAsset_Native(simpleSound._unmanagedInstance, location, volume, pitch);
        }

        public static bool PlaySoundAtLocation(string assetPath, Vector3 location, float volume = 1.0f, float pitch = 1.0f)
        {
            return PlaySoundAtLocationAssetPath_Native(assetPath, location, volume, pitch);
        }

        public static Sound2D CreateSound2D(SimpleSound simpleSound, float volume = 1.0f, float pitch = 1.0f)
        {
            return new Sound2D(new Entity(CreateSound2DAsset_Native(simpleSound._unmanagedInstance, volume, pitch)))
            {
                VolumeMultiplier = volume,
                PitchMultiplier = pitch,
            };
        }

        public static Sound2D CreateSound2D(string assetPath, float volume = 1.0f, float pitch = 1.0f)
        {
            return new Sound3D(new Entity(CreateSound2DAssetPath_Native(assetPath, volume, pitch)))
            {
                VolumeMultiplier = volume,
                PitchMultiplier = pitch,
            };
        }

        public static Sound3D CreateSoundAtLocation(SimpleSound simpleSound, Vector3 location, float volume = 1.0f, float pitch = 1.0f)
        {
            return new Sound3D(new Entity(CreateSoundAtLocationAsset_Native(simpleSound._unmanagedInstance, location, volume, pitch)))
            {
                VolumeMultiplier = volume,
                PitchMultiplier = pitch,
                Location = location
            };
        }
        public static Sound3D CreateSoundAtLocation(string assetPath, Vector3 location, float volume = 1.0f, float pitch = 1.0f)
        {
            return new Sound3D(new Entity(CreateSoundAtLocationAssetPath_Native(assetPath, location, volume, pitch)))
            {
                VolumeMultiplier = volume,
                PitchMultiplier = pitch,
                Location = location
            };
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool PlaySound2DAsset_Native(IntPtr simpleSound, float volume, float pitch);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool PlaySound2DAssetPath_Native(string assetPath, float volume, float pitch);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool PlaySoundAtLocationAsset_Native(IntPtr simpleSound, Vector3 location, float volume, float pitch);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool PlaySoundAtLocationAssetPath_Native(string assetPath, Vector3 location, float volume, float pitch);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong CreateSound2DAsset_Native(IntPtr simpleSound, float volume, float pitch);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong CreateSound2DAssetPath_Native(string assetPath, float volume, float pitch);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong CreateSoundAtLocationAsset_Native(IntPtr simpleSound, Vector3 location, float volume, float pitch);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong CreateSoundAtLocationAssetPath_Native(string assetPath, Vector3 location, float volume, float pitch);
    }
}