using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Serialization.Formatters.Binary;

namespace NR
{
    public static class MeshFactory
    {
        public static Mesh CreatePlane(float width, float height)
        {
            return new Mesh(CreatePlane_Native(width, height), true);
        }

        public static Mesh CreateCustomMesh(Vertex[][] vertices, int[][] indices, string[] materialNames)
        {
            if (vertices.Length != indices.Length || vertices.Length < 1)
            {
                Log.Error("[MeshFactory] Mismatch between vertex count and index count across submeshes.");
                return null;
            }

            // Indices to know where the next vertices begin and where they end
            int[] vertexOffsets = new int[vertices.Length];
            int[] indexOffsets = new int[indices.Length];
            List<Vertex> verticesList = new List<Vertex>();
            List<int> indicesList = new List<int>();

            for (int i = 0; i < vertices.Length; ++i)
            {
                verticesList.AddRange(vertices[i]);
                vertexOffsets[i] = verticesList.Count;
                indicesList.AddRange(indices[i]);
                indexOffsets[i] = indicesList.Count;
            }

            return new Mesh(CreateCustomMesh_Native(
                verticesList.ToArray(), vertexOffsets, 
                indicesList.ToArray(), indexOffsets, 
                materialNames)
                );
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr CreatePlane_Native(float width, float height);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr CreateCustomMesh_Native(
            object verticesData, int[] vertexOffsets, 
            int[] indicesData, int[] indexOffsets, 
            string[] matNames
            );

    }
}