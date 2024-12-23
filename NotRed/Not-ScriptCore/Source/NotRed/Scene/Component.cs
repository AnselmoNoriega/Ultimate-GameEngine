using System;
using System.Runtime.CompilerServices;

namespace NR
{
    public abstract class Component
    {
        public Entity Entity { get; set; }
    }

    public class TagComponent : Component
    {
        public string Tag
        {
            get
            {
                return GetTag_Native(Entity.ID);
            }
            set
            {
                SetTag_Native(Entity.ID, value);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetTag_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTag_Native(ulong entityID, string tag);

    }

    public class TransformComponent : Component
    {
        public Transform Transform
        {
            get
            {
                GetTransform_Native(Entity.ID, out Transform result);
                return result;
            }
            set
            {
                SetTransform_Native(Entity.ID, ref value);
            }
        }

        public Vector3 Translation
        {
            get
            {
                GetTranslation_Native(Entity.ID, out Vector3 result);
                return result;
            }

            set
            {
                SetTranslation_Native(Entity.ID, ref value);
            }
        }

        public Vector3 Rotation
        {
            get
            {
                GetRotation_Native(Entity.ID, out Vector3 result);
                return result;
            }

            set
            {
                SetRotation_Native(Entity.ID, ref value);
            }
        }

        public Vector3 Scale
        {
            get
            {
                GetScale_Native(Entity.ID, out Vector3 result);
                return result;
            }
            set
            {
                SetScale_Native(Entity.ID, ref value);
            }
        }

        public Transform GetWorldSpaceTransform()
        {
            GetWorldSpaceTransform_Native(Entity.ID, out Transform transform);
            return transform;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetTransform_Native(ulong entityID, out Transform outTransform);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetTransform_Native(ulong entityID, ref Transform inTransform);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetTranslation_Native(ulong entityID, out Vector3 outTranslation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetTranslation_Native(ulong entityID, ref Vector3 inTranslation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetRotation_Native(ulong entityID, out Vector3 outRotation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetRotation_Native(ulong entityID, ref Vector3 inRotation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetScale_Native(ulong entityID, out Vector3 outScale);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetScale_Native(ulong entityID, ref Vector3 inScale);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetWorldSpaceTransform_Native(ulong entityID, out Transform outTransform);
    }

    public class MeshComponent : Component
    {
        public Mesh Mesh
        {
            get
            {
                Mesh result = new Mesh(GetMesh_Native(Entity.ID));
                return result;
            }
            set
            {
                IntPtr ptr = value == null ? IntPtr.Zero : value._unmanagedInstance;
                SetMesh_Native(Entity.ID, ptr);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern IntPtr GetMesh_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetMesh_Native(ulong entityID, IntPtr unmanagedInstance);

    }

    public class CameraComponent : Component
    {
        // TODO
    }

    public class ScriptComponent : Component
    {
        // TODO
    }

    public class SpriteRendererComponent : Component
    {
        // TODO
    }

    public enum RigidBody2DBodyType
    {
        Static, Dynamic, Kinematic
    }

    public class RigidBody2DComponent : Component
    {
        public void ApplyImpulse(Vector2 impulse, Vector2 offset, bool wake)
        {
            ApplyImpulse_Native(Entity.ID, ref impulse, ref offset, wake);
        }

        public Vector2 GetVelocity()
        {
            GetVelocity_Native(Entity.ID, out Vector2 velocity);
            return velocity;
        }

        public void SetVelocity(Vector2 velocity)
        {
            SetVelocity_Native(Entity.ID, ref velocity);
        }

        public void Rotate(Vector3 rotation)
        {
            Rotate_Native(Entity.ID, ref rotation);
        }

        public RigidBody2DBodyType BodyType
        {
            get
            {
                RigidBody2DBodyType type;
                GetBodyType_Native(Entity.ID, out type);
                return type;
            }
            set
            {
                SetBodyType_Native(Entity.ID, ref value);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetBodyType_Native(ulong entityID, out RigidBody2DBodyType type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetBodyType_Native(ulong entityID, ref RigidBody2DBodyType type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ApplyImpulse_Native(ulong entityID, ref Vector2 impulse, ref Vector2 offset, bool wake);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetVelocity_Native(ulong entityID, out Vector2 velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetVelocity_Native(ulong entityID, ref Vector2 velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Rotate_Native(ulong entityID, ref Vector3 rotation);
    }

    public class BoxCollider2DComponent : Component
    {
    }

    public class RigidBodyComponent : Component
    {
        public enum Type
        {
            Static,
            Dynamic
        }

        public Type BodyType
        {
            get
            {
                return GetBodyType_Native(Entity.ID);
            }
        }

        public float Mass
        {
            get { return GetMass_Native(Entity.ID); }
            set { SetMass_Native(Entity.ID, value); }
        }

        public Vector3 Velocity
        {
            get
            {
                GetVelocity_Native(Entity.ID, out Vector3 velocity);
                return velocity;
            }
            set 
            {
                SetVelocity_Native(Entity.ID, ref value); 
            }
        }

        public Vector3 AngularVelocity
        {
            get
            {
                GetAngularVelocity_Native(Entity.ID, out Vector3 velocity);
                return velocity;
            }
            set 
            { 
                SetAngularVelocity_Native(Entity.ID, ref value); 
            }
        }

        public float MaxVelocity
        {
            get 
            {
                return GetMaxVelocity_Native(Entity.ID); 
            }
            set 
            { 
                SetMaxVelocity_Native(Entity.ID, value); 
            }
        }

        public float MaxAngularVelocity
        {
            get 
            { 
                return GetMaxAngularVelocity_Native(Entity.ID); 
            }
            set 
            { 
                SetMaxAngularVelocity_Native(Entity.ID, value); 
            }
        }

        public uint Layer 
        { 
            get 
            { 
                return GetLayer_Native(Entity.ID);
            } 
        }

        public void GetKinematicTarget(out Vector3 targetPosition, out Vector3 targetRotation)
        {
            GetKinematicTarget_Native(Entity.ID, out targetPosition, out targetRotation);
        }
        
        public void SetKinematicTarget(Vector3 targetPosition, Vector3 targetRotation)
        {
            SetKinematicTarget_Native(Entity.ID, ref targetPosition, ref targetRotation);
        }

        public void AddForce(Vector3 force, ForceMode forceMode = ForceMode.Force)
        {
            AddForce_Native(Entity.ID, ref force, forceMode);
        }

        public void AddTorque(Vector3 torque, ForceMode forceMode = ForceMode.Force)
        {
            AddTorque_Native(Entity.ID, ref torque, forceMode);
        }

        public void Rotate(Vector3 rotation)
        {
            Rotate_Native(Entity.ID, ref rotation);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddForce_Native(ulong entityID, ref Vector3 force, ForceMode forceMode);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddTorque_Native(ulong entityID, ref Vector3 torque, ForceMode forceMode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetVelocity_Native(ulong entityID, out Vector3 velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetVelocity_Native(ulong entityID, ref Vector3 velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetAngularVelocity_Native(ulong entityID, out Vector3 velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetAngularVelocity_Native(ulong entityID, ref Vector3 velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern uint GetLayer_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetMaxVelocity_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetMaxVelocity_Native(ulong entityID, float maxVelocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetMaxAngularVelocity_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetMaxAngularVelocity_Native(ulong entityID, float maxVelocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Rotate_Native(ulong entityID, ref Vector3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetMass_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetMass_Native(ulong entityID, float mass);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Type GetBodyType_Native(ulong entityID); 
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetKinematicTarget_Native(ulong entityID, out Vector3 targetPosition, out Vector3 targetRotation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetKinematicTarget_Native(ulong entityID, ref Vector3 targetPosition, ref Vector3 targetRotation);
    }

    public class AudioListenerComponent : Component
    {
    }
    public class AudioComponent : Component
    {
        public bool IsPlaying()
        {
            return IsPlaying_Native(Entity.ID);
        }

        public bool Play(float startTime = 0.0f)
        {
            return Play_Native(Entity.ID, startTime);
        }

        public bool Stop()
        {
            return Stop_Native(Entity.ID);
        }

        public bool Pause()
        {
            return Pause_Native(Entity.ID);
        }

        public float VolumeMultiplier
        {
            get { return GetVolumeMult_Native(Entity.ID); }
            set { SetVolumeMult_Native(Entity.ID, value); }
        }

        public float PitchMultiplier
        {
            get { return GetPitchMult_Native(Entity.ID); }
            set { SetPitchMult_Native(Entity.ID, value); }
        }

        public bool Looping
        {
            get { return GetLooping_Native(Entity.ID); }
            set { SetLooping_Native(Entity.ID, value); }
        }

        public void SetSound(SimpleSound sound)
        {
            SetSound_Native(Entity.ID, sound._unmanagedInstance);
        }

        public void SetSound(string assetPath)
        {
            SetSoundPath_Native(Entity.ID, assetPath);
        }

        public SimpleSound GetSound()
        {
            return new SimpleSound(GetSound_Native(Entity.ID));
        }

        public float MasterReverbSend
        {
            get 
            {
                return GetMasterReverbSend_Native(Entity.ID); 
            }
            set 
            { 
                SetMasterReverbSend_Native(Entity.ID, value); 
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsPlaying_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Play_Native(ulong entityID, float startTime);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Stop_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Pause_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetVolumeMult_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetVolumeMult_Native(ulong entityID, float volumeMult);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetPitchMult_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetPitchMult_Native(ulong entityID, float pitchMult);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool GetLooping_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetLooping_Native(ulong entityID, bool volumeMult);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetSound_Native(ulong entityID, IntPtr simpleSound);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetSoundPath_Native(ulong entityID, string assetPath);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string GetSound_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetMasterReverbSend_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetMasterReverbSend_Native(ulong entityID, float sendLevel);
    }
}