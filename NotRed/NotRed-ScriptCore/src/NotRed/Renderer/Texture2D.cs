using NR;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace NR
{
    public class Texture2D
    {
        public Texture2D(uint width, uint height)
        {
            _unmanagedInstance = Constructor_Native(width, height);
        }

        ~Texture2D()
        {
            Destructor_Native(_unmanagedInstance);
        }

        public void SetData(Vector4[] data)
        {
            SetData_Native(_unmanagedInstance, data, data.Length);
        }

        internal IntPtr _unmanagedInstance;

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern IntPtr Constructor_Native(uint width, uint height);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Destructor_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetData_Native(IntPtr unmanagedInstance, Vector4[] data, int size);
    }
}