﻿using NR;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

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

        public Material BaseMaterial
        {
            get
            {
                return new Material(GetMaterial_Native(_unmanagedInstance));
            }
        }

        public MaterialInstance GetMaterial(int index)
        {
            return new MaterialInstance(GetMaterialByIndex_Native(_unmanagedInstance, index));
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
        public static extern IntPtr GetMaterialByIndex_Native(IntPtr unmanagedInstance, int index);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int GetMaterialCount_Native(IntPtr unmanagedInstance);

    }
}