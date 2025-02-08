using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace NR
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Transform
    {
        public Vector3 Position;
        public Vector3 Rotation;
        public Vector3 Scale;

        public Vector3 Up { get; }
        public Vector3 Right { get; }
        public Vector3 Forward { get; }

        public static Transform operator *(Transform a, Transform b)
        {
            TransformMultiply_Native(a, b, out Transform result);
            return result;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void TransformMultiply_Native(Transform a, Transform b, out Transform result);
    }
}