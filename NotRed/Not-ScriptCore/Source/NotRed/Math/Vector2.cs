using System;
using System.Runtime.InteropServices;

namespace NR
{
    [StructLayout(LayoutKind.Sequential), Serializable]
    public struct Vector2
    {
        public static Vector2 Zero = new Vector2(0, 0);
        public static Vector2 Right = new Vector2(1, 0);
        public static Vector2 Left = new Vector2(-1, 0);
        public static Vector2 Up = new Vector2(0, 1);
        public static Vector2 Down = new Vector2(0, -1);

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
            x = Mathf.Clamp(x, min.x, max.x);
            y = Mathf.Clamp(y, min.y, max.y);
        }

        public float Length()
        {
            return (float)Math.Sqrt(x * x + y * y);
        }
        public Vector2 Normalized()
        {
            float length = Length();
            float x = this.x;
            float y = this.y;
            if (length > 0.0f)
            {
                x /= length;
                y /= length;
            }
            return new Vector2(x, y);
        }
        public void Normalize()
        {
            float length = Length();
            if (length > 0.0f)
            {
                x = x / length;
                y = y / length;
            }
        }

        public float Distance(Vector3 other)
        {
            return (float)Math.Sqrt(Math.Pow(other.x - x, 2) + Math.Pow(other.y - y, 2));
        }

        public static float Distance(Vector3 p1, Vector3 p2)
        {
            return (float)Math.Sqrt(Math.Pow(p2.x - p1.x, 2) + Math.Pow(p2.y - p1.y, 2));
        }

        public static Vector2 operator *(Vector2 left, float scalar)
        {
            return new Vector2(left.x * scalar, left.y * scalar);
        }
        public static Vector2 operator *(float scalar, Vector2 right)
        {
            return new Vector2(scalar * right.x, scalar * right.y);
        }
        public static Vector2 operator *(Vector2 left, Vector2 right)
        {
            return new Vector2(left.x * right.x, left.y * right.y);
        }
        public static Vector2 operator /(Vector2 left, Vector2 right)
        {
            return new Vector2(left.x / right.x, left.y / right.y);
        }
        public static Vector2 operator /(Vector2 left, float scalar)
        {
            return new Vector2(left.x / scalar, left.y / scalar);
        }
        public static Vector2 operator /(float scalar, Vector2 right)
        {
            return new Vector2(scalar / right.x, scalar / right.y);
        }
        public static Vector2 operator +(Vector2 left, Vector2 right)
        {
            return new Vector2(left.x + right.x, left.y + right.y);
        }
        public static Vector2 operator +(Vector2 left, float right)
        {
            return new Vector2(left.x + right, left.y + right);
        }
        public static Vector2 operator -(Vector2 left, Vector2 right)
        {
            return new Vector2(left.x - right.x, left.y - right.y);
        }
        public static Vector2 operator -(Vector2 vector)
        {
            return new Vector2(-vector.x, -vector.y);
        }

        public override string ToString()
        {
            return "Vector2[" + x + ", " + y + "]";
        }
    }
}