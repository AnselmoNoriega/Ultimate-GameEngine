using NR;
using System.Linq;

public class ChunkRenderer
{
    //TODO
    //MeshFilter _meshFilter;
    //MeshCollider _meshCollider;
    MeshComponent _mesh;

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

    public void InitializeChunk(Chunk data, MeshComponent mesh)
    {
        _mesh = mesh;
        ChunkData = data;
    }

    private void RenderMesh(MeshData meshData)
    {
        Vertex[][] vertices = new Vertex[2][];

        vertices[0] = meshData.Positions.Zip(
            meshData.TextureCoords, (position, uv) => new Vertex { Position = position, Texcoord = uv })
            .ToArray();
        vertices[1] = meshData.WaterMesh.Positions.Zip(
            meshData.WaterMesh.TextureCoords, (position, uv) => new Vertex { Position = position, Texcoord = uv })
            .ToArray();

        int[][] indices = new int[2][];
        indices[0] = meshData.Indices.ToArray();
        indices[1] = meshData.WaterMesh.Indices.Select(val => val + meshData.Positions.Count).ToArray();

        string[] meshNames = new string[2];
        meshNames[0] = "VoxelMat";
        meshNames[1] = "WaterVoxelMat";

        _mesh.Mesh = MeshFactory.CreateCustomMesh(
            vertices,
            indices,
            meshNames
            );
        
        //_mesh.RecalculateNormals(); Maybe?

        //_meshCollider.sharedMesh = null;
        
        //Mesh collisionMesh = new Mesh();
        //collisionMesh.vertices = meshData.ColliderVertices.ToArray();
        //collisionMesh.triangles = meshData.ColliderTriangles.ToArray();
        //collisionMesh.RecalculateNormals();
        //_meshCollider.sharedMesh = collisionMesh;
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
                {
                    Gizmos.color = new Color(0, 1, 0, 0.4f);
                }
                else
                {
                    Gizmos.color = new Color(1, 0, 1, 0.4f);
                }

                Gizmos.DrawCube(
                transform.position + new Vector3(ChunkData.chunkSize / 2f, ChunkData.chunkHeight / 2f, ChunkData.chunkSize / 2f), 
                new Vector3(ChunkData.chunkSize, ChunkData.chunkHeight, ChunkData.chunkSize)
                );
            }
        }
    }
#endif
}