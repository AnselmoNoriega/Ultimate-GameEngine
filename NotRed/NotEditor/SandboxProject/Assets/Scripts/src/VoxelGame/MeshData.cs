﻿using NR;
using System.Collections.Generic;

public class MeshData
{
    public List<Vector3> Positions = new List<Vector3>();
    public List<Vector3> Normals = new List<Vector3>();
    public List<Vector2> TextureCoords = new List<Vector2>();
    public List<int> Indices = new List<int>();

    public MeshData WaterMesh;

    private bool _isMainMesh = true;

    public MeshData(bool isMainMesh)
    {
        if(isMainMesh)
        {
            WaterMesh = new MeshData(false);
        }
    }

    public void AddVertex(Vector3 vertex)
    {
        Positions.Add(vertex);
    }

    public void AddQuadTriangles()
    {
        Indices.Add(Positions.Count - 4);
        Indices.Add(Positions.Count - 3);
        Indices.Add(Positions.Count - 2);

        Indices.Add(Positions.Count - 4);
        Indices.Add(Positions.Count - 2);
        Indices.Add(Positions.Count - 1);

        //Normals
        Vector3 dirA = Positions[Positions.Count - 1] - Positions[Positions.Count - 2];
        Vector3 dirB = Positions[Positions.Count - 2] - Positions[Positions.Count - 3];

        Normals.Add(Vector3.Cross(dirB, dirA).Normalized());
        Normals.Add(Vector3.Cross(dirB, dirA).Normalized());
        Normals.Add(Vector3.Cross(dirB, dirA).Normalized());
        Normals.Add(Vector3.Cross(dirB, dirA).Normalized());
    }
}