using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace NR
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2
    {
        public float x;
        public float y;

        public Vector2(float scalar)
        {
            x = y = scalar;
        }

        public Vector2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }

        public Vector2(Vector3 vector)
        {
            this.x = vector.x;
            this.y = vector.y;
        }

        public void Clamp(Vector2 min, Vector2 max)
        {
            if (x < min.x)
            {
                x = min.x;
            }
            if (x > max.x)
            {
                x = max.x;
            }
            if (y < min.y)
            {
                y = min.y;
            }
            if (y > max.y)
            {
                y = max.y;
            }
        }

        public static Vector2 operator -(Vector2 vector)
        {
            return new Vector2(-vector.x, -vector.y);
        }
    }
}