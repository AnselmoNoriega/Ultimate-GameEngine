using System;
using System.Runtime.CompilerServices;

namespace NR
{
    public class Material
    {
        public Vector3 AlbedoColor
        {
            get
            {
                GetAlbedoColor_Native(_unmanagedInstance, out Vector3 result);
                return result;
            }
            set
            {
                SetAlbedoColor_Native(_unmanagedInstance, ref value);
            }
        }

        public float Metalness
        {
            get
            {
                GetMetalness_Native(_unmanagedInstance, out float result);
                return result;
            }
            set
            {
                SetMetalness_Native(_unmanagedInstance, value);
            }
        }

        public float Roughness
        {
            get
            {
                GetRoughness_Native(_unmanagedInstance, out float result);
                return result;
            }
            set
            {
                SetRoughness_Native(_unmanagedInstance, value);
            }
        }

        public float Emission
        {
            get
            {
                GetEmission_Native(_unmanagedInstance, out float result);
                return result;
            }
            set
            {
                SetEmission_Native(_unmanagedInstance, value);
            }
        }

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

        public void Set(string uniform, Vector4 value)
        {
            SetVector4_Native(_unmanagedInstance, uniform, ref value);
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
        internal static extern void GetAlbedoColor_Native(IntPtr unmanagedInstance, out Vector3 outAlbedoColor);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetAlbedoColor_Native(IntPtr unmanagedInstance, ref Vector3 inAlbedoColor);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetMetalness_Native(IntPtr unmanagedInstance, out float outMetalness);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetMetalness_Native(IntPtr unmanagedInstance, float inMetalness);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetRoughness_Native(IntPtr unmanagedInstance, out float outRoughness);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetRoughness_Native(IntPtr unmanagedInstance, float inRoughness);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetEmission_Native(IntPtr unmanagedInstance, out float outEmission);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetEmission_Native(IntPtr unmanagedInstance, float inEmission);


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