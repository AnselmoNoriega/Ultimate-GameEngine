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
                SetTag_Native(value);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetTag_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTag_Native(string tag);

    }

    public class TransformComponent : Component
    {
        public Matrix4 Transform
        {
            get
            {
                Matrix4 result;
                GetTransform_Native(Entity.ID, out result);
                return result;
            }
            set
            {
                SetTransform_Native(Entity.ID, ref value);
            }
        }

        public Vector3 Rotation
        {
            get
            {
                GetRotation_Native(Entity.ID, out Vector3 rotation);
                return rotation;
            }

            set
            {
                SetRotation_Native(Entity.ID, ref value);
            }
        }

        public Vector3 Forward
        {
            get
            {
                GetRelativeDirection_Native(Entity.ID, out Vector3 result, ref Vector3.Forward);
                return result;
            }
        }

        public Vector3 Right
        {
            get
            {
                GetRelativeDirection_Native(Entity.ID, out Vector3 result, ref Vector3.Right);
                return result;
            }
        }

        public Vector3 Up
        {
            get
            {
                GetRelativeDirection_Native(Entity.ID, out Vector3 result, ref Vector3.Up);
                return result;
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetTransform_Native(ulong entityID, out Matrix4 result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetTransform_Native(ulong entityID, ref Matrix4 result);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetRelativeDirection_Native(ulong entityID, out Vector3 result, ref Vector3 direction);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetRotation_Native(ulong entityID, out Vector3 result);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetRotation_Native(ulong entityID, ref Vector3 rotation);
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

    public class RigidBody2DComponent : Component
    {
        public void ApplyImpulse(Vector2 impulse, Vector2 offset, bool wake)
        {
            ApplyLinearImpulse_Native(Entity.ID, ref impulse, ref offset, wake);
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

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ApplyLinearImpulse_Native(ulong entityID, ref Vector2 impulse, ref Vector2 offset, bool wake);

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

        public enum ForceMode
        {
            Force,
            Impulse,
            VelocityChange,
            Acceleration
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

        public uint Layer { get { return GetLayer_Native(Entity.ID); } }

        public void AddForce(Vector3 force, ForceMode forceMode = ForceMode.Force)
        {
            AddForce_Native(Entity.ID, ref force, forceMode);
        }

        public void AddTorque(Vector3 torque, ForceMode forceMode = ForceMode.Force)
        {
            AddTorque_Native(Entity.ID, ref torque, forceMode);
        }

        public Vector3 GetVelocity()
        {
            GetVelocity_Native(Entity.ID, out Vector3 velocity);
            return velocity;
        }

        public void SetVelocity(Vector3 velocity)
        {
            SetVelocity_Native(Entity.ID, ref velocity);
        }

        public Vector3 GetAngularVelocity()
        {
            GetAngularVelocity_Native(Entity.ID, out Vector3 velocity);
            return velocity;
        }

        public void SetAngularVelocity(Vector3 velocity)
        {
            SetAngularVelocity_Native(Entity.ID, ref velocity);
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
        internal static extern float GetMass_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float SetMass_Native(ulong entityID, float mass);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Type GetBodyType_Native(ulong entityID);
    }
}