using NR;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace NR
{
    public enum ForceMode
    {
        Force,
        Impulse,
        VelocityChange,
        Acceleration
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RaycastHit
    {
        public ulong EntityID { get; internal set; }
        public Vector3 Position { get; internal set; }
        public Vector3 Normal { get; internal set; }
        public float Distance { get; internal set; }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RaycastData
    {
        public Vector3 Origin;
        public Vector3 Direction;
        public float MaxDistance;
        public Type[] RequiredComponents;
    }

    public enum ActorLockFlag : uint
    {
        TranslationX = 1 << 0,
        TranslationY = 1 << 1,
        TranslationZ = 1 << 2,
        TranslationXYZ = TranslationX | TranslationY | TranslationZ,

        RotationX = 1 << 3,
        RotationY = 1 << 4,
        RotationZ = 1 << 5,
        RotationXYZ = RotationX | RotationY | RotationZ
    }

    public enum EFalloffMode { Constant, Linear }

    public static class Physics
    {
        public static Vector3 Gravity
        {
            get
            {
                GetGravity_Native(out Vector3 gravity);
                return gravity;
            }

            set => SetGravity_Native(ref value);
        }
        public static void AddRadialImpulse(Vector3 origin, float radius, float strength, EFalloffMode falloff = EFalloffMode.Constant, bool velocityChange = false)
        {
            AddRadialImpulse_Native(ref origin, radius, strength, falloff, velocityChange);
        }

        public static bool Raycast(RaycastData raycastData, out RaycastHit hit) => Raycast_Native(ref raycastData, out hit);

        public static bool Raycast(Vector3 origin, Vector3 direction, float maxDistance, out RaycastHit hit, params Type[] componentFilters)
        {
            sRaycastData.Origin = origin;
            sRaycastData.Direction = direction;
            sRaycastData.MaxDistance = maxDistance;
            sRaycastData.RequiredComponents = componentFilters;
            return Raycast_Native(ref sRaycastData, out hit);
        }

        public static Collider[] OverlapBox(Vector3 origin, Vector3 halfSize)
        {
            return OverlapBox_Native(ref origin, ref halfSize);
        }

        public static Collider[] OverlapSphere(Vector3 origin, float radius)
        {
            return OverlapSphere_Native(ref origin, radius);
        }

        public static Collider[] OverlapCapsule(Vector3 origin, float radius, float halfHeight)
        {
            return OverlapCapsule_Native(ref origin, radius, halfHeight);
        }

        public static int OverlapBoxNonAlloc(Vector3 origin, Vector3 halfSize, Collider[] colliders)
        {
            return OverlapBoxNonAlloc_Native(ref origin, ref halfSize, colliders);
        }

        public static int OverlapCapsuleNonAlloc(Vector3 origin, float radius, float halfHeight, Collider[] colliders)
        {
            return OverlapCapsuleNonAlloc_Native(ref origin, radius, halfHeight, colliders);
        }

        public static int OverlapSphereNonAlloc(Vector3 origin, float radius, Collider[] colliders)
        {
            return OverlapSphereNonAlloc_Native(ref origin, radius, colliders);
        }

        private static RaycastData sRaycastData = new RaycastData();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Raycast_Native(ref RaycastData raycastData, out RaycastHit hit);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Collider[] OverlapBox_Native(ref Vector3 origin, ref Vector3 halfSize);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Collider[] OverlapCapsule_Native(ref Vector3 origin, float radius, float halfHeight);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Collider[] OverlapSphere_Native(ref Vector3 origin, float radius);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern int OverlapBoxNonAlloc_Native(ref Vector3 origin, ref Vector3 halfSize, Collider[] colliders);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern int OverlapCapsuleNonAlloc_Native(ref Vector3 origin, float radius, float halfHeight, Collider[] colliders);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern int OverlapSphereNonAlloc_Native(ref Vector3 origin, float radius, Collider[] colliders);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetGravity_Native(out Vector3 gravity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetGravity_Native(ref Vector3 gravity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void AddRadialImpulse_Native(ref Vector3 origin, float radius, float strength, EFalloffMode falloff, bool velocityChange);
    }
}