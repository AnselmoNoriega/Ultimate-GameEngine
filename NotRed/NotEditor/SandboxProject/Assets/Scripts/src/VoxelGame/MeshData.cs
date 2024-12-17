using NR;
using System.Collections.Generic;

public class MeshData
{
    public List<Vector3> Vertices = new List<Vector3>();
    public List<int> Triangles = new List<int>();
    public List<Vector2> UVs = new List<Vector2>();

    public List<Vector3> ColliderVertices = new List<Vector3>();
    public List<int> ColliderTriangles = new List<int>();

    public MeshData WaterMesh;

    private bool _isMainMesh = true;

    public MeshData(bool isMainMesh)
    {
        if(isMainMesh)
        {
            WaterMesh = new MeshData(false);
        }
    }

    public void AddVertex(Vector3 vertex, bool generateCollider)
    {
        Vertices.Add(vertex);
        if(generateCollider)
        {
            ColliderVertices.Add(vertex);
        }
    }

    public void AddQuadTriangles(bool generateCollider)
    {
        Triangles.Add(Vertices.Count - 4);
        Triangles.Add(Vertices.Count - 3);
        Triangles.Add(Vertices.Count - 2);

        Triangles.Add(Vertices.Count - 4);
        Triangles.Add(Vertices.Count - 2);
        Triangles.Add(Vertices.Count - 1);

        if (generateCollider)
        {
            ColliderTriangles.Add(ColliderVertices.Count - 4);
            ColliderTriangles.Add(ColliderVertices.Count - 3);
            ColliderTriangles.Add(ColliderVertices.Count - 2);

            ColliderTriangles.Add(ColliderVertices.Count - 4);
            ColliderTriangles.Add(ColliderVertices.Count - 2);
            ColliderTriangles.Add(ColliderVertices.Count - 1);
        }
    }
}