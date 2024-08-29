using NR;
using System;
using System.Runtime.InteropServices;

namespace NR
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Quaternion
    {
        public float x;
        public float y;
        public float z;
        public float w;

        public Quaternion(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        public Quaternion(float w, Vector3 xyz)
        {
            x = xyz.x;
            y = xyz.y;
            z = xyz.z;
            this.w = w;
        }

        public Quaternion(Vector3 eulerAngles)
        {
            Vector3 c = Vector3.Cos(eulerAngles * 0.5f);
            Vector3 s = Vector3.Sin(eulerAngles * 0.5f);

            w = c.x * c.y * c.z + s.x * s.y * s.z;
            x = s.x * c.y * c.z - c.x * s.y * s.z;
            y = c.x * s.y * c.z + s.x * c.y * s.z;
            z = c.x * c.y * s.z - s.x * s.y * c.z;
        }

        public static Quaternion AngleAxis(float angle, Vector3 axis)
        {
            float s = (float)Math.Sin(angle * 0.5f);
            return new Quaternion(s, axis * s);
        }

        public static Quaternion operator *(Quaternion a, Quaternion b)
        {
            float w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
            float x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
            float y = a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z;
            float z = a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x;
            return new Quaternion(x, y, z, w);
        }
    }
}