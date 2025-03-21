﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Serialization.Formatters.Binary;

namespace NR
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vertex
    {
        public Vector3 Position;
        public Vector3 Normal;
        public Vector3 Tangent;
        public Vector3 Binormal;
        public Vector2 Texcoord;
    }

    public class Mesh
    {
        public Mesh(string filepath)
        {
            _unmanagedInstance = Constructor_Native(filepath, ref _isStatic);
        }

        internal Mesh(IntPtr unmanagedInstance, bool isStatic)
        {
            _unmanagedInstance = unmanagedInstance;
            _isStatic = isStatic;
        }

        ~Mesh()
        {
            Destructor_Native(_unmanagedInstance, _isStatic);
        }

        public Vertex[] Vertices
        {
            get
            {
                var data = GetVertices();
                using (var ms = new MemoryStream(data))
                {
                    var formatter = new BinaryFormatter();
                    return (Vertex[])formatter.Deserialize(ms);
                }
            }
            set => SetVertices(value, 0);
        }

        public int[] Indices
        {
            get => GetIndices();

            set => SetIndices(value, 0);
        }

        public int SubMeshCount
        {
            get
            {
                return GetSubMeshCount_Native(_unmanagedInstance);
            }

            set
            {
                SetSubMeshCount_Native(_unmanagedInstance, value);
            }
        }

        public Material BaseMaterial
        {
            get
            {
                return new Material(GetMaterial_Native(_unmanagedInstance, _isStatic));
            }
        }

        public void Clear()
        {
            // TODO
            //Vertices = null;
        }

        public Material GetMaterial(int index)
        {
            IntPtr result = GetMaterialByIndex_Native(_unmanagedInstance, index, _isStatic);
            if (result == null)
            {
                return null;
            }

            return new Material(result);
        }

        public void SetVertices(Vertex[] vertices, int index)
        {
            using (var ms = new MemoryStream())
            {
                var formatter = new BinaryFormatter();
                formatter.Serialize(ms, vertices);
                SetVertices_Native(_unmanagedInstance, ms.ToArray(), index);
            }
        }

        public byte[] GetVertices()
        {
            return GetVertices_Native(_unmanagedInstance);
        }

        public void SetIndices(int[] indices, int index)
        {
            SetIndices_Native(_unmanagedInstance, indices, index);
        }

        public int[] GetIndices()
        {
            return GetIndices_Native(_unmanagedInstance);
        }

        public int GetMaterialCount()
        {
            return GetMaterialCount_Native(_unmanagedInstance, _isStatic);
        }

        internal IntPtr _unmanagedInstance;
        internal bool _isStatic = true;
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern byte[] GetVertices_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetVertices_Native(IntPtr unmanagedInstance, byte[] data, int index);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int[] GetIndices_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetIndices_Native(IntPtr unmanagedInstance, int[] data, int index);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int GetSubMeshCount_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetSubMeshCount_Native(IntPtr unmanagedInstance, int count);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr Constructor_Native(string filepath, ref bool outIsStatic);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Destructor_Native(IntPtr unmanagedInstance, bool isStatic);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr GetMaterial_Native(IntPtr unmanagedInstance, bool isStatic);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr GetMaterialByIndex_Native(IntPtr unmanagedInstance, int index, bool isStatic);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int GetMaterialCount_Native(IntPtr unmanagedInstance, bool isStatic);
    }
}