using NR;
using System;
using System.Runtime.InteropServices;

namespace NR
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Quaternion : IEquatable<Quaternion>
    {
        public float W;
        public float x;
        public float y;
        public float z;
        public Quaternion(Vector3 euler)
        {
            Vector3 c = Vector3.Cos(euler * 0.5f);
            Vector3 s = Vector3.Sin(euler * 0.5f);
            W = c.x * c.y * c.z + s.x * s.y * s.z;
            x = s.x * c.y * c.z - c.x * s.y * s.z;
            y = c.x * s.y * c.z + s.x * c.y * s.z;
            z = c.x * c.y * s.z - s.x * s.y * c.z;
        }
        public static Vector3 operator *(Quaternion q, Vector3 v)
        {
            Vector3 qv = new Vector3(q.x, q.y, q.z);
            Vector3 uv = Vector3.Cross(qv, v);
            Vector3 uuv = Vector3.Cross(qv, uv);
            return v + ((uv * q.W) + uuv) * 2.0f;
        }
        public override int GetHashCode() => (W, x, y, z).GetHashCode();
        public override bool Equals(object obj) => obj is Quaternion other && this.Equals(other);
        public bool Equals(Quaternion right) => W == right.W && x == right.x && y == right.y && z == right.z;
        public static bool operator ==(Quaternion left, Quaternion right) => left.Equals(right);
        public static bool operator !=(Quaternion left, Quaternion right) => !(left == right);
    }
}