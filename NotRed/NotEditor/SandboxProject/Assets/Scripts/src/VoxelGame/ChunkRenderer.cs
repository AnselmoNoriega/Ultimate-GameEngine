using NR;
using System.Linq;

public class ChunkRenderer : Entity
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

    private void Init()
    {
        //TODO
        _mesh = GetComponent<MeshComponent>();
    }

    public void InitializeChunk(Chunk data)
    {
        ChunkData = data;
    }

    private void RenderMesh(MeshData meshData)
    {
        _mesh.Mesh.Clear();
        var combinedPositions = meshData.Positions.Concat(meshData.WaterMesh.Positions);
        var combinedUVs = meshData.TextureCoords.Concat(meshData.WaterMesh.TextureCoords);

        _mesh.Mesh.SubMeshCount = 2;

        _mesh.Mesh.SetVertices(meshData.Positions.Zip(
            meshData.TextureCoords, (position, uv) => new Vertex{ Position = position, Texcoord = uv })
            .ToArray(), 0);
        _mesh.Mesh.SetVertices(meshData.WaterMesh.Positions.Zip(
            meshData.WaterMesh.TextureCoords, (position, uv) => new Vertex{ Position = position, Texcoord = uv })
            .ToArray(), 1);

        _mesh.Mesh.SetIndices(meshData.Indices.ToArray(), 0);
        _mesh.Mesh.SetIndices(meshData.WaterMesh.Indices.Select(val => val + meshData.Positions.Count).ToArray(), 1);
        
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