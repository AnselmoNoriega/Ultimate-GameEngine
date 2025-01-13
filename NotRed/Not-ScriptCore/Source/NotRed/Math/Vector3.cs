using System;
using System.Runtime.InteropServices;

namespace NR
{
    [StructLayout(LayoutKind.Sequential), Serializable]
    public struct Vector3 : IEquatable<Vector3>
    {
        public static Vector3 zero = new Vector3(0, 0, 0);

        public static Vector3 Forward = new Vector3(0, 0, -1);
        public static Vector3 Right = new Vector3(1, 0, 0);
        public static Vector3 Up = new Vector3(0, 1, 0);
        public static Vector3 Down = new Vector3(0, -1, 0);

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

        public Vector3(float x, Vector2 yz)
        {
            this.x = x;
            y = yz.x;
            z = yz.y;
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

        public void Clamp(Vector3 min, Vector3 max)
        {
            x = Mathf.Clamp(x, min.x, max.x);
            y = Mathf.Clamp(y, min.y, max.y);
            z = Mathf.Clamp(z, min.z, max.z);
        }

        public float Length()
        {
            return (float)Math.Sqrt(x * x + y * y + z * z);
        }

        public Vector3 Normalized()
        {
            float length = Length();
            float x = this.x;
            float y = this.y;
            float z = this.z;

            if (length > 0.0f)
            {
                x /= length;
                y /= length;
                z /= length;
            }

            return new Vector3(x, y, z);
        }

        public void Normalize()
        {
            float length = Length();

            if (length > 0.0f)
            {
                x = x / length;
                y = y / length;
                z = z / length;
            }
        }

        public float Distance(Vector3 other)
        {
            return (float)Math.Sqrt(Math.Pow(other.x - x, 2) +
                                            Math.Pow(other.y - y, 2) +
                                            Math.Pow(other.z - z, 2));
        }

        public static float Distance(Vector3 p1, Vector3 p2)
        {
            return (float)Math.Sqrt(Math.Pow(p2.x - p1.x, 2) +
                                        Math.Pow(p2.y - p1.y, 2) +
                                        Math.Pow(p2.z - p1.z, 2));
        }

        public static Vector3 Lerp(Vector3 p1, Vector3 p2, float maxDistanceDelta)
        {
            if (maxDistanceDelta < 0.0f)
            {
                return p1;
            }
            if (maxDistanceDelta > 1.0f)
            {
                return p2;
            }

            return p1 + ((p2 - p1) * maxDistanceDelta);
        }

        public static Vector3 operator *(Vector3 left, float scalar)
        {
            return new Vector3(left.x * scalar, left.y * scalar, left.z * scalar);
        }

        public static Vector3 operator *(float scalar, Vector3 right)
        {
            return new Vector3(scalar * right.x, scalar * right.y, scalar * right.z);
        }

        public static Vector3 operator *(Vector3 left, Vector3 right)
        {
            return new Vector3(left.x * right.x, left.y * right.y, left.z * right.z);
        }

        public static Vector3 operator +(Vector3 left, Vector3 right)
        {
            return new Vector3(left.x + right.x, left.y + right.y, left.z + right.z);
        }

        public static Vector3 operator +(Vector3 left, float right)
        {
            return new Vector3(left.x + right, left.y + right, left.z + right);
        }

        public static Vector3 operator -(Vector3 left, Vector3 right)
        {
            return new Vector3(left.x - right.x, left.y - right.y, left.z - right.z);
        }

        public static Vector3 operator /(Vector3 left, Vector3 right)
        {
            return new Vector3(left.x / right.x, left.y / right.y, left.z / right.z);
        }

        public static Vector3 operator /(Vector3 left, float scalar)
        {
            return new Vector3(left.x / scalar, left.y / scalar, left.z / scalar);
        }

        public static Vector3 operator -(Vector3 vector)
        {
            return new Vector3(-vector.x, -vector.y, -vector.z);
        }

        public override bool Equals(object obj) => obj is Vector3 other && this.Equals(other);

        public bool Equals(Vector3 right) => x == right.x && y == right.y && z == right.z;

        public override int GetHashCode() => (x, y, z).GetHashCode();

        public static bool operator ==(Vector3 left, Vector3 right) => left.Equals(right);

        public static bool operator !=(Vector3 left, Vector3 right) => !(left == right);

        public static Vector3 Cos(Vector3 vector)
        {
            return new Vector3((float)Math.Cos(vector.x), (float)Math.Cos(vector.y), (float)Math.Cos(vector.z));
        }

        public static Vector3 Sin(Vector3 vector)
        {
            return new Vector3((float)Math.Sin(vector.x), (float)Math.Sin(vector.y), (float)Math.Sin(vector.z));
        }

        public override string ToString()
        {
            return "Vector3[" + x + ", " + y + ", " + z + "]";
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