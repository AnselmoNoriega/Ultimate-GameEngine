using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.Serialization.Formatters.Binary;

namespace NR
{
    public class Mesh
    {
        public Mesh(string filepath)
        {
            _unmanagedInstance = Constructor_Native(filepath);
        }

        internal Mesh(IntPtr unmanagedInstance)
        {
            _unmanagedInstance = unmanagedInstance;
        }

        ~Mesh()
        {
            Destructor_Native(_unmanagedInstance);
        }

        public Vector3[] Vertices 
        {  
            get
            {
                var data = GetVertices_Native(_unmanagedInstance);
                using (var ms = new MemoryStream(data))
                {
                    var formatter = new BinaryFormatter();
                    return (Vector3[])formatter.Deserialize(ms);
                }
            }
            set
            {
                using (var ms = new MemoryStream())
                {
                    var formatter = new BinaryFormatter();
                    formatter.Serialize(ms, value);
                    SetVertices_Native(_unmanagedInstance, ms.ToArray());
                }
            }
        }

        public Material BaseMaterial
        {
            get
            {
                return new Material(GetMaterial_Native(_unmanagedInstance));
            }
        }

        public Material GetMaterial(int index)
        {
            IntPtr result = GetMaterialByIndex_Native(_unmanagedInstance, index);
            if (result == null)
            {
                return null;
            }

            return new Material(result);
        }

        public int GetMaterialCount()
        {
            return GetMaterialCount_Native(_unmanagedInstance);
        }

        internal IntPtr _unmanagedInstance;

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr Constructor_Native(string filepath);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Destructor_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr GetMaterial_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern byte[] GetVertices_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetVertices_Native(IntPtr unmanagedInstance, byte[] data);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr GetMaterialByIndex_Native(IntPtr unmanagedInstance, int index);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int GetMaterialCount_Native(IntPtr unmanagedInstance);

    }
}