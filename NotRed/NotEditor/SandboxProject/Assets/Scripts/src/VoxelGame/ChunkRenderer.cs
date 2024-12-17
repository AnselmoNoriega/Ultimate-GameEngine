using NR;
using System.Runtime.CompilerServices;

public class ChunkRenderer : Entity
{
    //TODO
    //MeshFilter MeshFilter;
    MeshCollider MeshCollider;
    Mesh Mesh;

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
        //MeshFilter = GetComponent<MeshFilter>();
        //MeshCollider = GetComponent<MeshCollider>();
        //Mesh = MeshFilter.mesh;
    }
}