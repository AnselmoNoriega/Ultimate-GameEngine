using NR;

public class SurfaceLayerHandler : VoxelLayerHandler
{
    public VoxelType surfaceBlockType = VoxelType.Grass;
    
    protected override bool TryHandling(Chunk chunkData, float x, float y, float z, float surfaceHeightNoise, Vector2 mapSeedOffset)
    {
        Next = new UndergroundLayerHandler();

        if (y == surfaceHeightNoise)
        {
            Vector3 pos = new Vector3(x, y, z);
            ChunkManager.SetVoxel(chunkData, pos, surfaceBlockType);
            return true;
        }

        return false;
    }
}