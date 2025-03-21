﻿using System;
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
        public Transform WorldTransform
        {
            get
            {
                GetWorldSpaceTransform_Native(Entity.ID, out Transform result);
                return result;
            }
        }

        public bool IsKinematic
        {
            get => IsKinematic_Native(Entity.ID);
            set => SetIsKinematic_Native(Entity.ID, value);
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

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsKinematic_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetIsKinematic_Native(ulong entityID, bool isKinematic);
    }

    public class MeshComponent : Component
    {
        public Mesh Mesh
        {
            get
            {
                bool isStatic = true;
                IntPtr mesh = GetMesh_Native(Entity.ID, ref isStatic);
                Mesh result = new Mesh(mesh, isStatic);
                return result;
            }
            set
            {
                IntPtr ptr = value == null ? IntPtr.Zero : value._unmanagedInstance;
                SetMesh_Native(Entity.ID, ptr, value._isStatic);
            }
        }

        public bool HasMaterial(int index)
        {
            return HasMaterial_Native(Entity.ID, index);
        }

        public Material GetMaterial(int index)
        {
            if (!HasMaterial(index))
            {
                return null;
            }

            return new Material(GetMaterial_Native(Entity.ID, index));
        }

        public bool IsRigged
        {
            get
            {
                return GetIsRigged_Native(Entity.ID);
            }
        }

        public void ReloadMeshCollider()
        {
            ReloadMeshCollider_Native(Entity.ID);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern IntPtr GetMesh_Native(ulong entityID, ref bool isStatic);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetMesh_Native(ulong entityID, IntPtr unmanagedInstance, bool isStatic);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool GetIsRigged_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool HasMaterial_Native(ulong entityID, int index);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern IntPtr GetMaterial_Native(ulong entityID, int index);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ReloadMeshCollider_Native(ulong entityID);

    }

    public class AnimationComponent : Component
    {
        public bool IsAnimationPlaying
        {
            get => GetIsAnimationPlaying_Native(Entity.ID);
            set => SetIsAnimationPlaying_Native(Entity.ID, value);
        }
        public uint StateIndex
        {
            get => GetStateIndex_Native(Entity.ID);
            set => SetStateIndex_Native(Entity.ID, value);
        }

        public bool IsRootMotionEnabled
        {
            get => GetEnableRootMotion_Native(Entity.ID);
            set => SetEnableRootMotion_Native(Entity.ID, value);
        }

        public Transform RootMotion
        {
            get
            {
                GetRootMotion_Native(Entity.ID, out Transform result);
                return result;
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool GetIsAnimationPlaying_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetIsAnimationPlaying_Native(ulong entityID, bool value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern uint GetStateIndex_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetStateIndex_Native(ulong entityID, uint value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool GetEnableRootMotion_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetEnableRootMotion_Native(ulong entityID, bool value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetRootMotion_Native(ulong entityID, out Transform result);
    }

    public class CameraComponent : Component
    {
        // TODO
    }

    public class ScriptComponent : Component
    {
        public object Instance
        {
            get => GetInstance_Native(Entity.ID);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern object GetInstance_Native(ulong entityID);
    }

    public class SpriteRendererComponent : Component
    {
        // TODO
    }

    public class PointLightComponent : Component
    {
        public Vector3 Radiance
        {
            get
            {
                GetRadiance_Native(Entity.ID, out Vector3 result);
                return result;
            }
            set
            {
                SetRadiance_Native(Entity.ID, ref value);
            }
        }

        public float Intensity
        {
            get
            {
                return GetIntensity_Native(Entity.ID);
            }

            set
            {
                SetIntensity_Native(Entity.ID, value);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetRadiance_Native(ulong entityID, out Vector3 outRadiance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetRadiance_Native(ulong entityID, ref Vector3 inRadiance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetIntensity_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetIntensity_Native(ulong entityID, float intensity);
    }

    public enum RigidBody2DBodyType
    {
        Static, Dynamic, Kinematic
    }

    public class RigidBody2DComponent : Component
    {
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

        public Vector2 Translation
        {
            get
            {
                GetTranslation_Native(Entity.ID, out Vector2 translation);
                return translation;
            }

            set
            {
                SetTranslation_Native(Entity.ID, ref value);
            }
        }

        public float Rotation
        {
            get
            {
                GetRoation_Native(Entity.ID, out float rotation);
                return rotation;
            }
            set
            {
                SetRotationNative(Entity.ID, ref value);
            }
        }

        public float Mass
        {
            get { return GetMass_Native(Entity.ID); }
            set { SetMass_Native(Entity.ID, ref value); }
        }

        public Vector2 Velocity
        {
            get
            {
                GetVelocity_Native(Entity.ID, out Vector2 velocity);
                return velocity;
            }
            set { SetVelocity_Native(Entity.ID, ref value); }
        }

        public float GravityScale
        {
            get
            {
                GetGravityScale_Native(Entity.ID, out float gravityScale);
                return gravityScale;
            }

            set { SetGravityScale_Native(Entity.ID, ref value); }
        }

        public void ApplyLinearImpulse(Vector2 impulse, Vector2 offset, bool wake)
        {
            ApplyLinearImpulse_Native(Entity.ID, ref impulse, ref offset, ref wake);
        }

        public void ApplyAngularImpulse(float impulse, bool wake)
        {
            ApplyAngularImpulse_Native(Entity.ID, ref impulse, wake);
        }

        public void AddForce(Vector2 force, Vector2 offset, bool wake)
        {
            AddForce_Native(Entity.ID, ref force, ref offset, wake);
        }

        public void AddTorque(float torque, bool wake)
        {
            AddTorque_Native(Entity.ID, ref torque, wake);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetBodyType_Native(ulong entityID, out RigidBody2DBodyType type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetBodyType_Native(ulong entityID, ref RigidBody2DBodyType type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetTranslation_Native(ulong entityID, out Vector2 translation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetTranslation_Native(ulong entityID, ref Vector2 translation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetRoation_Native(ulong entityID, out float rotation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetRotationNative(ulong entityID, ref float rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetMass_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetMass_Native(ulong entityID, ref float mass);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetVelocity_Native(ulong entityID, out Vector2 velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetVelocity_Native(ulong entityID, ref Vector2 velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetGravityScale_Native(ulong entityID, out float gravityScale);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetGravityScale_Native(ulong entityID, ref float gravityScale);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ApplyLinearImpulse_Native(ulong entityID, ref Vector2 impulse, ref Vector2 offset, ref bool wake);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ApplyAngularImpulse_Native(ulong entityID, ref float impulse, bool wake);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddForce_Native(ulong entityID, ref Vector2 force, ref Vector2 offset, bool wake);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddTorque_Native(ulong entityID, ref float torque, bool wake);
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

        public Vector3 Position
        {
            get
            {
                GetPosition_Native(Entity.ID, out Vector3 position);
                return position;
            }
            set
            {
                SetPosition_Native(Entity.ID, ref value);
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

        public float LinearDrag
        {
            get { return GetLinearDrag_Native(Entity.ID); }
            set { SetLinearDrag_Native(Entity.ID, value); }
        }

        public float AngularDrag
        {
            get { return GetAngularDrag_Native(Entity.ID); }
            set { SetAngularDrag_Native(Entity.ID, value); }
        }

        public uint Layer { get { return GetLayer_Native(Entity.ID); } }

        public bool IsSleeping
        {
            get => IsSleeping_Native(Entity.ID);
            set => SetIsSleeping_Native(Entity.ID, value);
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

        public void SetLockFlag(ActorLockFlag flag, bool value) => SetLockFlag_Native(Entity.ID, flag, value);
        public bool IsLockFlagSet(ActorLockFlag flag) => IsLockFlagSet_Native(Entity.ID, flag);
        public uint GetLockFlags() => GetLockFlags_Native(Entity.ID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetPosition_Native(ulong entityID, out Vector3 position);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetPosition_Native(ulong entityID, ref Vector3 position);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetRotation_Native(ulong entityID, out Vector3 rotation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetRotation_Native(ulong entityID, ref Vector3 rotation);

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
        internal static extern float GetLinearDrag_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetLinearDrag_Native(ulong entityID, float linearDrag);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetAngularDrag_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetAngularDrag_Native(ulong entityID, float angularDrag);

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
        internal static extern void SetBodyType_Native(ulong entityID, Type type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetKinematicTarget_Native(ulong entityID, out Vector3 targetPosition, out Vector3 targetRotation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetKinematicTarget_Native(ulong entityID, ref Vector3 targetPosition, ref Vector3 targetRotation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetLockFlag_Native(ulong entityID, ActorLockFlag flag, bool value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsLockFlagSet_Native(ulong entityID, ActorLockFlag flag);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern uint GetLockFlags_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsSleeping_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetIsSleeping_Native(ulong entityID, bool isSleeping);
    }

    public enum CollisionFlags
    {
        None, Sides, Above, Below
    }

    public class CharacterControllerComponent : Component
    {
        public float SlopeLimit
        {
            get => GetSlopeLimit_Native(Entity.ID);
            set => SetSlopeLimit_Native(Entity.ID, value);
        }

        public float StepOffset
        {
            get => GetStepOffset_Native(Entity.ID);
            set => SetStepOffset_Native(Entity.ID, value);
        }

        public void Move(Vector3 displacement) => Move_Native(Entity.ID, ref displacement);

        public void Jump(float jumpPower) => Jump_Native(Entity.ID, jumpPower);

        public float SpeedDown => GetSpeedDown_Native(Entity.ID);

        public bool IsGrounded
        {
            get => IsGrounded_Native(Entity.ID);
        }

        public CollisionFlags CollisionFlags
        {
            get => GetCollisionFlags_Native(Entity.ID);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetSlopeLimit_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetSlopeLimit_Native(ulong entityID, float mass);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetStepOffset_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetStepOffset_Native(ulong entityID, float mass);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Move_Native(ulong entityID, ref Vector3 displacement);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Jump_Native(ulong entityID, float jumpPower);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetSpeedDown_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsGrounded_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern CollisionFlags GetCollisionFlags_Native(ulong entityID);
    }

    public class FixedJointComponent : Component
    {

        public Entity ConnectedEntity
        {
            get => new Entity(GetConnectedEntity_Native(Entity.ID));
            set => SetConnectedEntity_Native(Entity.ID, value.ID);
        }

        public bool IsBreakable
        {
            get => IsBreakable_Native(Entity.ID);
            set => SetIsBreakable_Native(Entity.ID, value);
        }

        public bool IsBroken => IsBroken_Native(Entity.ID);

        public float BreakForce
        {
            get => GetBreakForce_Native(Entity.ID);
            set => SetBreakForce_Native(Entity.ID, value);
        }

        public float BreakTorque
        {
            get => GetBreakTorque_Native(Entity.ID);
            set => SetBreakTorque_Native(Entity.ID, value);
        }

        public bool IsCollisionEnabled
        {
            get => IsCollisionEnabled_Native(Entity.ID);
            set => SetCollisionEnabled_Native(Entity.ID, value);
        }

        public bool IsPreProcessingEnabled
        {
            get => IsPreProcessingEnabled_Native(Entity.ID);
            set => SetPreProcessingEnabled_Native(Entity.ID, value);
        }

        public void Break() => Break_Native(Entity.ID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong GetConnectedEntity_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetConnectedEntity_Native(ulong entityID, ulong connectedEntityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsBreakable_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetIsBreakable_Native(ulong entityID, bool isBreakable);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsBroken_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Break_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetBreakForce_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetBreakForce_Native(ulong entityID, float breakForce);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetBreakTorque_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetBreakTorque_Native(ulong entityID, float breakForce);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsCollisionEnabled_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetCollisionEnabled_Native(ulong entityID, bool isCollisionEnabled);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsPreProcessingEnabled_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetPreProcessingEnabled_Native(ulong entityID, bool isPreProcessingEnabled);

    }

    public class BoxColliderComponent : Component
    {

        public Vector3 Size
        {
            get
            {
                GetSize_Native(Entity.ID, out Vector3 size);
                return size;
            }

            //set { SetSize_Native(Entity.ID, ref value); }
        }

        public Vector3 Offset
        {
            get
            {
                GetOffset_Native(Entity.ID, out Vector3 offset);
                return offset;
            }

            //set { SetOffset_Native(Entity.ID, ref value); }
        }

        public bool IsTrigger
        {
            get => IsTrigger_Native(Entity.ID);
            //set => SetTrigger_Native(Entity.ID, value);
        }


        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetSize_Native(ulong entityID, out Vector3 size);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetSize_Native(ulong entityID, ref Vector3 size);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetOffset_Native(ulong entityID, out Vector3 offset);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetOffset_Native(ulong entityID, ref Vector3 offset);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsTrigger_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetTrigger_Native(ulong entityID, bool isTrigger);
    }

    public class SphereColliderComponent : Component
    {

        public float Radius
        {
            get => GetRadius_Native(Entity.ID);
            //set => SetRadius_Native(Entity.ID, value);
        }

        public Vector3 Offset
        {
            get
            {
                GetOffset_Native(Entity.ID, out Vector3 offset);
                return offset;
            }

            //set { SetOffset_Native(Entity.ID, ref value); }
        }

        public bool IsTrigger
        {
            get => IsTrigger_Native(Entity.ID);
            //set => SetIsTrigger_Native(Entity.ID, value);
        }


        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetRadius_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetRadius_Native(ulong entityID, float radius);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetOffset_Native(ulong entityID, out Vector3 offset);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetOffset_Native(ulong entityID, ref Vector3 offset);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsTrigger_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetTrigger_Native(ulong entityID, bool isTrigger);
    }

    public class CapsuleColliderComponent : Component
    {

        public float Radius
        {
            get => GetRadius_Native(Entity.ID);
            //set => SetRadius_Native(Entity.ID, value);
        }

        public float Height
        {
            get => GetHeight_Native(Entity.ID);
            //set => SetHeight_Native(Entity.ID, value);
        }

        public Vector3 Offset
        {
            get
            {
                GetOffset_Native(Entity.ID, out Vector3 offset);
                return offset;
            }

            //set { SetOffset_Native(Entity.ID, ref value); }
        }

        public bool IsTrigger
        {
            get => IsTrigger_Native(Entity.ID);
            //set => SetIsTrigger_Native(Entity.ID, value);
        }


        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetRadius_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetRadius_Native(ulong entityID, float radius);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetHeight_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetHeight_Native(ulong entityID, float height);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetOffset_Native(ulong entityID, out Vector3 offset);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetOffset_Native(ulong entityID, ref Vector3 offset);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsTrigger_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetTrigger_Native(ulong entityID, bool isTrigger);
    }

    public class MeshColliderComponent : Component
    {
        public Mesh ColliderMesh
        {
            get
            {
                bool isStatic = true;
                IntPtr mesh = GetColliderMesh_Native(Entity.ID, ref isStatic);
                Mesh result = new Mesh(mesh, isStatic);
                return result;
            }
            /*set
			{
				IntPtr ptr = value == null ? IntPtr.Zero : value.m_UnmanagedInstance;
				SetColliderMesh_Native(Entity.ID, ptr);
			}*/
        }

        public bool IsConvex
        {
            get => IsConvex_Native(Entity.ID);
            //set => SetConvex_Native(Entity.ID, value);
        }

        public bool IsTrigger
        {
            get => IsTrigger_Native(Entity.ID);
            //set => SetIsTrigger_Native(Entity.ID, value);
        }


        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern IntPtr GetColliderMesh_Native(ulong entityID, ref bool outIsStatic);
        [MethodImpl(MethodImplOptions.InternalCall)] 
        internal static extern void SetColliderMesh_Native(ulong entityID, IntPtr mesh, bool isStatic);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsConvex_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetConvex_Native(ulong entityID, bool isConvex);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsTrigger_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetTrigger_Native(ulong entityID, bool isTrigger);

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

        public bool Resume()
        {
            return Resume_Native(Entity.ID);
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
        public void SetEvent(Audio.CommandID eventID)
        {
            SetEvent_Native(Entity.ID, (uint)eventID.GetHashCode());
        }

        public void SetEvent(string eventName)
        {
            Audio.CommandID command = new Audio.CommandID(eventName);
            SetEvent_Native(Entity.ID, (uint)command.GetHashCode());
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
        internal static extern bool Resume_Native(ulong entityID);


        //=================================================================================================

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetVolumeMult_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetVolumeMult_Native(ulong entityID, float volumeMult);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetPitchMult_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetPitchMult_Native(ulong entityID, float pitchMult);


        //=================================================================================================

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetEvent_Native(ulong entityID, uint eventID);
    }

    public class TextComponent : Component
    {
        public string Text
        {
            get => GetText_Native(Entity.ID);
            set => SetText_Native(Entity.ID, value);
        }
        public Vector4 Color
        {
            get
            {
                GetColor_Native(Entity.ID, out Vector4 color);
                return color;
            }
            set => SetColor_Native(Entity.ID, ref value);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string GetText_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetText_Native(ulong entityID, string text);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetColor_Native(ulong entityID, out Vector4 outColor);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetColor_Native(ulong entityID, ref Vector4 inColor);
    }
}