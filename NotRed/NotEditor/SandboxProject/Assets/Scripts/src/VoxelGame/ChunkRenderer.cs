using NR;
using System.Runtime.CompilerServices;
using System.Linq;

public class ChunkRenderer : Entity
{
    //TODO
    MeshFilter _meshFilter;
    MeshCollider _meshCollider;
    Mesh _mesh;

    public Chunk ChunkData { get; private set; }

    public bool ModifiedChunk
    {
        get
        {
            return ChunkData.IsModied;
        }
        set
        {
            ChunkData.IsModied = value;
        }
    }

    private void Init()
    {
        //TODO
        MeshFilter = GetComponent<MeshFilter>();
        MeshCollider = GetComponent<MeshCollider>(); // does not inherit from component
        Mesh = MeshFilter.mesh;
    }

    public void InitializeChink(Chunk data)
    {
        ChunkData = data;
    }

    private void RenderMesh(MeshData meshData)
    {
        _mesh.Clear();

        _mesh.subMeshCount = 2;
        _mesh.vertices = meshData.Vertices.Concat(meshData.WaterMesh.Vertices).ToArray();

        _mesh.SetTriangles(meshData.Triangles.ToArray(), 0);
        _mesh.SetTriangles(meshData.WaterMesh.Triangles.Select(val => val + meshData.Vertices.Count).ToArray(), 1);
        
        _mesh.uv = meshData.TextureCoords.Concat(meshData.WaterMesh.TextureCoords).ToArray();
        _mesh.RecalculateNormals();

        _meshCollider.sharedMesh = null;
        
        Mesh collisionMesh = new Mesh();
        collisionMesh.vertices = meshData.ColliderVertices.ToArray();
        collisionMesh.triangles = meshData.ColliderTriangles.ToArray();
        collisionMesh.RecalculateNormals();
        _meshCollider.sharedMesh = collisionMesh;
    }
    public void UpdateChunk()
    {
        RenderMesh(ChunkManager.GetChunkMeshData(ChunkData));
    }

    public void UpdateChunk(MeshData data)
    {
        RenderMesh(data);
    }

#if EDITOR
    private void OnDrawGizmos()
    {
        if (showGizmo)
        {
            if (Application.isPlaying && ChunkData != null)
            {
                if (Selection.activeObject == gameObject)
                    Gizmos.color = new Color(0, 1, 0, 0.4f);
                else
                    Gizmos.color = new Color(1, 0, 1, 0.4f);
                Gizmos.DrawCube(transform.position + Vector3.one * (ChunkData.chunkSize / (float)2 - 0.5f), new Vector3(ChunkData.chunkSize, ChunkData.chunkHeight, ChunkData.chunkSize));
            }
        }
    }
#endif
}