using NR;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace NR
{
    public static class MeshFactory
    {
        public static Mesh CreatePlane(float width, float height)
        {
            return new Mesh(CreatePlane_Native(width, height));
        }

        public static Mesh CreateCustomMesh(Vertex[][] vertices, int[][] indices, string[] materialNames)
        {
            // Indices to know where the next vertices begin and where they end
            int[] vertIndices = new int[vertices.Length];
            int[] indIndices = new int[indices.Length];
            List<Vertex> verticesList = new List<Vertex>();
            List<int> indicesList = new List<int>();

            if (vertices.Length != indices.Length)
            {
                Log.Error("[MeshFactory] Mismatch between vertex count and index count across submeshes.");
                return null;
            }

            for (int i = 0; i < vertices.Length; ++i)
            {
                verticesList.AddRange(vertices[i]);
                vertIndices[i] = verticesList.Count;
                indicesList.AddRange(indices[i]);
                indIndices[i] = indicesList.Count;
            }


            return new Mesh(CreateCustomMesh_Native(width, vertIndices, indicesList.ToArray(), indIndices, materialNames));
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr CreatePlane_Native(float width, float height);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr CreateCustomMesh_Native(byte[] verticesData, int[] vertIndices, int[] indicesData, int[] indIndices, string[] matNames);

    }
}