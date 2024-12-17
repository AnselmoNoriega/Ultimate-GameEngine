using NR;

public class Chunk
{
    public VoxelType[] Voxels;
    public int Size = 16;
    public int Height = 100;
    public World WorldRef;
    public Vector3 WorldPosition;

    public bool IsModied = false;

    public Chunk(int size, int height, World world, Vector3 worldPosition)
    {
        Size = size;
        Height = height;
        WorldRef = world;
        WorldPosition = worldPosition;
        Voxels = new VoxelType[(size * size) * height];
    }
}