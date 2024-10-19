using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace NR
{
    [StructLayout(LayoutKind.Explicit)]
    public struct Matrix4
    {
        [FieldOffset(0)] public float a0;
        [FieldOffset(4)] public float b0;
        [FieldOffset(8)] public float c0;
        [FieldOffset(12)] public float d0;

        [FieldOffset(16)] public float a1;
        [FieldOffset(20)] public float b1;
        [FieldOffset(24)] public float c1;
        [FieldOffset(28)] public float d1;

        [FieldOffset(32)] public float a2;
        [FieldOffset(36)] public float b2;
        [FieldOffset(40)] public float c2;
        [FieldOffset(44)] public float d2;

        [FieldOffset(48)] public float a3;
        [FieldOffset(52)] public float b3;
        [FieldOffset(56)] public float c3;
        [FieldOffset(60)] public float d3;

        public Matrix4(float value)
        {
            a0 = value; b0 = 0.0f;  c0 = 0.0f;  d0 = 0.0f;
            a1 = 0.0f;  b1 = value; c1 = 0.0f;  d1 = 0.0f;
            a2 = 0.0f;  b2 = 0.0f;  c2 = value; d2 = 0.0f;
            a3 = 0.0f;  b3 = 0.0f;  c3 = 0.0f;  d3 = value;
        }

        public Vector3 Position
        {
            get { return new Vector3(a3, b3, c3); }
            set { a3 = value.x; b3 = value.y; c3 = value.z; }
        }

        public static Matrix4 Translate(Vector3 position)
        {
            Matrix4 result = new Matrix4(1.0f);
            result.a3 = position.x;
            result.b3 = position.y;
            result.c3 = position.z;
            return result;
        }

        public static Matrix4 Scale(Vector3 scale)
        {
            Matrix4 result = new Matrix4(1.0f);
            result.a0 = scale.x;
            result.b1 = scale.y;
            result.c2 = scale.z;
            return result;
        }

        public static Matrix4 Scale(float scale)
        {
            Matrix4 result = new Matrix4(1.0f);
            result.a0 = scale;
            result.b1 = scale;
            result.c2 = scale;
            return result;
        }

        public void DebugPrint()
        {
            Console.WriteLine("{0:0.00}  {1:0.00}  {2:0.00}  {3:0.00}", a0, b0, c0, d0);
            Console.WriteLine("{0:0.00}  {1:0.00}  {2:0.00}  {3:0.00}", a1, b1, c1, d1);
            Console.WriteLine("{0:0.00}  {1:0.00}  {2:0.00}  {3:0.00}", a2, b2, c2, d2);
            Console.WriteLine("{0:0.00}  {1:0.00}  {2:0.00}  {3:0.00}", a3, b3, c3, d3);
        }

    }
}