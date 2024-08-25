using NR;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace NR
{
    public class Material
    {
        public void Set(string uniform, float value)
        {
            SetFloat_Native(_unmanagedInstance, uniform, value);
        }

        public void Set(string uniform, Texture2D texture)
        {
            SetTexture_Native(_unmanagedInstance, uniform, texture._unmanagedInstance);
        }

        public void SetTexture(string uniform, Texture2D texture)
        {
            SetTexture_Native(_unmanagedInstance, uniform, texture._unmanagedInstance);
        }

        internal Material(IntPtr unmanagedInstance)
        {
            _unmanagedInstance = unmanagedInstance;
        }

        ~Material()
        {
            Destructor_Native(_unmanagedInstance);
        }

        internal IntPtr _unmanagedInstance;

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Destructor_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFloat_Native(IntPtr unmanagedInstance, string uniform, float value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTexture_Native(IntPtr unmanagedInstance, string uniform, IntPtr texture);

    }

    public class MaterialInstance
    {
        public void Set(string uniform, float value)
        {
            SetFloat_Native(_unmanagedInstance, uniform, value);
        }

        public void Set(string uniform, Texture2D texture)
        {
            SetTexture_Native(_unmanagedInstance, uniform, texture._unmanagedInstance);
        }

        public void Set(string uniform, Vector3 value)
        {
            SetVector3_Native(_unmanagedInstance, uniform, ref value);
        }

        public void SetTexture(string uniform, Texture2D texture)
        {
            SetTexture_Native(_unmanagedInstance, uniform, texture._unmanagedInstance);
        }

        public void Set(string uniform, Vector4 value)
        {
            SetVector4_Native(_unmanagedInstance, uniform, ref value);
        }

        internal MaterialInstance(IntPtr unmanagedInstance)
        {
            _unmanagedInstance = unmanagedInstance;
        }

        ~MaterialInstance()
        {
            Destructor_Native(_unmanagedInstance);
        }

        internal IntPtr _unmanagedInstance;

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Destructor_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFloat_Native(IntPtr unmanagedInstance, string uniform, float value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetVector3_Native(IntPtr unmanagedInstance, string uniform, ref Vector3 value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetVector4_Native(IntPtr unmanagedInstance, string uniform, ref Vector4 value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTexture_Native(IntPtr unmanagedInstance, string uniform, IntPtr texture);
    }
}