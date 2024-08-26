using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace NR
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector3
    {
        public float x;
        public float y;
        public float z;

        public Vector3(float scalar)
        {
            x = y = z = scalar;
        }

        public Vector3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public Vector3(Vector2 vector)
        {
            x = vector.x;
            y = vector.y;
            z = 0.0f;
        }

        public Vector3(Vector4 vector)
        {
            x = vector.x;
            y = vector.y;
            z = vector.z;
        }

        public Vector2 xy
        {
            get { return new Vector2(x, y); }
            set { x = value.x; y = value.y; }
        }
        public Vector2 xz
        {
            get { return new Vector2(x, z); }
            set { x = value.x; z = value.y; }
        }
        public Vector2 yz
        {
            get { return new Vector2(y, z); }
            set { y = value.x; z = value.y; }
        }
    }
}