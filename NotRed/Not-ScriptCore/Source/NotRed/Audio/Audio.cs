using NR;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace NR
{
    public class Sound2D
    {
        internal protected Sound2D(Entity _entity)
        {
            entity = _entity;
            _audioComponent = entity.GetComponent<AudioComponent>();
        }

        protected Entity entity;
        protected AudioComponent _audioComponent;

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

        public bool Resume()
        {
            return _audioComponent.Resume();
        }
    }

    /// <summary> <c>Sound2D</c> is just a basic helper class 
    /// to easily spawn and controll simple positionable sound objects.
    /// </summary>
    public class Sound3D : Sound2D
    {
        internal Sound3D(Entity _entity) : base(_entity) { }

        public Vector3 Location
        {
            get { return entity.Translation; }
            set { entity.Translation = value; }
        }
        public Vector3 Orientation
        {
            get { return entity.Rotation; }
            set { entity.Rotation = value; }
        }
    }

    public static class Audio
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct Transform
        {
            public Vector3 Position;
            public Vector3 Orientation;
            public Vector3 Up;

            public Transform(Vector3 pos, Vector3 rot)
            {
                Position = pos;
                Orientation = rot;
                Up = new Vector3(0.0f, 1.0f, 0.0f);
            }

            public Transform(Vector3 pos, Vector3 rot, Vector3 up)
            {
                Position = pos;
                Orientation = rot;
                Up = up;
            }
        }

        public class CommandID
        {
            private readonly uint ID;
            public CommandID(string commandName)
            {
                ID = Constructor_Native(commandName);
            }

            public static bool operator ==(CommandID c1, CommandID c2) => c1.ID == c2.ID;
            public static bool operator !=(CommandID c1, CommandID c2) => c1.ID != c2.ID;
            public override bool Equals(object c) => ID == ((CommandID)c).ID;
            public override int GetHashCode() => (int)ID;


            [MethodImpl(MethodImplOptions.InternalCall)]
            public static extern uint Constructor_Native(string commandName);
        }

        public class Object
        {
            public ulong ID { get; private set; }
            public string DebugName { get; internal set; }

            internal Object(string debugName, Audio.Transform objectTransform)
            {
                ID = Constructor_Native(debugName, ref objectTransform);
                DebugName = debugName;
            }

            internal Object(ulong _ID)
            {
                ID = _ID;
            }

            ~Object()
            {
                ReleaseAudioObject_Native(ID);
            }

            public void SetTransform(Audio.Transform objectTransform)
            {
                SetTransform_Native(ID, ref objectTransform);
            }

            public void SetTransform(NR.Transform objectTransform)
            {
                var transform = new Audio.Transform
                {
                    Position = objectTransform.Position,
                    Orientation = objectTransform.Rotation
                };
                SetTransform_Native(ID, ref transform);
            }

            public Audio.Transform GetTransform()
            {
                GetTransform_Native(ID, out Audio.Transform transform);
                return transform;
            }

            [MethodImpl(MethodImplOptions.InternalCall)]
            public static extern ulong Constructor_Native(string debugName, ref Audio.Transform objectTransform);

            [MethodImpl(MethodImplOptions.InternalCall)]
            public static extern void SetTransform_Native(ulong objectID, ref Audio.Transform objectTransform);
            [MethodImpl(MethodImplOptions.InternalCall)]
            public static extern void GetTransform_Native(ulong objectID, out Audio.Transform transformOut);
        }

        public static Audio.Object InitializeAudioObject(string debugName, Audio.Transform objectTransform)
        {
            return new Object(debugName, objectTransform);
        }

        public static Audio.Object InitializeAudioObject(string debugName, NR.Transform objectTransform)
        {
            var transform = new Audio.Transform
            {
                Position = objectTransform.Position,
                Orientation = objectTransform.Rotation
            };
            return new Object(debugName, transform);
        }

        public static void ReleaseAudioObject(ulong objectID)
        {
            ReleaseAudioObject_Native(objectID);
        }

        public static void ReleaseAudioObject(Audio.Object audioObj)
        {
            ReleaseAudioObject(audioObj.ID);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool ReleaseAudioObject_Native(ulong objectID);

        public static Audio.Object GetObject(ulong objectID)
        {
            //string debugName = "";
            if (GetObjectInfo_Native(objectID, out string debugName))
            {
                return new Audio.Object(objectID) { DebugName = debugName };

            }
            else
                return null;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool GetObjectInfo_Native(ulong objectID, out string debugName);

        /// <summary> Post event on an object.
        /// <returns>Returns active Event ID
        /// </returns></summary>
        public static uint PostEvent(CommandID id, ulong objectID)
        {
            return PostEvent_Native((uint)id.GetHashCode(), objectID);
        }

        /// <summary> Post event by name. Prefer overload that takes CommandID for speed and initialize CommandIDs once.
        /// This creates new CommandID object from eventName, which has to hash the string.
        /// </summary>
        public static uint PostEvent(string eventName, ulong objectID)
        {
            return PostEvent(new CommandID(eventName), objectID);
        }

        public static uint PostEvent(string eventName, Audio.Object audioObj)
        {
            return PostEvent(new CommandID(eventName), audioObj.ID);
        }

        /// <summary> Post event on AudioComponent.
        /// <returns>Returns active Event ID
        /// </returns></summary>
        public static uint PostEvent(CommandID id, ref AudioComponent audioComponent)
        {
            return PostEventFromAC_Native((uint)id.GetHashCode(), audioComponent.Entity.ID);
        }

        /// <summary> Post event on at location creating temporary <c>Audio Object</c>.
        /// <returns>Returns active Event ID
        /// </returns></summary>
        public static uint PostEventAtLocation(CommandID id, Vector3 location, Vector3 orientation)
        {
            var spawnLocation = new Audio.Transform();
            spawnLocation.Position = location;
            spawnLocation.Orientation = orientation;
            return PostEventAtLocation_Native((uint)id.GetHashCode(), ref spawnLocation);
        }

        /// <summary> Stop playing audio sources associated to active event.
        /// <returns>Returns true - if event has playing audio soruces to stop.
        /// </returns></summary>
        public static bool StopEventID(uint eventID)
        {
            return StopEventID_Native(eventID);
        }

        /// <summary> Pause playing audio sources associated to active event.
        /// <returns>Returns true - if event has playing audio soruces to pause.
        /// </returns></summary>
        public static bool PauseEventID(uint eventID)
        {
            return PauseEventID_Native(eventID);
        }

        /// <summary> Resume playing audio sources associated to active event.
        /// <returns>Returns true - if event has playing audio soruces to resume.
        /// </returns></summary>
        public static bool ResumeEventID(uint eventID)
        {
            return ResumeEventID_Native(eventID);
        }

        //============================================================================================

        public static Sound3D CreateSound(CommandID triggerEventID, Audio.Transform location, float volume = 1.0f, float pitch = 1.0f)
        {
            return new Sound3D(new Entity(CreateSound_Native((uint)triggerEventID.GetHashCode(), ref location, volume, pitch)))
            {
                Location = location.Position,
                Orientation = location.Orientation
            };
        }

        public static Sound3D CreateSound(string triggerEventName, Audio.Transform location, float volume = 1.0f, float pitch = 1.0f)
        {
            return CreateSound(new CommandID(triggerEventName), location, volume, pitch);
        }


        /*public static Sound2D CreateSound2D(string assetPath, float volume = 1.0f, float pitch = 1.0f)
        {
            return new Sound3D(new Entity(CreateSound2DAssetPath_Native(assetPath, volume, pitch)))
            {
                VolumeMultiplier = volume,
                PitchMultiplier = pitch,
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
        }*/


        //============================================================================================

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern uint PostEvent_Native(uint id, ulong objectID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern uint PostEventFromAC_Native(uint id, ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern uint PostEventAtLocation_Native(uint id, ref Audio.Transform location);


        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool StopEventID_Native(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool PauseEventID_Native(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool ResumeEventID_Native(uint id);

        //============================================================================================


        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool PlaySound2DAssetPath_Native(string assetPath, float volume, float pitch);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool PlaySoundAtLocationAssetPath_Native(string assetPath, Vector3 location, float volume, float pitch);

        //============================================================================================

        /*[MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong CreateSound2DAssetPath_Native(string assetPath, float volume, float pitch);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong CreateSoundAtLocationAssetPath_Native(string assetPath, Vector3 location, float volume, float pitch);*/

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong CreateSound_Native(uint eventID, ref Audio.Transform location, float volume, float pitch);
    }
}