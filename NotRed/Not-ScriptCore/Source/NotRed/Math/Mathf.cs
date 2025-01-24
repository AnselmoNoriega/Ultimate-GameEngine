using System;

namespace NR
{
    public static class Mathf
    {
        public const float PI = (float)Math.PI;

        public const float Deg2Rad = PI / 180.0f;
        public const float Rad2Deg = 180.0f / PI;

        public static float Sin(float value)
        {
            return (float)Math.Sin(value);
        }

        public static float Cos(float value)
        {
            return (float)Math.Cos(value);
        }

        public static float Clamp(float value, float min, float max)
        {
            if (value < min)
            {
                return min;
            }
            if (value > max)
            {
                return max;
            }

            return value;
        }
        public static float Atan2(float y, float x)
        {
            return (float)System.Math.Atan2((double)y, (double)x);
        }

        public static float Min(float v0, float v1)
        {
            return v0 < v1 ? v0 : v1;
        }

        public static float Max(float v0, float v1)
        {
            return v0 > v1 ? v0 : v1;
        }

        public static float Sqrt(float value)
        {
            return (float)Math.Sqrt(value);
        }

        public static float Pow(float baseValue, float exponent)
        {
            if (exponent == 0)
            {
                return 1.0f;
            }

            return (float)Math.Exp(exponent * Math.Log(baseValue));
        }

        public static float Abs(float value)
        {
            return Math.Abs(value);
        }

        public static Vector3 Abs(Vector3 value)
        {
            return new Vector3(Math.Abs(value.x), Math.Abs(value.y), Math.Abs(value.z));
        }

        public static float Lerp(float p1, float p2, float t)
        {
            if (t < 0.0f)
            {
                return p1;
            }
            else if (t > 1.0f)
            {
                return p2;
            }
            return p1 + ((p2 - p1) * t);
        }

        public static float Modulo(float a, float b)
        {
            return a - b * (float)Math.Floor(a / b);
        }

        public static int FloorToInt(float value)
        {
            return (int)Math.Floor(value);
        }

        public static int RoundToInt(float value)
        {
            return (int)Math.Round(value);
        }
    }
}