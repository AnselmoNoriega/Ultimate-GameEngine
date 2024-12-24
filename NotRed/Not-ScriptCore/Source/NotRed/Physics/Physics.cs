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

    public static class Physics
    {
        public static bool Raycast(RaycastData raycastData, out RaycastHit hit) => RaycastWithStruct_Native(ref raycastData, out hit);
        public static bool Raycast(Vector3 origin, Vector3 direction, float maxDistance, out RaycastHit hit) => Raycast_Native(ref origin, ref direction, maxDistance, null, out hit);
        public static bool Raycast(Vector3 origin, Vector3 direction, float maxDistance, out RaycastHit hit, params Type[] componentFilters) => Raycast_Native(ref origin, ref direction, maxDistance, componentFilters, out hit);

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

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool RaycastWithStruct_Native(ref RaycastData raycastData, out RaycastHit hit);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Raycast_Native(ref Vector3 origin, ref Vector3 direction, float maxDistance, Type[] requiredComponents, out RaycastHit hit);
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
    }
}