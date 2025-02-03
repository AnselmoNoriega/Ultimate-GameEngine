using NR;
using System.Collections.Generic;
using System.Threading.Tasks;

public class ChunkRenderer : Entity
{
    private MeshComponent _mesh;

    public Chunk ChunkData { get; private set; }

    public void SetIsModified(bool modified)
    {
        ChunkData.IsModied = modified;
    }

    public bool IsModified()
    {
        return ChunkData.IsModied;
    }

    public void InitializeChunk(Chunk data)
    {
        _mesh = GetComponent<MeshComponent>();
        ChunkData = data;
    }

    private void RenderMesh(MeshData meshData)
    {
        Vertex[][] vertices = new Vertex[2][];

        vertices[0] = new Vertex[meshData.Positions.Count];
        Parallel.For(0, meshData.Positions.Count, i =>
        {
            vertices[0][i] = new Vertex
            {
                Position = new Vector3(
                    Mathf.FloorToInt(meshData.Positions[i].x),
                    Mathf.FloorToInt(meshData.Positions[i].y),
                    Mathf.FloorToInt(meshData.Positions[i].z)
                    ),
                Texcoord = meshData.TextureCoords[i]
            };
        });
        vertices[1] = new Vertex[meshData.WaterMesh.Positions.Count];
        Parallel.For(0, meshData.WaterMesh.Positions.Count, i =>
        {
            vertices[1][i] = new Vertex
            {
                Position = new Vector3(
                    Mathf.FloorToInt(meshData.WaterMesh.Positions[i].x),
                    Mathf.FloorToInt(meshData.WaterMesh.Positions[i].y),
                    Mathf.FloorToInt(meshData.WaterMesh.Positions[i].z)
                    ),
                Texcoord = meshData.WaterMesh.TextureCoords[i]
            };
        });

        int[][] indices = new int[2][];

        indices[0] = new int[meshData.Indices.Count];
        Parallel.For(0, meshData.Indices.Count, i =>
        {
            indices[0][i] = meshData.Indices[i];
        });

        indices[1] = new int[meshData.WaterMesh.Indices.Count];
        Parallel.For(0, meshData.WaterMesh.Indices.Count, i =>
        {
            indices[1][i] = meshData.WaterMesh.Indices[i];
        });

        string[] meshNames = new string[2];
        meshNames[0] = "Textures/Materials/VoxelMat.nrmaterial";
        meshNames[1] = "Textures/Materials/WaterVoxelMat.nrmaterial";

        _mesh.Mesh = MeshFactory.CreateCustomMesh(
            vertices,
            indices,
            meshNames
            );

        _mesh.ReloadMeshCollider();

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